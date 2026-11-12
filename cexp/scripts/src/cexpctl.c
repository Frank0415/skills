/*
 * cexpctl.c - Self-contained Commander CLI for cexp queue discovery,
 *             progressive-disclosure headers, round management, and
 *             flock-safe pointer manipulation.
 *
 * Portable POSIX C11 (Linux musl static & macOS Mach-O native).
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#define _DARWIN_C_SOURCE

#include <ctype.h>
#include <dirent.h>
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
#include <time.h>
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
    char kind[32];      /* "spec" or "result" */
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

static void today_iso(char *buf, size_t cap) {
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    strftime(buf, cap, "%Y-%m-%d", &tmv);
}

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

static char *read_entire_fd(int fd, size_t *out_len) {
    if (lseek(fd, 0, SEEK_SET) < 0) return NULL;
    size_t cap = 8192;
    size_t len = 0;
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
    if (out_len) *out_len = len;
    return buf;
}

static bool parse_queue_text(const char *text, QueueDoc *doc, bool header_only) {
    memset(doc, 0, sizeof(*doc));
    snprintf(doc->hdr.contract, sizeof(doc->hdr.contract), "specs/00-contract.md");

    const char *p = text;
    if (strncmp(p, "---\n", 4) != 0 && strncmp(p, "---\r\n", 5) != 0) {
        return false;
    }
    p = strchr(p, '\n');
    if (!p) return false;
    p++;

    /* Parse YAML frontmatter */
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
    if (header_only) return true;

    /* Parse sections: ## Rounds and ## Pointers */
    int section = 0; /* 1 = rounds, 2 = pointers */
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

static bool load_queue_file(const char *path, QueueDoc *doc, bool header_only) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return false;
    char *txt = NULL;
    if (header_only) {
        txt = malloc(4097);
        if (!txt) { close(fd); return false; }
        ssize_t n = read(fd, txt, 4096);
        if (n < 0) { free(txt); close(fd); return false; }
        txt[n] = '\0';
    } else {
        txt = read_entire_fd(fd, NULL);
        if (!txt) { close(fd); return false; }
    }
    close(fd);
    bool ok = parse_queue_text(txt, doc, header_only);
    free(txt);
    return ok;
}

