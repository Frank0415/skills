# CSlides 使用指南

这套工具包含三个 skill：

| Skill | 用途 | 输入 |
|---|---|---|
| `cslides` | 把一份课程 PDF 做成单文件中文 HTML 讲义 | 一个 PDF |
| `cslides-orchestrator` | 调度多份 PDF，逐份转换、检查、修复 | PDF 目录、压缩包或多个 PDF |
| `cslides-tutor` | 回答学生追问，并把答案写回已有 HTML | 一个已有 HTML，可附源 PDF |

## 仓库

- 网页：<https://github.com/Frank0415/skills>
- HTTPS clone：

```bash
git clone https://github.com/Frank0415/skills.git "$HOME/skills"
```

- 已配置 GitHub SSH 时可用：

```bash
git clone git@github.com:Frank0415/skills.git "$HOME/skills"
```

## 安装到 Codex

推荐使用软链接。仓库更新后，运行时自动看到新版本。

```bash
mkdir -p "$HOME/.agents/skills"

ln -s "$HOME/skills/cslides" "$HOME/.agents/skills/cslides"
ln -s "$HOME/skills/cslides-orchestrator" "$HOME/.agents/skills/cslides-orchestrator"
ln -s "$HOME/skills/cslides-tutor" "$HOME/.agents/skills/cslides-tutor"
```

不要对已有同名目录直接运行 `ln -s`。先确认它是旧副本还是软链接，再决定替换。

部分 Codex 安装扫描 `${CODEX_HOME:-$HOME/.codex}/skills`。如果当前客户端使用该目录，把上面命令中的 `$HOME/.agents/skills` 换成对应路径。

不想使用软链接时，可复制：

```bash
mkdir -p "$HOME/.agents/skills"
cp -R "$HOME/skills/cslides" "$HOME/.agents/skills/"
cp -R "$HOME/skills/cslides-orchestrator" "$HOME/.agents/skills/"
cp -R "$HOME/skills/cslides-tutor" "$HOME/.agents/skills/"
```

复制模式不会自动更新。每次 `git pull` 后需重新复制。

安装完成后，新开一个 Codex task，使 skill metadata 重新加载。

## 依赖

- 三个 skill 都需要支持 `SKILL.md` 的 Codex 环境。
- `cslides-orchestrator` 额外需要可执行的 `codex` CLI：

```bash
command -v codex
```

- `cslides-tutor` 的审计脚本需要 Node.js：

```bash
command -v node
```

- `humanizer-zh` 是可选增强。已安装时 `cslides` 才调用；未安装时跳过，不报错或阻塞。核心写作规则已包含在 `cslides` 内。
- PDF 文本提取、页面渲染和浏览器验证能力越完整，`cslides` 输出越可靠。具体工具由当前 agent 环境选择。

## 选择哪个 skill

### 一份 PDF：`cslides`

```text
使用 $cslides。

把 /absolute/path/lecture-04.pdf 转成单文件中文 HTML 学习讲义。
HTML 放在 PDF 同目录，文件名使用 lecture-04.html。
```

`cslides` 每次只处理一份 PDF。它会逐页取证，把连续页面按教学任务分组，保留原 slide 图像，并补全公式、图表、例子、导航和 MathJax。

### 多份 PDF：`cslides-orchestrator`

```text
使用 $cslides-orchestrator。

处理 /absolute/path/course-slides/ 中的全部 PDF。
并发 worker 数量为 2，使用 Codex CLI 默认模型。
```

orchestrator 不直接生成讲义。它给每份 PDF 启动独立 `cslides` worker，并分开执行：

1. convert；
2. judge；
3. 必要时 fix；
4. 再次 judge。

每个 worker 只处理一份 PDF。没有 `codex` CLI 时，回退为当前 agent 串行处理。

### 已有 HTML 上继续追问：`cslides-tutor`

```text
使用 $cslides-tutor。

目标 HTML：/absolute/path/lecture-04.html
源 PDF：/absolute/path/lecture-04.pdf

问题：P020 的 Householder reflection 为什么能把向量下面的元素消成 0？
请详细推导并把问答写回对应讲解单元。
```

`cslides-tutor` 会：

- 定位相关 slide 与 HTML 单元；
- 先核对原页，再回答；
- 把问题和答案插入对应知识点附近；
- 复用原页面的 CSS 与 MathJax 结构；
- 保持学生追问块默认关闭；
- 检查重复 ID、公式结构和裸 MathJax 定界符。

已有追问需要补充或纠正时，直接说明原 block 标题或问题。tutor 会修改原 block，避免留下互相矛盾的答案。

## 推荐流程

1. 用 `cslides` 生成单份讲义。
2. 学习时用 `cslides-tutor` 持续补充追问。
3. 只有同时处理多份独立 PDF 时才用 `cslides-orchestrator`。

## 验证安装

```bash
for skill in cslides cslides-orchestrator cslides-tutor; do
  test -f "$HOME/.agents/skills/$skill/SKILL.md" && echo "OK $skill"
done
```

检查 tutor 审计脚本语法：

```bash
node --check "$HOME/.agents/skills/cslides-tutor/scripts/audit-qna-html.mjs"
```

审计一份已生成讲义：

```bash
node "$HOME/.agents/skills/cslides-tutor/scripts/audit-qna-html.mjs" \
  /absolute/path/lecture-04.html
```

## 更新

```bash
cd "$HOME/skills"
git pull --ff-only
```

软链接安装无需其他操作。复制安装需要重新复制三个目录，然后新开 Codex task。

## 使用注意

- 给出绝对路径，减少 PDF/HTML 目标歧义。
- 把课程文件放进 Git 或先做备份；tutor 会直接修改 HTML。
- `cslides` 负责首次生成，`cslides-tutor` 负责后续追问，不要混用职责。
- MathJax 使用 CDN 时需要网络。离线使用前应把依赖改成本地资源。
- 浏览器若禁止刷新本地 `file://` 页面，agent 应报告限制并继续静态检查，不应伪造视觉验证结果。
