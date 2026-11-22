# Aurora-GPUParity

GPU research for the 14" MacBook Pro M2 Pro (T6020, `apple,j414s`, G14X) running the
[linux-aurora](https://github.com/omacom/linux-aurora) / [aurora-silicon](https://github.com/aurora-silicon)
stack. The goal: AI inference, games and graphics-heavy tools use the whole GPU on Linux, as fast as
on macOS or faster, without timeouts, desktop corruption or lost clients.

There is no Metal on Linux, and this repo does not try to add one first. "Metal-equivalent" here means
the Vulkan driver (Borealis, a fork of Honeykrisp) and the kernel's GPU scheduling reach the performance
and robustness that Metal reaches on the same chip. Metal is the reference we measure against.

Start with [docs/status.md](docs/status.md). The research so far is in [docs/prior-art.md](docs/prior-art.md)
(what exists, and what it means for t6020) and the plan is [docs/plan.md](docs/plan.md). The running log
of what was run on the machine is [docs/worklog.md](docs/worklog.md).

## First target

[aurora-silicon/linux#2701](https://github.com/aurora-silicon/linux/issues/2701): long Vulkan compute
(ACE-Step, FLUX.2 via ggml-vulkan) trips repeated GPU timeouts and corrupts the desktop on t6020.
Working hypothesis: the kernel runs t6020 compute without firmware preemption (see the plan).

## Layout

| Path | Contents |
|---|---|
| `docs/` | status, prior art, plan, worklog, issue drafts |
| `harness/` | scripts that run on the machine: reproducers, timeout counters, A/B runners |
| `tools/` | analysis helpers (log parsing, `gpubench` record comparison) |
| `data/` | raw measurements behind every number, one folder per run |
| `patches/` | kernel and Mesa changes, as `git format-patch` output against a named base |

## Rules for this repo

* Every number in `docs/` points at a folder in `data/`. No data, no claim.
* Patches state their base commit (linux-aurora or mesa-aurora) and the kernel they were tested on.
* A boot with a non-default GPU setting is logged in the worklog before it happens, with the way back.
* Every step is logged in [docs/worklog.md](docs/worklog.md), in detail, as it happens, and committed straight
  to `main` in small commits. No PRs and no tracking issues; status lives in [docs/status.md](docs/status.md).
* Attribution follows Aurora's [contributing guide](https://aurorasilicon.org/docs/contributing/) and
  [agent-guidelines](https://github.com/aurora-silicon/agent-guidelines): commits carry the human's identity
  only, with no AI trailers, and a PR description into an Aurora repository ends with an `Assisted-by:` line.
