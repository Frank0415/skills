/*
 * cexp_worker.c - Self-contained Worker CLI for cexperiment-worker:
 *   - wakeup:     exact 5*X single-job / 30-min multi-job wakeup calculator
 *   - head:       progressive-disclosure header inspection
 *   - pending:    out-of-order & parallel READY spec discovery
 *   - add-result: flock-safe result pointer registration
 *
 * Portable POSIX C11 (Linux musl static & macOS Mach-O native).
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#define _DARWIN_C_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_ROUNDS 256
#define MAX_ROWS 4096

typedef struct {
    int version;
    char title[512];
    char focus[1024];
    char contract[512];
    char created[64];
} QueueHeader;

typedef struct {
    char id[64];
    char mode[64];
    char created[64];
    char summary[512];
} RoundEntry;

typedef struct {
    char round[64];
    char id[64];
    char kind[32];
    char depends[256];
    char resources[256];
    char status[64];
    char pointer[1024];
} PointerRow;

typedef struct {
    QueueHeader hdr;
    bool valid_cexp;
    RoundEntry rounds[MAX_ROUNDS];
    int n_rounds;
    PointerRow rows[MAX_ROWS];
    int n_rows;
} QueueDoc;

static char *trim_inplace(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    return s;
}

static void strip_quotes(char *dst, size_t cap, const char *src) {
    while (*src && isspace((unsigned char)*src)) src++;
    size_t len = strlen(src);
    while (len > 0 && isspace((unsigned char)src[len - 1])) len--;
    if (len >= 2 && ((src[0] == '"' && src[len - 1] == '"') ||
                     (src[0] == '\'' && src[len - 1] == '\''))) {
        src++;
        len -= 2;
    }
    if (len >= cap) len = cap - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

static void sanitize_cell(char *dst, size_t cap, const char *src) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j + 1 < cap; i++) {
        char c = src[i];
        if (c == '|') c = '/';
        if (c == '\n' || c == '\r') c = ' ';
        dst[j++] = c;
    }
    dst[j] = '\0';
}

static void json_escape_print(FILE *out, const char *s) {
    fputc('"', out);
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        switch (*p) {
            case '"': fputs("\\\"", out); break;
            case '\\': fputs("\\\\", out); break;
            case '\n': fputs("\\n", out); break;
            case '\r': fputs("\\r", out); break;
            case '\t': fputs("\\t", out); break;
            default:
                if (*p < 0x20) fprintf(out, "\\u%04x", *p);
                else fputc(*p, out);
        }
    }
    fputc('"', out);
}

static bool is_separator_row(char **cells, int n_cells) {
    if (n_cells == 0) return false;
    for (int i = 0; i < n_cells; i++) {
        const char *c = cells[i];
        if (*c == ':') c++;
        int dashes = 0;
        while (*c == '-') { dashes++; c++; }
        if (*c == ':') c++;
        if (*c != '\0' || dashes < 2) return false;
    }
    return true;
}

static int split_pipe_row(char *line, char **cells, int max_cells) {
    char *s = trim_inplace(line);
    size_t len = strlen(s);
    if (len < 2 || s[0] != '|' || s[len - 1] != '|') return 0;
    s[len - 1] = '\0';
    s++;

    int count = 0;
    char *cur = s;
    while (count < max_cells) {
        char *bar = strchr(cur, '|');
        if (!bar) {
            cells[count++] = trim_inplace(cur);
            break;
        }
        *bar = '\0';
        cells[count++] = trim_inplace(cur);
        cur = bar + 1;
    }
    return count;
}

static char *read_entire_fd(int fd) {
    if (lseek(fd, 0, SEEK_SET) < 0) return NULL;
    size_t cap = 8192, len = 0;
    char *buf = malloc(cap);
    if (!buf) return NULL;
    for (;;) {
        if (len + 4096 + 1 > cap) {
            cap *= 2;
            char *nb = realloc(buf, cap);
            if (!nb) { free(buf); return NULL; }
            buf = nb;
        }
        ssize_t r = read(fd, buf + len, 4096);
        if (r < 0) {
            if (errno == EINTR) continue;
            free(buf);
            return NULL;
        }
        if (r == 0) break;
        len += (size_t)r;
    }
    buf[len] = '\0';
    return buf;
}

static bool parse_queue_text(const char *text, QueueDoc *doc) {
    memset(doc, 0, sizeof(*doc));
    snprintf(doc->hdr.contract, sizeof(doc->hdr.contract), "specs/00-contract.md");

    const char *p = text;
    if (strncmp(p, "---\n", 4) != 0 && strncmp(p, "---\r\n", 5) != 0) return false;
    p = strchr(p, '\n');
    if (!p) return false;
    p++;

    bool fm_closed = false;
    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t l = nl ? (size_t)(nl - p) : strlen(p);
        char line[2048];
        if (l >= sizeof(line)) l = sizeof(line) - 1;
        memcpy(line, p, l);
        line[l] = '\0';
        char *t = trim_inplace(line);
        if (strcmp(t, "---") == 0) {
            fm_closed = true;
            p = nl ? nl + 1 : p + l;
            break;
        }
        char *colon = strchr(t, ':');
        if (colon) {
            *colon = '\0';
            char *key = trim_inplace(t);
            char *val = colon + 1;
            if (strcmp(key, "cexp_queue") == 0) {
                char tmp[64];
                strip_quotes(tmp, sizeof(tmp), val);
                doc->hdr.version = atoi(tmp);
                if (doc->hdr.version >= 1) doc->valid_cexp = true;
            } else if (strcmp(key, "title") == 0) {
                strip_quotes(doc->hdr.title, sizeof(doc->hdr.title), val);
            } else if (strcmp(key, "focus") == 0) {
                strip_quotes(doc->hdr.focus, sizeof(doc->hdr.focus), val);
            } else if (strcmp(key, "contract") == 0) {
                strip_quotes(doc->hdr.contract, sizeof(doc->hdr.contract), val);
            } else if (strcmp(key, "created") == 0) {
                strip_quotes(doc->hdr.created, sizeof(doc->hdr.created), val);
            }
        }
        if (!nl) break;
        p = nl + 1;
    }

    if (!fm_closed || !doc->valid_cexp) return false;

    int section = 0;
    bool table_header_seen = false;

    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t l = nl ? (size_t)(nl - p) : strlen(p);
        char line[2048];
        if (l >= sizeof(line)) l = sizeof(line) - 1;
        memcpy(line, p, l);
        line[l] = '\0';
        char *t = trim_inplace(line);

        if (strncmp(t, "## ", 3) == 0) {
            char *sec = trim_inplace(t + 3);
            if (strcasecmp(sec, "Rounds") == 0) {
                section = 1;
                table_header_seen = false;
            } else if (strcasecmp(sec, "Pointers") == 0) {
                section = 2;
                table_header_seen = false;
            } else {
                section = 0;
            }
        } else if (section > 0 && t[0] == '|') {
            char *cells[12];
            int nc = split_pipe_row(t, cells, 12);
            if (nc > 0 && !is_separator_row(cells, nc)) {
                if (!table_header_seen) {
                    table_header_seen = true;
                } else if (section == 1 && nc >= 4 && doc->n_rounds < MAX_ROUNDS) {
                    RoundEntry *re = &doc->rounds[doc->n_rounds++];
                    snprintf(re->id, sizeof(re->id), "%s", cells[0]);
                    snprintf(re->mode, sizeof(re->mode), "%s", cells[1]);
                    snprintf(re->created, sizeof(re->created), "%s", cells[2]);
                    snprintf(re->summary, sizeof(re->summary), "%s", cells[3]);
                } else if (section == 2 && nc >= 7 && doc->n_rows < MAX_ROWS) {
                    PointerRow *pr = &doc->rows[doc->n_rows++];
                    snprintf(pr->round, sizeof(pr->round), "%s", cells[0]);
                    snprintf(pr->id, sizeof(pr->id), "%s", cells[1]);
                    snprintf(pr->kind, sizeof(pr->kind), "%s", cells[2]);
                    snprintf(pr->depends, sizeof(pr->depends), "%s", cells[3]);
                    snprintf(pr->resources, sizeof(pr->resources), "%s", cells[4]);
                    snprintf(pr->status, sizeof(pr->status), "%s", cells[5]);
                    snprintf(pr->pointer, sizeof(pr->pointer), "%s", cells[6]);
                }
            }
        }
        if (!nl) break;
        p = nl + 1;
    }
    return true;
}

static void parent_dir_of(const char *path, char *out, size_t cap) {
    const char *slash = strrchr(path, '/');
    if (!slash) snprintf(out, cap, ".");
    else if (slash == path) snprintf(out, cap, "/");
    else {
        size_t n = (size_t)(slash - path);
        if (n >= cap) n = cap - 1;
        memcpy(out, path, n);
        out[n] = '\0';
    }
}

static void extract_md_target(const char *pointer_cell, char *out, size_t cap) {
    const char *lb = strchr(pointer_cell, ']');
    if (lb && lb[1] == '(') {
        const char *rp = strchr(lb + 2, ')');
        if (rp) {
            size_t n = (size_t)(rp - (lb + 2));
            if (n >= cap) n = cap - 1;
            memcpy(out, lb + 2, n);
            out[n] = '\0';
            return;
        }
    }
    snprintf(out, cap, "%s", pointer_cell);
}

static int find_result_index(const QueueDoc *doc, const char *spec_id) {
    for (int i = 0; i < doc->n_rows; i++) {
        if (strcasecmp(doc->rows[i].kind, "result") == 0 &&
            strcmp(doc->rows[i].id, spec_id) == 0) {
            return i;
        }
    }
    return -1;
}

static bool has_completed_result(const QueueDoc *doc, const char *spec_id) {
    int idx = find_result_index(doc, spec_id);
    if (idx < 0) return false;
    return strcasecmp(doc->rows[idx].status, "blocked") != 0;
}

static bool deps_satisfied(const QueueDoc *doc, const char *deps_str) {
    if (!deps_str || !*deps_str) return true;
    if (strcasecmp(deps_str, "none") == 0 || strcmp(deps_str, "-") == 0) return true;
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", deps_str);
    char *save = NULL;
    for (char *tok = strtok_r(buf, ",; ", &save); tok; tok = strtok_r(NULL, ",; ", &save)) {
        if (!has_completed_result(doc, tok)) return false;
    }
    return true;
}

static const char *round_mode_of(const QueueDoc *doc, const char *round_id) {
    for (int i = 0; i < doc->n_rounds; i++) {
        if (strcmp(doc->rounds[i].id, round_id) == 0) {
            return doc->rounds[i].mode;
        }
    }
    return "standard";
}

static int write_serialized_doc(int fd, const QueueDoc *doc) {
    if (lseek(fd, 0, SEEK_SET) < 0) return -1;
    if (ftruncate(fd, 0) < 0) return -1;

    FILE *fp = fdopen(dup(fd), "w");
    if (!fp) return -1;

    fprintf(fp, "---\n");
    fprintf(fp, "cexp_queue: %d\n", doc->hdr.version > 0 ? doc->hdr.version : 1);
    fprintf(fp, "title: ");
    json_escape_print(fp, doc->hdr.title);
    fprintf(fp, "\nfocus: ");
    json_escape_print(fp, doc->hdr.focus);
    fprintf(fp, "\ncontract: ");
    json_escape_print(fp, doc->hdr.contract);
    fprintf(fp, "\ncreated: ");
    json_escape_print(fp, doc->hdr.created);
    fprintf(fp, "\n---\n\n");

    fprintf(fp, "## Rounds\n\n");
    fprintf(fp, "| Round | Mode | Created | Summary |\n");
    fprintf(fp, "| --- | --- | --- | --- |\n");
    for (int i = 0; i < doc->n_rounds; i++) {
        const RoundEntry *r = &doc->rounds[i];
        fprintf(fp, "| %s | %s | %s | %s |\n", r->id, r->mode, r->created, r->summary);
    }

    fprintf(fp, "\n## Pointers\n\n");
    fprintf(fp, "| Round | ID | Kind | Depends | Resources | Status | Pointer |\n");
    fprintf(fp, "| --- | --- | --- | --- | --- | --- | --- |\n");
    for (int i = 0; i < doc->n_rows; i++) {
        const PointerRow *pr = &doc->rows[i];
        fprintf(fp, "| %s | %s | %s | %s | %s | %s | %s |\n",
                pr->round, pr->id, pr->kind, pr->depends, pr->resources, pr->status, pr->pointer);
    }

    fflush(fp);
    fsync(fileno(fp));
    fclose(fp);
    return 0;
}

static void print_usage(void) {
    printf(
        "cexp-worker - Self-contained Worker CLI for cexperiment-worker\n\n"
        "USAGE:\n"
        "  cexp-worker <command> [arguments]\n\n"
        "COMMANDS:\n"
        "  wakeup [rem_mins_1 rem_mins_2 ...] [--json]\n"
        "      Compute the mandatory Worker scheduled wakeup period:\n"
        "        - 0 active jobs  -> STOP_ALL_TIMERS (0 min)\n"
        "        - 1 active job   -> X = T_rem / 5, clamped to [15, 30] min\n"
        "        - >=2 active jobs -> fixed 30 min (1800 s)\n\n"
        "  head <queue-file> [--json]\n"
        "      Progressively disclose ONLY the queue's frontmatter header & counts.\n\n"
        "  pending <queue-file> [--round <R_id>] [--only-ready] [--json]\n"
        "      List all uncompleted spec pointers, resolved spec file paths, round mode,\n"
        "      and READY vs. WAITING dependency status for parallel multi-GPU packing.\n\n"
        "  add-result <queue-file> --id <ID> --path <rel-path> [options]\n"
        "      Verify <rel-path> exists on disk, then atomically (flock) register the\n"
        "      paired result pointer row (auto-inheriting Round from the spec row).\n"
        "      Options:\n"
        "        --label <str>     Link text (default: '<ID> Result')\n"
        "        --status <state>  complete | negative | blocked (default: complete)\n"
    );
}

static int cmd_wakeup(int argc, char **argv) {
    double mins[256];
    int n = 0;
    bool as_json = false;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0) as_json = true;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (strcmp(argv[i], "--remaining-mins") == 0) {
            continue;
        } else {
            char *endp = NULL;
            double v = strtod(argv[i], &endp);
            if (endp == argv[i] || *endp != '\0') {
                fprintf(stderr, "ERROR: Invalid numeric duration '%s'\n", argv[i]);
                return 1;
            }
            if (n < 256) mins[n++] = v < 0.0 ? 0.0 : v;
        }
    }

    if (n == 0) {
        if (as_json) {
            printf("{\n  \"active_jobs\": 0,\n  \"action\": \"STOP_ALL_TIMERS\",\n"
                   "  \"period_minutes\": 0,\n  \"period_seconds\": 0,\n"
                   "  \"reason\": \"0 active jobs -> cancel/delete all scheduled timers immediately\"\n}\n");
        } else {
            printf("Action: STOP_ALL_TIMERS\nPeriod: 0 min (0 s)\n"
                   "Reason: 0 active jobs -> cancel/delete all scheduled timers immediately\n");
        }
        return 0;
    }

    if (n >= 2) {
        if (as_json) {
            printf("{\n  \"active_jobs\": %d,\n  \"action\": \"ARM_TIMER\",\n"
                   "  \"period_minutes\": 30,\n  \"period_seconds\": 1800,\n"
                   "  \"cron_expression\": \"*/30 * * * *\",\n"
                   "  \"reason\": \"%d concurrent active jobs (>=2) -> fixed 30-minute wakeup\"\n}\n",
                   n, n);
        } else {
            printf("Action: ARM_TIMER\nPeriod: 30 min (1800 s)\nCron:   */30 * * * *\n"
                   "Reason: %d concurrent active jobs (>=2) -> fixed 30-minute wakeup\n", n);
        }
        return 0;
    }

    double t_rem = mins[0];
    double x = t_rem / 5.0;
    int period = 15;
    char note[128];
    if (x < 15.0) {
        period = 15;
        snprintf(note, sizeof(note), "1 active job (5*X=%.1fm, X=%.2fm < 15m) -> clamped to 15m", t_rem, x);
    } else if (x > 30.0) {
        period = 30;
        snprintf(note, sizeof(note), "1 active job (5*X=%.1fm, X=%.2fm > 30m) -> clamped to 30m", t_rem, x);
    } else {
        period = (int)(x + 0.5);
        if (period < 15) period = 15;
        if (period > 30) period = 30;
        snprintf(note, sizeof(note), "1 active job (5*X=%.1fm, 15m <= X=%.2fm <= 30m) -> %dm", t_rem, x, period);
    }

    if (as_json) {
        printf("{\n  \"active_jobs\": 1,\n  \"action\": \"ARM_TIMER\",\n"
               "  \"estimated_remaining_minutes\": %.2f,\n  \"one_fifth_x_minutes\": %.2f,\n"
               "  \"period_minutes\": %d,\n  \"period_seconds\": %d,\n"
               "  \"cron_expression\": \"*/%d * * * *\",\n  \"reason\": ",
               t_rem, x, period, period * 60, period);
        json_escape_print(stdout, note);
        printf("\n}\n");
    } else {
        printf("Action: ARM_TIMER\nPeriod: %d min (%d s)\nCron:   */%d * * * *\nReason: %s\n",
               period, period * 60, period, note);
    }
    return 0;
}

