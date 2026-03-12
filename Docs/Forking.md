# SmartFuseBox – Forking and Merge Strategy

This guide explains how to fork SmartFuseBox, make device-specific changes to `Local.h` and
`SmartFuseBox.ino`, and safely pull future updates from the upstream repository without
overwriting your configuration.

---

## Overview

### Consumer-owned files (protected from upstream updates)

| File | Purpose |
|---|---|
| `SmartFuseBox/SmartFuseBox/Local.h` | Board selection, pins, feature flags, relay config |
| `SmartFuseBox/SmartFuseBox/SmartFuseBox.ino` | Sensor wiring, UART selection, baud rates |

These files are protected by `merge=ours` in `.gitattributes` and will never be overwritten
when pulling upstream changes.

### Framework-owned files (updated freely by upstream)

| File | Purpose |
|---|---|
| `SmartFuseBox/SmartFuseBox/BoardConfig.h` | EEPROM table, derived flags, board validation guards |

`BoardConfig.h` is included at the bottom of `Local.h` and provides all derived capability
flags (`WIFI_SUPPORT`, `SCHEDULER_SUPPORT`, etc.) and `#error` guards based on your board
selection. You never need to edit or include it directly — it is an implementation detail of
`Local.h`. This split means upstream can add new board support without ever touching `Local.h`.

---

## 1. Initial Fork Setup

**Step 1** — Fork the repository on GitHub using the Fork button, then clone your fork locally:

````````

**Step 2** — Add the original repository as a second remote named `upstream`:

````````

**Step 3** — Create a branch for your device configuration:

````````

**Step 4** — Configure the merge driver once per machine. This activates the `merge=ours`
protection defined in `.gitattributes`:

````````

> **Note:** This command must be run once on every machine that clones your fork.
> It does not need to be repeated after that.

**Step 5** — Edit `Local.h` and `SmartFuseBox.ino` for your hardware, then commit:

````````

---

## 2. Pulling Upstream Updates

When the upstream repository has new features or fixes you want to bring in:

````````
git checkout main              # or your default branch name
git fetch upstream              # fetch latest changes from original repo
git merge upstream/main --ff-only # fast-forward your branch
git checkout your-device-branch  # switch back to your device branch
git rebase main                 # apply your changes on top of the updated main
````````

The `.gitattributes` merge driver will automatically keep **your** versions of `Local.h` and
`SmartFuseBox.ino` — no manual conflict resolution needed for those files.

All other files (library code, handlers, managers) will update normally.

---

## 3. How the Protection Works

The `.gitattributes` file at the repository root contains:

````````
SmartFuseBox/SmartFuseBox/Local.h merge=ours
SmartFuseBox/SmartFuseBox/SmartFuseBox.ino merge=ours
````````

The `merge=ours` driver tells git to always resolve conflicts in those files by keeping the
local (your) version. `BoardConfig.h` is deliberately absent from this list so that upstream
board and capability updates flow through to your fork unimpeded.

> **Important:** `merge=ours` silently keeps your file. If upstream makes a structural change
> to `SmartFuseBox.ino` that you need (for example, a new late-binding sensor call), you will
> not receive it automatically. To review upstream changes to a protected file without being
> affected by them:

````````
git diff upstream/main -- SmartFuseBox/SmartFuseBox/SmartFuseBox.ino
git diff upstream/main -- SmartFuseBox/SmartFuseBox/Local.h
````````

Apply any relevant changes manually, then commit.

---

## 4. Recommended Branch Layout

````````

Keeping `main` clean means you can always do a fast comparison:

````````

to see exactly what upstream has changed before merging into your device branch.

---

## Quick Reference

| Task | Command |
|---|---|
| Add upstream remote | `git remote add upstream <url>` |
| Enable merge driver (once per machine) | `git config merge.ours.driver true` |
| Fetch upstream changes | `git fetch upstream` |
| Merge upstream into your branch | `git merge upstream/main` |
| Inspect upstream changes to a protected file | `git diff upstream/main -- <file>` |
| Temporarily disable file protection | `git update-index --no-skip-worktree <file>` |
| Inspect upstream changes to BoardConfig.h | `git diff upstream/main -- SmartFuseBox/SmartFuseBox/BoardConfig.h` |
