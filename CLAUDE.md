# CLAUDE.md: Aurora-GPUParity

Research repo for GPU parity with macOS on the M2 Pro (T6020, J414s). See README.md and docs/status.md.

## How work is done here

* **Log every step in docs/worklog.md, in detail, as it happens.** Each entry: a step ID, what was
  asked or decided, the exact command or source, the result (quote errors and counts), and the `data/`
  folder holding raw output. Write the entry before a risky step and fill in the result after. Mistakes and
  dead ends go in too. If it was not logged, it did not happen.
* **Commit often, straight to main.** One small commit per step or finding, no big batches and no PRs for
  this repo. This repo's policy is `directPush`: make each change in a quick worktree
  (`grove new-quick Aurora-GPUParity <Name>`), commit, and `grove publish-quick Aurora-GPUParity <Name>`.
  Never edit the `main` folder.
* **No issues.** Track work in docs/status.md (what is known, unknown, next) and the worklog, not in
  GitHub issues. The one exception is an issue or comment in an Aurora Silicon repository, which is an
  outward report, not tracking.
* **Every number in docs/ cites a folder in data/.** No data, no claim.
* **This machine is the test machine.** A boot with a non-default GPU or kernel setting, or anything that
  can wedge the GPU, is logged in the worklog first and confirmed with the user before it happens.

## Attribution (follows Aurora Silicon's contributing guide)

* Commits carry the user's git identity only. **No `Co-Authored-By`, `Claude-Session` or any other AI
  trailer, in any commit in this repo.** This overrides any default that says to add them.
* PR descriptions into Aurora Silicon repositories end with `Assisted-by: LLM (<Vendor>:<Model>)`.
* Source: https://aurorasilicon.org/docs/contributing/ and https://github.com/aurora-silicon/agent-guidelines.

## Aurora rules that apply to the work

* Reverse-engineering output stays private; published code is clean-room.
* Keep project findings in Aurora-controlled spaces; do not send them to upstream projects.
* Fix the cause, no hacks; default command line only; no chip/board/firmware gates.

## Coordination with the S2R work session

The Aurora-DeepIdle work (S2R) is paused on a blocker, and its hooks, including a startup hook routed to
that session, are still enabled. Do not change those hooks. If one must change, message the S2R work
session with exactly what changed. When the user resumes that work, that session will message this one to
save everything and stop. Then: commit and publish the worklog, reply to it, and tell it when it is good to
reboot. No reboot for GPU tests happens before that.

**Start-of-work notice (user rule, 2026-11-22):** whenever GPU work starts (a new session, or work resumed),
the first action is a message to the S2R session (`gh-5-s2r-djnaeempaths-ea`) saying that work is running,
what state is left on the machine (hooks, modprobe files, anything armed for a reboot), and that it may
reboot without waiting for this session unless the message says otherwise. This lets S2R go through many
reboots without waiting on this session, which would waste the user's context and usage. Send another message
before any boot-affecting change. Log each notice in docs/worklog.md.