static int cmd_head(int argc, char **argv) {
    const char *qfile = NULL;
    bool as_json = false;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0) as_json = true;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (!qfile && argv[i][0] != '-') qfile = argv[i];
    }
    if (!qfile) {
        fprintf(stderr, "ERROR: head requires <queue-file>\n");
        return 1;
    }

    int fd = open(qfile, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open '%s': %s\n", qfile, strerror(errno));
        return 1;
    }
    char *txt = read_entire_fd(fd);
    close(fd);
    QueueDoc *doc = calloc(1, sizeof(QueueDoc));
    if (!txt || !doc || !parse_queue_text(txt, doc)) {
        fprintf(stderr, "ERROR: '%s' is not a valid cexp_queue file.\n", qfile);
        free(txt);
        free(doc);
        return 1;
    }
    free(txt);

    int specs = 0, done = 0, pend = 0;
    for (int i = 0; i < doc->n_rows; i++) {
        if (strcasecmp(doc->rows[i].kind, "spec") == 0) {
            specs++;
            if (has_completed_result(doc, doc->rows[i].id)) done++;
            else pend++;
        }
    }

    if (as_json) {
        printf("{\n  \"title\": ");
        json_escape_print(stdout, doc->hdr.title);
        printf(",\n  \"focus\": ");
        json_escape_print(stdout, doc->hdr.focus);
        printf(",\n  \"contract\": ");
        json_escape_print(stdout, doc->hdr.contract);
        printf(",\n  \"rounds\": %d, \"specs\": %d, \"completed\": %d, \"pending\": %d\n}\n",
               doc->n_rounds, specs, done, pend);
    } else {
        printf("Title:    %s\nFocus:    %s\nContract: %s\nSummary:  rounds=%d, specs=%d, completed=%d, pending=%d\n",
               doc->hdr.title, doc->hdr.focus, doc->hdr.contract, doc->n_rounds, specs, done, pend);
    }
    free(doc);
    return 0;
}

