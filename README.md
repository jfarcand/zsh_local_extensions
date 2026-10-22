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

Claude Code sessions run on a **remote Mac** inside tmux; they are driven from
**this Mac** over SSH, so a dropped connection never loses a session. The remote
Mac is the SSH host `ccc-remote` (define it in `~/.ssh/config`, or set
`CCC_HOST`), with a second host `ccc-remote-fwd` carrying the port forwards.

| Command | What it does |
|---|---|
| `ccc` | Reconnect this iTerm pane to the Claude conversation it showed (found by id, then name; resumed with `claude --resume` if no Claude runs it any more). A Claude counts as running it even before it writes its session file, such as one stuck on the folder-trust question, and a conversation running in Claude Code's background host is shown with `claude attach` from a tmux session of its own instead of being resumed a second time, otherwise show the session menu: account, bilan, state, whether a pane here shows it, title. Reconnects on its own when the connection drops; every connection's end and ccc's own exit (with any signal) are logged to `~/Library/Logs/ccc.log`. On exit it prints the conversation the pane showed, with the command to resume it on another account. |
| `ccc list` / `ccc new` / `ccc <n>` | The menu / a fresh session / attach session `n`. The menu's prompt also takes a saved conversation's id (or 6+ character prefix) to resume it, and `ccc <n>` takes an id made only of digits when no session has that number. Below the tmux sessions, the menu lists Claude Code's background sessions (outside tmux) by id; picking one shows it in a tmux session of its own. Add an account name to bind a new or resumed session to it: `oss` is `~/.claude-perso`, any other name `x` is `~/.claude-x`. |
| `ccc switch <account>` | Resume this pane's conversation on another account, in a fresh session (exit Claude first). |
| `ccc [resume] <id> [<account>]` | Reattach, or resume, a saved conversation by id or id prefix (6+ characters), e.g. one no pane shows any more. With an account, it resumes there; if a Claude on another account still runs it, exit that one first. |
| `ccc @<alias> …` | Any `ccc` command on another remote Mac (an ssh alias set up like `ccc-remote`), e.g. `ccc @douche new`. `ccc-remote` stays the default; a bare `ccc` in a pane that last showed another Mac says how to go back there. Each Mac's password is its own keychain item: `ccc-remote-login` for `ccc-remote`, `ccc-<host>-login` for the others. |
| `ccc ssh` | A plain shell on the remote Mac: no tmux, no Claude, no reconnect. |
| `ccc [ssh] <command…>` | Run a command on the remote Mac instead, e.g. `ccc mole status` or `ccc @douche df -h`, and exit with its status. The words are joined and parsed by the remote shell, as with ssh, so `ccc 'ls ~ \| wc -l'` pipes there. On a terminal it runs in an interactive login shell, as `ccc ssh` gives; into a pipe or a file, in a login shell without the remote `.zshrc`, so the output is clean. `ssh` is needed only when the command's first word is one of ccc's own words or an account name. |
| `ccc clean` | List the finished remote sessions (a Claude idle at bilan 10 for over 2 hours, a shell unused for a day) and close them after one confirmation. Each Claude exits through `/exit`, so its end-of-session hooks run, and its conversation stays resumable by id. Sessions a pane shows, or that are working or waiting, are never touched; ones without a bilan are listed for a decision by hand. |
| `ccc recover` | Open an iTerm window for every remote Claude session that no pane here shows. |
| `tunnel [@<alias>] [port…]` | Forward localhost ports to the remote Mac (or `@<alias>`) until Ctrl-C. Bare `tunnel` finds the callback port of a pending `/login` there. |
| `ccc-forward` | The connection behind the `local.ccc-forward` LaunchAgent: the remote dev ports, the gcloud login callback, and the URL-back channel. |
| `clip-to-remote` | Runs under the `local.clip-to-remote` LaunchAgent: copies every image and text put on this Mac's clipboard to the remote Mac's, so pasting in Claude there works; text a password manager marks concealed stays here. |
| `clip-from-remote` | The other direction, under the `local.clip-from-remote` LaunchAgent: keeps one ssh connection running the remote `clip-watch` and puts every image and text copied on the remote Mac on this Mac's clipboard. Both directions mark what they set (`local.clip-sync`), so a copy is never sent back to the Mac it came from. |
| `cclimits` | Usage limits of every logged-in Claude account, side by side, with alerts; shows a Team account's organization and any extra-usage spend. `cclimits -w 120` watches. |
| `cclimits -g` | Guard: blocks an account that is at 100% of a limit while its org's extra usage would bill every further request. Alone it runs one silent pass (the `local.cclimits-guard` LaunchAgent, every 5 min); with `-w` it guards on each refresh. `CCLIMITS_GUARD_HOSTS` lists the ssh hosts that get the block list. |
| `cclimits-guard` | The Claude Code hook (UserPromptSubmit + PreToolUse) that refuses prompts and tool calls for a blocked account. A session started with `ANTHROPIC_API_KEY` bills the key's Console organization instead, so it is let through. A block names the login it was set for (email and organization), so a Mac whose config dir of that name holds another login is not blocked. |
| `rtk-claude-hook` | Runs [rtk](https://github.com/rtk-ai/rtk)'s Claude Code hook (condensed command output, fewer tokens) only for the accounts listed in `~/.config/rtk/claude-accounts`; the other accounts share the same `settings.json` untouched. `rtk gain` shows the savings. |

Claude accounts are config dirs, each selected by an alias that sets
`CLAUDE_CONFIG_DIR` (`~/.claude` is the default account). They share everything
through symlinks except the login.

`remote/` holds what runs on the remote Mac, installed into its `~/.local/bin`
(`scp remote/* ccc-remote:.local/bin/`): `ccc-tmux` (the session menu and
reconnect logic), `ccc-tag` (tags each iTerm pane with its conversation),
`clip-set` (puts a synced text or image on the clipboard; this Mac's
`clip-from-remote` uses it too) and `clip-watch` (prints the remote Mac's copies
for `clip-from-remote`).

Pieces that live outside this repo:

- **Remote Mac** `~/.local/bin`: `cclimits-guard` (copy of the hook);
  `~/.tmux.conf`; the SSH block in `~/.zshrc`; a `caffeinate` LaunchAgent.
- **Both Macs** `~/.claude/settings.json`: the `ccc-tag`, `cclimits-guard` and `rtk-claude-hook` hooks; `~/.config/rtk/claude-accounts`; an rtk account's own `CLAUDE.md` importing the shared one plus `@RTK.md`.
- **This Mac** `~/Library/LaunchAgents`: `local.url-listener`,
  `local.ccc-forward`, `local.clip-to-remote`, `local.clip-from-remote`, `local.cclimits-guard`,
  `local.pr-notify-<name>`; `pr-notify`'s notifier app in `~/Applications` and
  its sound in `~/Library/Sounds`.
- **This Mac** keychain item `ccc-remote-login`, and `ccc-<host>-login` per other
  remote Mac (or `$CCC_KEYCHAIN_ITEM`), which `ccc` uses to unlock the remote login keychain.
- **Another remote Mac** gets the same remote pieces, its own `<alias>-fwd` ssh host and
  `local.ccc-forward-<alias>` LaunchAgent for the URL-back channel, and a place in
  `CCLIMITS_GUARD_HOSTS`.

## GitHub

| Command | What it does |
|---|---|
| `ghactions` | Watch GitHub Actions runs for one or more repos (`-w` to refresh). |
| `ghrate` | API rate-limit buckets and their reset times; alerts when low; `--callers` lists local processes talking to GitHub. |
| `ghstats` | Stars and forks per repo, with daily deltas. |
| `pr-notify <owner/repo>` | A macOS notification for each pull request opened since the last run; clicking it opens the PR. Runs under a `local.pr-notify-<name>` LaunchAgent every minute; an unchanged repo answers 304 to its ETag, which costs no rate limit. `PR_NOTIFY_APP` points at a copy of `terminal-notifier.app` with its own bundle id, so only these notifications can be set to stay on screen; `PR_NOTIFY_SOUND` names a sound in `~/Library/Sounds`. |
| `killActions` | Cancel in-progress and queued workflow runs on a branch, or keep only the latest per workflow. |
| `branch-cleanup` | Delete local branches whose remote branch is gone. |
| `cr` | Create an issue, commit with `Fixes #n`, and push. |
| `recover_git_history` | Find deleted files or folders in git history and restore them. |

## dravr

| Command | What it does |
|---|---|
| `dravr-deps` | Who depends on each `dravr-*` crate on crates.io. |
| `dravr-sciotte-cleanup` | Yank every dravr-sciotte version, archive its Homebrew tap, delete its GHCR images. |

## iTerm

| Command | What it does |
|---|---|
| `hue` | Give this terminal the next colour nothing else is using, and remember it. Twelve tabs come out twelve different colours, and re-running it in the same pane returns the colour it already has. |
| `hue red` / `hue '#ff8800'` | A name (`hue -l` lists them), or hex as `#ff8800`, `ff8800` or `f80`. |
| `hue repo` | Colour from the git repo name instead, so every tab of one project matches on purpose. |
| `hue auto <seed>` | Colour from any string: a host, an env, a ticket. |
| `hue off` | Back to the profile default, tab and background, and the colour returns to the pool. |

The tab gets the colour at full strength and the window background gets it
darkened, so a whole pane is identifiable at a glance without hurting contrast.
`HUE_TINT` (default 64 of 255) sets how bright that background is; `hue -t`, or
`HUE_TINT=0`, colours the tab only.

Sessions are keyed by `ITERM_SESSION_ID` (the UUID, which survives moving a tab),
or by the tmux pane in a `ccc` pane on the remote Mac, and kept in
`~/.local/state/hue/sessions` for a week after they are last seen. Because a
second run is a no-op, `hue >/dev/null` is safe at the end of `.zshrc` — every
new tab then colours itself.

Run it at a shell prompt — through Claude Code's `!` prefix there is no terminal
to colour, and `hue` says so instead of printing escapes at you (`-p` prints them
on purpose).

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