static int mkdir_p(const char *dir) {
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", dir);
    size_t len = strlen(tmp);
    if (len == 0) return 0;
    if (tmp[len - 1] == '/') tmp[len - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
    return 0;
}

static void parent_dir_of(const char *path, char *out, size_t cap) {
    const char *slash = strrchr(path, '/');
    if (!slash) {
        snprintf(out, cap, ".");
    } else if (slash == path) {
        snprintf(out, cap, "/");
    } else {
        size_t n = (size_t)(slash - path);
        if (n >= cap) n = cap - 1;
        memcpy(out, path, n);
        out[n] = '\0';
    }
}

static void compute_next_round_id(const QueueDoc *doc, char *out, size_t cap) {
    int max_r = 0;
    for (int i = 0; i < doc->n_rounds; i++) {
        const char *id = doc->rounds[i].id;
        if ((id[0] == 'R' || id[0] == 'r') && isdigit((unsigned char)id[1])) {
            int v = atoi(id + 1);
            if (v > max_r) max_r = v;
        }
    }
    snprintf(out, cap, "R%d", max_r + 1);
}

static void compute_next_spec_id(const QueueDoc *doc, const char *default_prefix, char *out, size_t cap) {
    char prefix[32];
    snprintf(prefix, sizeof(prefix), "%s", default_prefix ? default_prefix : "E");
    int max_num = 0;
    int width = 3;

    for (int i = 0; i < doc->n_rows; i++) {
        const char *id = doc->rows[i].id;
        size_t plen = 0;
        while (isalpha((unsigned char)id[plen])) plen++;
        if (plen > 0 && isdigit((unsigned char)id[plen])) {
            size_t dlen = 0;
            while (isdigit((unsigned char)id[plen + dlen])) dlen++;
            if (id[plen + dlen] == '\0') {
                if (plen < sizeof(prefix)) {
                    memcpy(prefix, id, plen);
                    prefix[plen] = '\0';
                }
                if ((int)dlen > width) width = (int)dlen;
                int v = atoi(id + plen);
                if (v > max_num) max_num = v;
            }
        }
    }
    snprintf(out, cap, "%s%0*d", prefix, width, max_num + 1);
}

static bool has_completed_result(const QueueDoc *doc, const char *spec_id) {
    for (int i = 0; i < doc->n_rows; i++) {
        if (strcasecmp(doc->rows[i].kind, "result") == 0 &&
            strcmp(doc->rows[i].id, spec_id) == 0 &&
            strcasecmp(doc->rows[i].status, "blocked") != 0) {
            return true;
        }
    }
    return false;
}

static const PointerRow *find_result_row(const QueueDoc *doc, const char *spec_id) {
    for (int i = 0; i < doc->n_rows; i++) {
        if (strcasecmp(doc->rows[i].kind, "result") == 0 &&
            strcmp(doc->rows[i].id, spec_id) == 0) {
            return &doc->rows[i];
        }
    }
    return NULL;
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

static void count_doc_stats(const QueueDoc *doc, int *out_specs, int *out_done, int *out_pending, int *out_blocked) {
    int specs = 0, done = 0, pend = 0, blk = 0;
    for (int i = 0; i < doc->n_rows; i++) {
        if (strcasecmp(doc->rows[i].kind, "spec") == 0) {
            specs++;
            const PointerRow *res = find_result_row(doc, doc->rows[i].id);
            if (!res) pend++;
            else if (strcasecmp(res->status, "blocked") == 0) blk++;
            else done++;
        }
    }
    if (out_specs) *out_specs = specs;
    if (out_done) *out_done = done;
    if (out_pending) *out_pending = pend;
    if (out_blocked) *out_blocked = blk;
}

/* --- Recursive Discovery --- */
typedef struct {
    char path[PATH_MAX];
    QueueHeader hdr;
    int n_rounds;
    int n_specs;
    int n_done;
    int n_pending;
    char latest_round[64];
} DiscoveredQueue;

#define MAX_DISCOVERED 256

static bool should_skip_dir(const char *name) {
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return true;
    if (name[0] == '.') return true;
    if (strcmp(name, "node_modules") == 0 || strcmp(name, "__pycache__") == 0 ||
        strcmp(name, "target") == 0 || strcmp(name, "build") == 0 ||
        strcmp(name, "dist") == 0 || strcmp(name, "venv") == 0) {
        return true;
    }
    return false;
}

static bool has_md_ext(const char *name) {
    size_t l = strlen(name);
    return l > 3 && strcmp(name + l - 3, ".md") == 0;
}

static void walk_discover(const char *dir, int depth, DiscoveredQueue *list, int *count) {
    if (depth > 12 || *count >= MAX_DISCOVERED) return;
    DIR *dp = opendir(dir);
    if (!dp) return;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (*count >= MAX_DISCOVERED) break;
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

        char full[PATH_MAX];
        if (strcmp(dir, "/") == 0) snprintf(full, sizeof(full), "/%s", de->d_name);
        else snprintf(full, sizeof(full), "%s/%s", dir, de->d_name);

        struct stat st;
        if (lstat(full, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            if (!should_skip_dir(de->d_name)) {
                walk_discover(full, depth + 1, list, count);
            }
        } else if (S_ISREG(st.st_mode) && has_md_ext(de->d_name)) {
            QueueDoc *doc = calloc(1, sizeof(QueueDoc));
            if (!doc) continue;
            if (load_queue_file(full, doc, true)) {
                /* Load full counts only for confirmed cexp_queue files */
                if (load_queue_file(full, doc, false)) {
                    DiscoveredQueue *dq = &list[(*count)++];
                    if (!realpath(full, dq->path)) {
                        snprintf(dq->path, sizeof(dq->path), "%s", full);
                    }
                    dq->hdr = doc->hdr;
                    dq->n_rounds = doc->n_rounds;
                    count_doc_stats(doc, &dq->n_specs, &dq->n_done, &dq->n_pending, NULL);
                    if (doc->n_rounds > 0) {
                        snprintf(dq->latest_round, sizeof(dq->latest_round), "%s",
                                 doc->rounds[doc->n_rounds - 1].id);
                    } else {
                        snprintf(dq->latest_round, sizeof(dq->latest_round), "-");
                    }
                }
            }
            free(doc);
        }
    }
    closedir(dp);
}

static void print_usage(void) {
    printf(
        "cexpctl - Self-contained Commander CLI for cexp pointer queues\n\n"
        "USAGE:\n"
        "  cexpctl <command> [arguments]\n\n"
        "COMMANDS:\n"
        "  discover [dir] [--json]\n"
        "      Recursively find all cexp queue documents under [dir] (default '.')\n"
        "      and progressively disclose ONLY each queue's focus header & counts.\n\n"
        "  head <queue-file> [--json]\n"
        "      Progressively disclose ONLY a queue file's frontmatter (title, focus,\n"
        "      contract, created) and compact summary counts without dumping rows.\n\n"
        "  init <queue-file> --title <str> --focus <str> [options]\n"
        "      Create a new cexp_queue v1 document and sibling specs/ results/ artifacts/.\n"
        "      Options:\n"
        "        --contract <rel-path>      Shared contract path (default: specs/00-contract.md)\n"
        "        --round-summary <str>      Summary for initial Round R1 (default: Initial batch)\n"
        "        --mode <standard|recursive-improve>  Mode for R1 (default: standard)\n"
        "        --force                    Overwrite if file already exists\n\n"
        "  new-round <queue-file> --summary <str> [--mode <standard|recursive-improve>] [--round <R_id>]\n"
        "      Atomically (flock) register a new experiment round (e.g. R2) for clear\n"
        "      round-by-round separation in multi-batch campaigns.\n\n"
        "  add-spec <queue-file> --label <str> --path <rel-path> [options]\n"
        "      Atomically (flock) append a spec pointer row.\n"
        "      Options:\n"
        "        --id <ID>                  Explicit ID (auto-increments E001/E002 if omitted)\n"
        "        --round <R_id>             Target round (defaults to latest round)\n"
        "        --depends <ids|none>       Prerequisite IDs or 'none' (default: none)\n"
        "        --resources <profile>      Short resource tag, e.g. 1gpu:32G:excl (default: 1gpu)\n"
        "        --prefix <alpha>           ID prefix when queue is empty (default: E)\n\n"
        "  status <queue-file> [--round <R_id>] [--json]\n"
        "      Inspect rounds, completed pairs, pending/ready specs, and next IDs.\n"
        "      Pass --round R1 to isolate a specific round for reporting.\n"
    );
}

static int cmd_discover(int argc, char **argv) {
    const char *dir = ".";
    bool as_json = false;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0) as_json = true;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (argv[i][0] != '-') dir = argv[i];
    }

    DiscoveredQueue *found = calloc(MAX_DISCOVERED, sizeof(DiscoveredQueue));
    if (!found) return 1;
    int count = 0;
    walk_discover(dir, 0, found, &count);

    if (as_json) {
        printf("{\n  \"count\": %d,\n  \"queues\": [\n", count);
        for (int i = 0; i < count; i++) {
            const DiscoveredQueue *q = &found[i];
            printf("    {\n      \"path\": ");
            json_escape_print(stdout, q->path);
            printf(",\n      \"title\": ");
            json_escape_print(stdout, q->hdr.title);
            printf(",\n      \"focus\": ");
            json_escape_print(stdout, q->hdr.focus);
            printf(",\n      \"contract\": ");
            json_escape_print(stdout, q->hdr.contract);
            printf(",\n      \"created\": ");
            json_escape_print(stdout, q->hdr.created);
            printf(",\n      \"rounds\": %d, \"latest_round\": ", q->n_rounds);
            json_escape_print(stdout, q->latest_round);
            printf(", \"specs\": %d, \"completed\": %d, \"pending\": %d\n    }%s\n",
                   q->n_specs, q->n_done, q->n_pending, (i + 1 < count) ? "," : "");
        }
        printf("  ]\n}\n");
    } else {
        if (count == 0) {
            printf("No cexp queue files found under '%s'.\n", dir);
        } else {
            printf("Discovered %d cexp queue(s) under '%s':\n", count, dir);
            for (int i = 0; i < count; i++) {
                const DiscoveredQueue *q = &found[i];
                printf("\n[%d] %s\n", i + 1, q->path);
                printf("    Title:   %s\n", q->hdr.title);
                printf("    Focus:   %s\n", q->hdr.focus);
                printf("    Stats:   rounds=%d (latest=%s), specs=%d, completed=%d, pending=%d\n",
                       q->n_rounds, q->latest_round, q->n_specs, q->n_done, q->n_pending);
            }
        }
    }
    free(found);
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

    QueueDoc *doc = calloc(1, sizeof(QueueDoc));
    if (!doc) return 1;
    if (!load_queue_file(qfile, doc, false)) {
        fprintf(stderr, "ERROR: '%s' is missing or not a valid cexp_queue file.\n", qfile);
        free(doc);
        return 1;
    }

    int specs = 0, done = 0, pend = 0, blk = 0;
    count_doc_stats(doc, &specs, &done, &pend, &blk);

    if (as_json) {
        printf("{\n  \"cexp_queue\": %d,\n  \"title\": ", doc->hdr.version);
        json_escape_print(stdout, doc->hdr.title);
        printf(",\n  \"focus\": ");
        json_escape_print(stdout, doc->hdr.focus);
        printf(",\n  \"contract\": ");
        json_escape_print(stdout, doc->hdr.contract);
        printf(",\n  \"created\": ");
        json_escape_print(stdout, doc->hdr.created);
        printf(",\n  \"rounds\": %d, \"specs\": %d, \"completed\": %d, \"pending\": %d, \"blocked\": %d\n}\n",
               doc->n_rounds, specs, done, pend, blk);
    } else {
        printf("Title:    %s\n", doc->hdr.title);
        printf("Focus:    %s\n", doc->hdr.focus);
        printf("Contract: %s\n", doc->hdr.contract);
        printf("Created:  %s\n", doc->hdr.created);
        printf("Summary:  rounds=%d, specs=%d, completed=%d, pending=%d, blocked=%d\n",
               doc->n_rounds, specs, done, pend, blk);
    }
    free(doc);
    return 0;
}