static int cmd_pending(int argc, char **argv) {
    const char *qfile = NULL;
    const char *filter_round = NULL;
    bool only_ready = false;
    bool as_json = false;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--round") == 0 && i + 1 < argc) filter_round = argv[++i];
        else if (strcmp(argv[i], "--only-ready") == 0) only_ready = true;
        else if (strcmp(argv[i], "--json") == 0) as_json = true;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (!qfile && argv[i][0] != '-') qfile = argv[i];
    }
    if (!qfile) {
        fprintf(stderr, "ERROR: pending requires <queue-file>\n");
        return 1;
    }

    int fd = open(qfile, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open '%s': %s\n", qfile, strerror(errno));
        return 1;
    }
    char *txt = read_entire_fd(fd);
    close(fd);
    QueueDoc *doc = calloc(1, sizeof(QueueDoc));
    if (!txt || !doc || !parse_queue_text(txt, doc)) {
        fprintf(stderr, "ERROR: '%s' is not a valid cexp_queue file.\n", qfile);
        free(txt);
        free(doc);
        return 1;
    }
    free(txt);

    char qdir[PATH_MAX];
    parent_dir_of(qfile, qdir, sizeof(qdir));

    int ready_cnt = 0, waiting_cnt = 0, blocked_cnt = 0;

    if (as_json) {
        printf("{\n  \"contract\": ");
        char cpath[PATH_MAX];
        snprintf(cpath, sizeof(cpath), "%s/%s", qdir, doc->hdr.contract);
        json_escape_print(stdout, cpath);
        printf(",\n  \"items\": [\n");
        bool first = true;
        for (int i = 0; i < doc->n_rows; i++) {
            const PointerRow *sp = &doc->rows[i];
            if (strcasecmp(sp->kind, "spec") != 0) continue;
            if (filter_round && strcmp(sp->round, filter_round) != 0) continue;
            int ridx = find_result_index(doc, sp->id);
            bool is_blocked = (ridx >= 0 && strcasecmp(doc->rows[ridx].status, "blocked") == 0);
            if (ridx >= 0 && !is_blocked) continue; /* completed */

            bool rdy = !is_blocked && deps_satisfied(doc, sp->depends);
            if (only_ready && !rdy) continue;

            const char *state = is_blocked ? "BLOCKED" : (rdy ? "READY" : "WAITING");
            char rel_t[512], full_t[PATH_MAX];
            extract_md_target(sp->pointer, rel_t, sizeof(rel_t));
            snprintf(full_t, sizeof(full_t), "%s/%s", qdir, rel_t);

            if (!first) printf(",\n");
            first = false;
            printf("    {\"round\": ");
            json_escape_print(stdout, sp->round);
            printf(", \"mode\": ");
            json_escape_print(stdout, round_mode_of(doc, sp->round));
            printf(", \"id\": ");
            json_escape_print(stdout, sp->id);
            printf(", \"state\": ");
            json_escape_print(stdout, state);
            printf(", \"depends\": ");
            json_escape_print(stdout, sp->depends);
            printf(", \"resources\": ");
            json_escape_print(stdout, sp->resources);
            printf(", \"spec_path\": ");
            json_escape_print(stdout, full_t);
            printf("}");
        }
        printf("\n  ]\n}\n");
    } else {
        printf("Contract: %s/%s\n", qdir, doc->hdr.contract);
        for (int i = 0; i < doc->n_rows; i++) {
            const PointerRow *sp = &doc->rows[i];
            if (strcasecmp(sp->kind, "spec") != 0) continue;
            if (filter_round && strcmp(sp->round, filter_round) != 0) continue;
            int ridx = find_result_index(doc, sp->id);
            bool is_blocked = (ridx >= 0 && strcasecmp(doc->rows[ridx].status, "blocked") == 0);
            if (ridx >= 0 && !is_blocked) continue;

            bool rdy = !is_blocked && deps_satisfied(doc, sp->depends);
            if (is_blocked) blocked_cnt++;
            else if (rdy) ready_cnt++;
            else waiting_cnt++;

            if (only_ready && !rdy) continue;

            const char *state = is_blocked ? "BLOCKED" : (rdy ? "READY" : "WAITING");
            char rel_t[512];
            extract_md_target(sp->pointer, rel_t, sizeof(rel_t));
            printf("  [%-7s] %-6s (round=%s, mode=%s, res=%s, dep=%s) -> %s/%s\n",
                   state, sp->id, sp->round, round_mode_of(doc, sp->round),
                   sp->resources, sp->depends, qdir, rel_t);
        }
        printf("Summary: ready=%d, waiting_on_deps=%d, blocked=%d\n",
               ready_cnt, waiting_cnt, blocked_cnt);
    }
    free(doc);
    return 0;
}

