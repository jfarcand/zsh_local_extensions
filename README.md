# zsh_local_extensions

Personal command-line tools, one executable script per command.

## Install

The repo is symlinked as `~/.zsh_local_extensions` and put on `PATH` by `~/.zshrc`:

```sh
ln -s ~/workspace/zsh_local_extensions ~/.zsh_local_extensions
# ~/.zshrc
export ZSH_EXTENSIONS=$HOME/.zsh_local_extensions
export PATH=$PATH:$ZSH_EXTENSIONS
```

Most scripts print their usage with `-h`/`--help` or when called without the
arguments they need; the header comment of each script is its documentation.

## Claude Code across two Macs

Claude Code sessions run on **2Q84** inside tmux; they are driven from **1Q84**
over SSH, so a dropped connection never loses a session.

| Command | What it does |
|---|---|
| `ccc` | Reconnect this iTerm pane to the Claude conversation it showed (found by id, then name; resumed with `claude --resume` if no Claude runs it any more), otherwise show the session menu: account, bilan, state, whether a pane here shows it, title. Reconnects on its own when the connection drops. |
| `ccc list` / `ccc new` / `ccc <n>` | The menu / a fresh session / attach session `n`. Add `oss` or `gatling` to bind a new session to that Claude account. |
| `ccc recover` | Open an iTerm tab for every Claude session on 2Q84 that no pane here shows. |
| `tunnel [port…]` | Forward localhost ports to 2Q84 until Ctrl-C. Bare `tunnel` finds the callback port of a pending `/login` on 2Q84. |
| `ssh-mbp-fwd` | The connection behind the `local.ssh-mbp-fwd` LaunchAgent: 2Q84's dev ports (18081/15173/18082), gcloud's 8085, and the URL-back channel. |
| `clip2q84` | Runs under the `local.clip2q84` LaunchAgent: copies every image put on 1Q84's clipboard to 2Q84's, so pasting in Claude there works. |
| `cclimits` | Usage limits of every logged-in Claude account, side by side, with alerts; shows the org (`· Gatling`) and any extra-usage spend. `cclimits -w 120` watches. |
| `cclimits -g` | Guard: blocks an account that is at 100% of a limit while its org's extra usage would bill every further request. Alone it runs one silent pass (the `local.cclimits-guard` LaunchAgent, every 5 min); with `-w` it guards on each refresh. |
| `cclimits-guard` | The Claude Code hook (UserPromptSubmit + PreToolUse) that refuses prompts and tool calls for a blocked account. |

Claude accounts are config dirs selected by aliases in `~/.zshrc`: `claude`
(`~/.claude`), `claude-oss` (`~/.claude-perso`), `claude-gatling`
(`~/.claude-gatling`). They share everything through symlinks except the login.

Pieces that live outside this repo:

- **2Q84** `~/.local/bin`: `ccc-tmux` (the session menu and reconnect logic),
  `ccc-tag` (tags each iTerm pane with its conversation), `clip-set-png`,
  `cclimits-guard` (copy of the hook); `~/.tmux.conf`; the SSH block in `~/.zshrc`.
- **Both Macs** `~/.claude/settings.json`: the `ccc-tag` and `cclimits-guard` hooks.
- **1Q84** `~/Library/LaunchAgents`: `local.url-listener`, `local.ssh-mbp-fwd`,
  `local.clip2q84`, `local.cclimits-guard`; **2Q84**: `local.caffeinate`.
- **1Q84** keychain item `2Q84-login`, which `ccc` uses to unlock 2Q84's keychain.

## GitHub

| Command | What it does |
|---|---|
| `ghactions` | Watch GitHub Actions runs for one or more repos (`-w` to refresh). |
| `ghrate` | API rate-limit buckets and their reset times; alerts when low; `--callers` lists local processes talking to GitHub. |
| `ghstats` | Stars and forks per repo, with daily deltas. |
| `killActions` | Cancel in-progress and queued workflow runs on a branch, or keep only the latest per workflow. |
| `branch-cleanup` | Delete local branches whose remote branch is gone. |
| `cr` | Create an issue, commit with `Fixes #n`, and push. |
| `recover_git_history` | Find deleted files or folders in git history and restore them. |

## dravr

| Command | What it does |
|---|---|
| `dravr-deps` | Who depends on each `dravr-*` crate on crates.io. |
| `dravr-sciotte-cleanup` | Yank every dravr-sciotte version, archive its Homebrew tap, delete its GHCR images. |

## Notes and vaults

| Command | What it does |
|---|---|
| `vault-sync` | Commit and push Obsidian vaults on a timer, independent of Obsidian; notifies on conflicts. |

## Kubernetes (namespace `production` unless `-n`)

| Command | What it does |
|---|---|
| `kl <key>` | Follow the logs of the pods matching `key`, one iTerm pane each, through `jq`. |
| `kl-all [namespace]` | Open one iTerm pane per pod, each following its logs through `jq`. |
| `kd <key>` | `kubectl describe` the matching pods. |
| `ki <key>` | The image of the matching pods. |
| `ks <secret>` | Decode and print a secret. |
| `kdp <keyword>` | Delete the jobs whose name matches. |

## Cloud and local dev

| Command | What it does |
|---|---|
| `cloudrunLog JOB [REGION]` | Cloud Run job logs (`--all`, `--limit=N`). |
| `killPort <port>` | Kill whatever listens on a port. |
| `tailDockerProcess [service]` | Follow a docker-compose service's logs, reattaching when it restarts. |
| `backup_postgres` | `pg_dump` the Postgres container named in `.envrc`. |
| `ys` | `yarn start:debug` with JSON log lines pretty-printed. |
| `cop [--opus\|--sonnet\|--gemini\|--gpt]` | GitHub Copilot CLI with a model picked by name. |

## Feeding files to an LLM

| Command | What it does |
|---|---|
| `catFiles` | Print files matching base names, with an optional schema lookup. |
| `catAllFiles` / `catAllFilesPlus` | `catFiles` over every base name in directories (`Plus` adds a common file). |
| `batchCatFiles <dir>` | Run it per directory, writing to `~/Downloads`. |
| `catModel <A,B>` | Print Prisma models from `prisma/schema.prisma`. |
| `xfind` | `find` that skips hidden dirs, `node_modules`, `ios` and `android`. |