static int cmd_init(int argc, char **argv) {
    const char *qfile = NULL;
    const char *title = NULL;
    const char *focus = NULL;
    const char *contract = "specs/00-contract.md";
    const char *round_sum = "Initial batch";
    const char *mode = "standard";
    bool force = false;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--title") == 0 && i + 1 < argc) title = argv[++i];
        else if (strcmp(argv[i], "--focus") == 0 && i + 1 < argc) focus = argv[++i];
        else if (strcmp(argv[i], "--contract") == 0 && i + 1 < argc) contract = argv[++i];
        else if (strcmp(argv[i], "--round-summary") == 0 && i + 1 < argc) round_sum = argv[++i];
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) mode = argv[++i];
        else if (strcmp(argv[i], "--force") == 0) force = true;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (!qfile && argv[i][0] != '-') qfile = argv[i];
    }

    if (!qfile || !title || !focus) {
        fprintf(stderr, "ERROR: init requires <queue-file> --title <str> --focus <str>\n");
        return 1;
    }

    struct stat st;
    if (stat(qfile, &st) == 0 && !force) {
        fprintf(stderr, "ERROR: Queue file '%s' already exists. Reuse it via new-round/add-spec or pass --force.\n", qfile);
        return 1;
    }

    char pdir[PATH_MAX];
    parent_dir_of(qfile, pdir, sizeof(pdir));
    if (mkdir_p(pdir) != 0) {
        fprintf(stderr, "ERROR: Failed to create directory '%s'\n", pdir);
        return 1;
    }

    const char *subdirs[3] = {"specs", "results", "artifacts"};
    for (int i = 0; i < 3; i++) {
        char sub[PATH_MAX];
        snprintf(sub, sizeof(sub), "%s/%s", pdir, subdirs[i]);
        mkdir_p(sub);
    }

    QueueDoc *doc = calloc(1, sizeof(QueueDoc));
    if (!doc) return 1;
    doc->hdr.version = 1;
    snprintf(doc->hdr.title, sizeof(doc->hdr.title), "%s", title);
    snprintf(doc->hdr.focus, sizeof(doc->hdr.focus), "%s", focus);
    snprintf(doc->hdr.contract, sizeof(doc->hdr.contract), "%s", contract);
    today_iso(doc->hdr.created, sizeof(doc->hdr.created));

    doc->n_rounds = 1;
    snprintf(doc->rounds[0].id, sizeof(doc->rounds[0].id), "R1");
    sanitize_cell(doc->rounds[0].mode, sizeof(doc->rounds[0].mode), mode);
    snprintf(doc->rounds[0].created, sizeof(doc->rounds[0].created), "%s", doc->hdr.created);
    sanitize_cell(doc->rounds[0].summary, sizeof(doc->rounds[0].summary), round_sum);

    int fd = open(qfile, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open '%s' for writing: %s\n", qfile, strerror(errno));
        free(doc);
        return 1;
    }
    flock(fd, LOCK_EX);
    int rc = write_serialized_doc(fd, doc);
    flock(fd, LOCK_UN);
    close(fd);
    free(doc);

    if (rc != 0) {
        fprintf(stderr, "ERROR: Failed writing queue document '%s'\n", qfile);
        return 1;
    }
    printf("Initialized cexp queue '%s' (Round R1 [%s])\n", qfile, mode);
    return 0;
}