static int cmd_add_result(int argc, char **argv) {
    const char *qfile = NULL;
    const char *id = NULL;
    const char *rel_path = NULL;
    const char *label = NULL;
    const char *status = "complete";

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--id") == 0 && i + 1 < argc) id = argv[++i];
        else if (strcmp(argv[i], "--path") == 0 && i + 1 < argc) rel_path = argv[++i];
        else if (strcmp(argv[i], "--label") == 0 && i + 1 < argc) label = argv[++i];
        else if (strcmp(argv[i], "--status") == 0 && i + 1 < argc) status = argv[++i];
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (!qfile && argv[i][0] != '-') qfile = argv[i];
    }

    if (!qfile || !id || !rel_path) {
        fprintf(stderr, "ERROR: add-result requires <queue-file> --id <ID> --path <rel-path>\n");
        return 1;
    }

    if (strcasecmp(status, "complete") != 0 &&
        strcasecmp(status, "negative") != 0 &&
        strcasecmp(status, "blocked") != 0) {
        fprintf(stderr, "ERROR: --status must be complete, negative, or blocked.\n");
        return 1;
    }

    char qdir[PATH_MAX], full_res[PATH_MAX];
    parent_dir_of(qfile, qdir, sizeof(qdir));
    snprintf(full_res, sizeof(full_res), "%s/%s", qdir, rel_path);

    struct stat st;
    if (stat(full_res, &st) != 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "ERROR: Result file '%s' does not exist yet. Write results/<ID>.md BEFORE registering its pointer.\n", full_res);
        return 1;
    }

    int fd = open(qfile, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open '%s': %s\n", qfile, strerror(errno));
        return 1;
    }
    flock(fd, LOCK_EX);

    char *txt = read_entire_fd(fd);
    QueueDoc *doc = calloc(1, sizeof(QueueDoc));
    if (!txt || !doc || !parse_queue_text(txt, doc)) {
        fprintf(stderr, "ERROR: '%s' is not a valid cexp_queue file.\n", qfile);
        free(txt);
        free(doc);
        flock(fd, LOCK_UN);
        close(fd);
        return 1;
    }
    free(txt);

    const PointerRow *spec_row = NULL;
    for (int i = 0; i < doc->n_rows; i++) {
        if (strcasecmp(doc->rows[i].kind, "spec") == 0 && strcmp(doc->rows[i].id, id) == 0) {
            spec_row = &doc->rows[i];
            break;
        }
    }
    if (!spec_row) {
        fprintf(stderr, "ERROR: Spec ID '%s' not found in '%s'.\n", id, qfile);
        free(doc);
        flock(fd, LOCK_UN);
        close(fd);
        return 1;
    }

    char auto_label[256];
    if (!label) {
        snprintf(auto_label, sizeof(auto_label), "%s Result", id);
        label = auto_label;
    }
    char clean_label[512], clean_path[512], clean_status[64];
    sanitize_cell(clean_label, sizeof(clean_label), label);
    sanitize_cell(clean_path, sizeof(clean_path), rel_path);
    sanitize_cell(clean_status, sizeof(clean_status), status);

    int existing_ridx = find_result_index(doc, id);
    if (existing_ridx >= 0) {
        if (strcasecmp(doc->rows[existing_ridx].status, "blocked") == 0) {
            /* Upgrade previously blocked entry upon resolution */
            PointerRow *pr = &doc->rows[existing_ridx];
            snprintf(pr->status, sizeof(pr->status), "%s", clean_status);
            snprintf(pr->pointer, sizeof(pr->pointer), "[%s](%s)", clean_label, clean_path);
        } else {
            fprintf(stderr, "ERROR: Completed result pointer for '%s' already exists. History is immutable.\n", id);
            free(doc);
            flock(fd, LOCK_UN);
            close(fd);
            return 1;
        }
    } else {
        if (doc->n_rows >= MAX_ROWS) {
            fprintf(stderr, "ERROR: Maximum rows reached.\n");
            free(doc);
            flock(fd, LOCK_UN);
            close(fd);
            return 1;
        }
        PointerRow *pr = &doc->rows[doc->n_rows++];
        snprintf(pr->round, sizeof(pr->round), "%s", spec_row->round);
        snprintf(pr->id, sizeof(pr->id), "%s", id);
        snprintf(pr->kind, sizeof(pr->kind), "result");
        snprintf(pr->depends, sizeof(pr->depends), "-");
        snprintf(pr->resources, sizeof(pr->resources), "-");
        snprintf(pr->status, sizeof(pr->status), "%s", clean_status);
        snprintf(pr->pointer, sizeof(pr->pointer), "[%s](%s)", clean_label, clean_path);
    }

    int rc = write_serialized_doc(fd, doc);
    flock(fd, LOCK_UN);
    close(fd);
    free(doc);

    if (rc != 0) return 1;
    printf("Registered result %s [%s] -> %s\n", id, clean_status, clean_path);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "help") == 0) {
        print_usage();
        return 0;
    }
    const char *cmd = argv[1];
    if (strcmp(cmd, "wakeup") == 0 || strcmp(cmd, "calc-wakeup") == 0) return cmd_wakeup(argc, argv);
    if (strcmp(cmd, "head") == 0) return cmd_head(argc, argv);
    if (strcmp(cmd, "pending") == 0) return cmd_pending(argc, argv);
    if (strcmp(cmd, "add-result") == 0 || strcmp(cmd, "append-result") == 0) return cmd_add_result(argc, argv);

    fprintf(stderr, "ERROR: Unknown command '%s'. Run 'cexp-worker --help' for usage.\n", cmd);
    return 1;
}