static int cmd_new_round(int argc, char **argv) {
    const char *qfile = NULL;
    const char *summary = NULL;
    const char *mode = "standard";
    const char *explicit_id = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--summary") == 0 && i + 1 < argc) summary = argv[++i];
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) mode = argv[++i];
        else if (strcmp(argv[i], "--round") == 0 && i + 1 < argc) explicit_id = argv[++i];
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (!qfile && argv[i][0] != '-') qfile = argv[i];
    }

    if (!qfile || !summary) {
        fprintf(stderr, "ERROR: new-round requires <queue-file> --summary <str>\n");
        return 1;
    }

    int fd = open(qfile, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open '%s': %s\n", qfile, strerror(errno));
        return 1;
    }
    flock(fd, LOCK_EX);

    char *txt = read_entire_fd(fd, NULL);
    QueueDoc *doc = calloc(1, sizeof(QueueDoc));
    if (!txt || !doc || !parse_queue_text(txt, doc, false)) {
        fprintf(stderr, "ERROR: '%s' is not a valid cexp_queue file.\n", qfile);
        free(txt);
        free(doc);
        flock(fd, LOCK_UN);
        close(fd);
        return 1;
    }
    free(txt);

    if (doc->n_rounds >= MAX_ROUNDS) {
        fprintf(stderr, "ERROR: Maximum rounds reached.\n");
        free(doc);
        flock(fd, LOCK_UN);
        close(fd);
        return 1;
    }

    char rid[64];
    if (explicit_id) sanitize_cell(rid, sizeof(rid), explicit_id);
    else compute_next_round_id(doc, rid, sizeof(rid));

    for (int i = 0; i < doc->n_rounds; i++) {
        if (strcmp(doc->rounds[i].id, rid) == 0) {
            fprintf(stderr, "ERROR: Round '%s' already exists in '%s'.\n", rid, qfile);
            free(doc);
            flock(fd, LOCK_UN);
            close(fd);
            return 1;
        }
    }

    RoundEntry *re = &doc->rounds[doc->n_rounds++];
    snprintf(re->id, sizeof(re->id), "%s", rid);
    sanitize_cell(re->mode, sizeof(re->mode), mode);
    today_iso(re->created, sizeof(re->created));
    sanitize_cell(re->summary, sizeof(re->summary), summary);

    int rc = write_serialized_doc(fd, doc);
    flock(fd, LOCK_UN);
    close(fd);
    free(doc);

    if (rc != 0) return 1;
    printf("Added round %s (%s): %s\n", rid, mode, summary);
    return 0;
}

static int cmd_add_spec(int argc, char **argv) {
    const char *qfile = NULL;
    const char *label = NULL;
    const char *rel_path = NULL;
    const char *explicit_id = NULL;
    const char *round_id = NULL;
    const char *depends = "none";
    const char *resources = "1gpu";
    const char *prefix = "E";

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--label") == 0 && i + 1 < argc) label = argv[++i];
        else if (strcmp(argv[i], "--path") == 0 && i + 1 < argc) rel_path = argv[++i];
        else if (strcmp(argv[i], "--id") == 0 && i + 1 < argc) explicit_id = argv[++i];
        else if (strcmp(argv[i], "--round") == 0 && i + 1 < argc) round_id = argv[++i];
        else if (strcmp(argv[i], "--depends") == 0 && i + 1 < argc) depends = argv[++i];
        else if (strcmp(argv[i], "--resources") == 0 && i + 1 < argc) resources = argv[++i];
        else if (strcmp(argv[i], "--prefix") == 0 && i + 1 < argc) prefix = argv[++i];
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (!qfile && argv[i][0] != '-') qfile = argv[i];
    }

    if (!qfile || !label || !rel_path) {
        fprintf(stderr, "ERROR: add-spec requires <queue-file> --label <str> --path <rel-path>\n");
        return 1;
    }

    int fd = open(qfile, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open '%s': %s\n", qfile, strerror(errno));
        return 1;
    }
    flock(fd, LOCK_EX);

    char *txt = read_entire_fd(fd, NULL);
    QueueDoc *doc = calloc(1, sizeof(QueueDoc));
    if (!txt || !doc || !parse_queue_text(txt, doc, false)) {
        fprintf(stderr, "ERROR: '%s' is not a valid cexp_queue file.\n", qfile);
        free(txt);
        free(doc);
        flock(fd, LOCK_UN);
        close(fd);
        return 1;
    }
    free(txt);

    if (doc->n_rows >= MAX_ROWS) {
        fprintf(stderr, "ERROR: Maximum rows reached.\n");
        free(doc);
        flock(fd, LOCK_UN);
        close(fd);
        return 1;
    }

    if (doc->n_rounds == 0) {
        RoundEntry *re = &doc->rounds[doc->n_rounds++];
        snprintf(re->id, sizeof(re->id), "R1");
        snprintf(re->mode, sizeof(re->mode), "standard");
        today_iso(re->created, sizeof(re->created));
        snprintf(re->summary, sizeof(re->summary), "Initial batch");
    }

    char chosen_round[64];
    if (round_id) sanitize_cell(chosen_round, sizeof(chosen_round), round_id);
    else snprintf(chosen_round, sizeof(chosen_round), "%s", doc->rounds[doc->n_rounds - 1].id);

    char chosen_id[64];
    if (explicit_id) sanitize_cell(chosen_id, sizeof(chosen_id), explicit_id);
    else compute_next_spec_id(doc, prefix, chosen_id, sizeof(chosen_id));

    for (int i = 0; i < doc->n_rows; i++) {
        if (strcasecmp(doc->rows[i].kind, "spec") == 0 &&
            strcmp(doc->rows[i].id, chosen_id) == 0) {
            fprintf(stderr, "ERROR: Spec ID '%s' already exists. Use a new ID.\n", chosen_id);
            free(doc);
            flock(fd, LOCK_UN);
            close(fd);
            return 1;
        }
    }

    char clean_label[512], clean_path[512];
    sanitize_cell(clean_label, sizeof(clean_label), label);
    sanitize_cell(clean_path, sizeof(clean_path), rel_path);

    PointerRow *pr = &doc->rows[doc->n_rows++];
    snprintf(pr->round, sizeof(pr->round), "%s", chosen_round);
    snprintf(pr->id, sizeof(pr->id), "%s", chosen_id);
    snprintf(pr->kind, sizeof(pr->kind), "spec");
    sanitize_cell(pr->depends, sizeof(pr->depends), depends);
    sanitize_cell(pr->resources, sizeof(pr->resources), resources);
    snprintf(pr->status, sizeof(pr->status), "-");
    snprintf(pr->pointer, sizeof(pr->pointer), "[%s](%s)", clean_label, clean_path);

    int rc = write_serialized_doc(fd, doc);
    flock(fd, LOCK_UN);
    close(fd);
    free(doc);

    if (rc != 0) return 1;
    printf("Added spec [%s] %s -> %s (depends=%s, resources=%s)\n",
           chosen_round, chosen_id, clean_path, depends, resources);
    return 0;
}

static int cmd_status(int argc, char **argv) {
    const char *qfile = NULL;
    const char *filter_round = NULL;
    bool as_json = false;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--round") == 0 && i + 1 < argc) filter_round = argv[++i];
        else if (strcmp(argv[i], "--json") == 0) as_json = true;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (!qfile && argv[i][0] != '-') qfile = argv[i];
    }

    if (!qfile) {
        fprintf(stderr, "ERROR: status requires <queue-file>\n");
        return 1;
    }

    QueueDoc *doc = calloc(1, sizeof(QueueDoc));
    if (!doc) return 1;
    if (!load_queue_file(qfile, doc, false)) {
        fprintf(stderr, "ERROR: '%s' is missing or not a valid cexp_queue file.\n", qfile);
        free(doc);
        return 1;
    }

    char next_rid[64], next_sid[64];
    compute_next_round_id(doc, next_rid, sizeof(next_rid));
    compute_next_spec_id(doc, "E", next_sid, sizeof(next_sid));

    if (as_json) {
        printf("{\n  \"title\": ");
        json_escape_print(stdout, doc->hdr.title);
        printf(",\n  \"focus\": ");
        json_escape_print(stdout, doc->hdr.focus);
        printf(",\n  \"next_round_id\": ");
        json_escape_print(stdout, next_rid);
        printf(",\n  \"next_spec_id\": ");
        json_escape_print(stdout, next_sid);
        printf(",\n  \"rounds\": [\n");
        bool first_r = true;
        for (int r = 0; r < doc->n_rounds; r++) {
            const RoundEntry *re = &doc->rounds[r];
            if (filter_round && strcmp(re->id, filter_round) != 0) continue;
            if (!first_r) printf(",\n");
            first_r = false;
            printf("    {\"id\": ");
            json_escape_print(stdout, re->id);
            printf(", \"mode\": ");
            json_escape_print(stdout, re->mode);
            printf(", \"created\": ");
            json_escape_print(stdout, re->created);
            printf(", \"summary\": ");
            json_escape_print(stdout, re->summary);
            printf(", \"items\": [");
            bool first_i = true;
            for (int i = 0; i < doc->n_rows; i++) {
                const PointerRow *sp = &doc->rows[i];
                if (strcasecmp(sp->kind, "spec") != 0 || strcmp(sp->round, re->id) != 0) continue;
                const PointerRow *rs = find_result_row(doc, sp->id);
                const char *state = rs ? rs->status : (deps_satisfied(doc, sp->depends) ? "ready" : "waiting");
                if (!first_i) printf(", ");
                first_i = false;
                printf("{\"id\": ");
                json_escape_print(stdout, sp->id);
                printf(", \"state\": ");
                json_escape_print(stdout, state);
                printf(", \"depends\": ");
                json_escape_print(stdout, sp->depends);
                printf(", \"resources\": ");
                json_escape_print(stdout, sp->resources);
                printf(", \"spec_pointer\": ");
                json_escape_print(stdout, sp->pointer);
                printf(", \"result_pointer\": ");
                json_escape_print(stdout, rs ? rs->pointer : "");
                printf("}");
            }
            printf("]}");
        }
        printf("\n  ]\n}\n");
    } else {
        printf("Title:      %s\n", doc->hdr.title);
        printf("Focus:      %s\n", doc->hdr.focus);
        printf("Next IDs:   round=%s, spec=%s\n", next_rid, next_sid);
        for (int r = 0; r < doc->n_rounds; r++) {
            const RoundEntry *re = &doc->rounds[r];
            if (filter_round && strcmp(re->id, filter_round) != 0) continue;
            printf("\n=== Round %s [%s | %s] — %s ===\n",
                   re->id, re->mode, re->created, re->summary);
            int shown = 0;
            for (int i = 0; i < doc->n_rows; i++) {
                const PointerRow *sp = &doc->rows[i];
                if (strcasecmp(sp->kind, "spec") != 0 || strcmp(sp->round, re->id) != 0) continue;
                shown++;
                const PointerRow *rs = find_result_row(doc, sp->id);
                const char *state = rs ? rs->status : (deps_satisfied(doc, sp->depends) ? "PENDING (READY)" : "PENDING (WAITING)");
                printf("  - %-6s [%-17s] res=%-16s dep=%-8s spec=%s",
                       sp->id, state, sp->resources, sp->depends, sp->pointer);
                if (rs) printf(" -> result=%s", rs->pointer);
                printf("\n");
            }
            if (shown == 0) printf("  (no specs registered in this round yet)\n");
        }
    }
    free(doc);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "help") == 0) {
        print_usage();
        return 0;
    }
    const char *cmd = argv[1];
    if (strcmp(cmd, "discover") == 0) return cmd_discover(argc, argv);
    if (strcmp(cmd, "head") == 0) return cmd_head(argc, argv);
    if (strcmp(cmd, "init") == 0) return cmd_init(argc, argv);
    if (strcmp(cmd, "new-round") == 0) return cmd_new_round(argc, argv);
    if (strcmp(cmd, "add-spec") == 0) return cmd_add_spec(argc, argv);
    if (strcmp(cmd, "status") == 0) return cmd_status(argc, argv);

    fprintf(stderr, "ERROR: Unknown command '%s'. Run 'cexpctl --help' for usage.\n", cmd);
    return 1;
}
