# Handoff prompt for AGY (written 2026-11-24)

Paste everything below the line into a new AGY session started in `~/dev/Aurora-GPUParity/main` (read-only
start; make changes in a quick worktree, see "Workflow").

---

You are taking over a GPU research and fix effort for an Apple MacBook Pro 14" M2 Pro running Aurora Silicon
Linux. Read this whole prompt, then the files it names, before doing anything.

## Goal

The user wants a "Metal-equivalent graphics experience" on Linux: the GPU should be as fast as or faster than
macOS, so AI (llama.cpp, stable-diffusion.cpp, FLUX, ACE-Step), games and graphics tools use the full GPU.
There is no native Metal API on Linux and none is planned; Metal is only the macOS reference measured with
gpubench. In practice that means Vulkan (Mesa `mesa-aurora`, driver "Borealis", derived from Honeykrisp,
installed as `/usr/lib/libvulkan_asahi.so`) plus the kernel GPU driver and scheduling.

**First goal:** fix aurora-silicon/linux#2701, the user's own issue: long Vulkan compute jobs trip GPU
timeouts (`GPU timeout`, `gCDM_CS`/`gCDM_ID` unmapped-read faults) and corrupt the desktop (magenta).
Aurora Silicon is NOT Asahi Linux; use the aurora-silicon org (linux, linux-aurora, mesa, gpubench), not Asahi's.

## The machine (this one is the test machine)

* MacBook Pro 14" M2 Pro, DT `apple,j414s` / `apple,t6020`, GPU generation G14X, GPU name `Apple M2 Pro (G14S B1)`, 32 GB.
* Kernel `7.2.11-16-1-ARCH` (`linux-aurora 7.2.11.aurora16-1`), Mesa `26.3.0-devel (git-b7b03c5b3b)`, Vulkan 1.4.363.
* DRM: `card2` = `406400000.gpu` (driver `asahi`), `card1` = `apple-drm`. Hyprland (Omarchy) desktop.
* The kernel driver is `drivers/gpu/drm/asahi` (Rust), built as a module (`CONFIG_DRM_ASAHI=m`, 46 users while the
  desktop runs, so a live reload kills the session).
* Passwordless `sudo` works. Another machine state matters: macOS 13.5 at `MacOS135` is used by the S2R
  work, so do not touch it. `MacOSTest` (about 14.8) is free for us and boots through the "pi Aura proxy".

## Where everything is

* **Repo (the work):** `JJRPF/Aurora-GPUParity`, now PUBLIC, local base clone `~/dev/Aurora-GPUParity/main`.
  * `README.md`, `CLAUDE.md` (repo rules, read it first), `docs/status.md` (known / unknown / next),
    `docs/plan.md` (phases 1-3), `docs/preempt-test.md` (how the preemption test runs), `docs/prior-art.md`
    (Aurora items and Metal-on-Vulkan projects), `docs/gpu-status.json` (feeds the website), and
    **`docs/worklog.md`**: the detailed step log, entries G1 to G38. Read it fully; it is the memory of this effort.
  * `data/` (every number cites one of these folders): `20261122-baseline-identity/` (machine identity, packages,
    GPU error history of 99 boots), `20261122-gpubench-baseline/` (gpubench core-a, core-b records),
    `20261122-ramp-default-preempt/` (single dispatch ramp), `...-batched/`, `...-long/`.
  * `harness/gpuprobe/`: `gpuprobe.c` + `probe.comp` (Vulkan compute reproducer: `dispatch --ms N --count K
    --inflight 2 --reps R` and `latency --seconds S`), `build.sh`, `run-ramp.sh` (ramp, stops at first GPU timeout).
    Build output goes to `~/.cache/gpuparity/gpuprobe/gpuprobe`.
  * `patches/`, `tools/`: empty READMEs so far.
* **Website:** https://jjrpf.com/GPU (case-insensitive URL; repo `JJRPF/jjrpf.com`, a Cloudflare Worker, page
  `GPU/index.html`, worker redirect in `worker/index.js`). The page reads `docs/gpu-status.json`, `docs/status.md`,
  the worklog (one entry per `**G<n>.**` step) and the commit list from this repo's `main`, so keeping those files
  accurate IS updating the site. Its "Right now" line is a KV key `gpu-message` (`/api/gpu-message`). The sister page
  is jjrpf.com/S2R. PR #5 (the page) is merged.
* **gpubench:** `aurora-silicon/gpubench`, clone `~/dev/gpubench/main` (never edited; built out of tree:
  `cmake -S ~/dev/gpubench/main -B ~/.cache/gpuparity/gpubench-build -G Ninja`, binaries in `.../bin`; run with
  `tools/run-linux.sh --device j414s-m2pro --chunks core-a --bin ~/.cache/gpuparity/gpubench-build/bin --out-root <dir>`;
  macOS side `tools/run-macos.sh`; compare with `tools/performance-gate.py`). No t6020 macOS record exists.
* **Sister project:** `~/dev/Aurora-DeepIdle` (S2R, suspend-to-RAM / idle power). Its boot harness
  (`harness/README.md`: `runner.sh`, `early-restore.sh`, watchdog, GRUB entries `43_deepidle`, initramfs environment)
  is the model for any root-FS-risk boot test. Its hooks are still enabled (see S2R coordination).
* **Aurora upstream issues/PRs to know:** linux#2701 (this bug), linux-aurora#25 (compute preemption: automatic on
  M2 Max, opt-in on Pro/Ultra pending qualification), #335 (kill with two queued submissions faults), #387/#389
  (G15 compositor delay and batch-time cap), #440 (G15C fail-stop), mesa#457 (Cyberpunk tiers).

## What has been established (all logged with data)

1. **Hypothesis for #2701, untested:** `workqueue.rs` (~line 782) sets
   `#[ver(G == G14X)] let preempt_auto = cfg.chip_id == 0x6021;` and `compute_preempt` (i32 module param, default
   -1, "-1: M2 Max or Debug0; 0: off; nonzero: on") resolves `-1 => preempt_auto || debug_enabled(Debug0)`.
   T6020 is 0x6020, so it runs with no compute preemption; the code comment says the firmware then cannot switch
   compute out for another client and on G14X kills the work when the context-switch and kill timeouts expire.
2. **No sysfs file** for the parameter (so `/sys/module/asahi/parameters/` is empty of it), but `modinfo -p asahi`
   lists it. The test is `options asahi compute_preempt=1` in `/etc/modprobe.d/` for one boot, not the kernel
   command line.
3. **History:** only 2 of 99 journal boots had GPU errors. The #2701 boot (2026-11-21 09:03-09:29) has 116
   timeouts and 6 fault dumps; one other boot has 1 timeout right after a global OOM kill.
4. **Baseline on this boot (2026-11-22, default preemption):** gpubench core-a (23 results) and core-b (21 results),
   0 GPU timeouts.
5. **Probe results (G34-G36):** a single long dispatch (50 ms to 12 s) causes 0 timeouts but the second client's
   latency scales with dispatch length (p95 2.9 ms at 50 ms, 32 ms at 3 s, 69 ms at 12 s; up to 35% of samples over
   8.3 ms): that matches "no preemption". Batches of many 20 ms dispatches in one submission (up to 10 s) do not
   starve the other client. **The probe has NOT reproduced the actual timeouts**, so the trigger likely needs
   llama.cpp/FLUX-style load (several queued submissions, memory pressure).
6. Cost evidence from linux-aurora#25 for the later qualification: M2 Max 0 timeouts vs 39-120/min, about 1%
   throughput cost; M1 MoE decode costs 12-20%; `compute_preempt=0` wedged the M2 Max machine.

## What to do next, in order

1. **Reproduce #2701 without a reboot** using a realistic load: llama.cpp (ggml-vulkan) with a model that fits
   memory, plus a second graphics client (a latency probe or an animated window). Watch `journalctl -t kernel
   --grep 'GPU timeout'` (note: `journalctl -k` implies `-b` and only covers the current boot; use `-t kernel`
   for cross-boot searches). Stop at the first timeout. Log the risk before running: a timeout can corrupt the
   desktop. Save counts and logs into a new `data/<date>-<name>/` folder.
2. **Run the preemption test** (`docs/preempt-test.md`, way A): write `/etc/modprobe.d/zz-gpuparity.conf` with
   `options asahi compute_preempt=1`, reboot, run the same workloads, then remove the file and reboot back. Build
   the auto-revert, watchdog and result logging first (model: DeepIdle's `runner.sh`, `early-restore.sh`,
   `90-deepidle-watchdog.conf`), because a hung GPU needs a reboot and a stopped-in-m1n1 `boot.bin` hangs before any
   watchdog. Results must be written to `data/` by the harness, not only the terminal. **Do not reboot before the
   S2R coordination below is done.** Log the step in the worklog first, with the way back.
3. **Qualify** with linux-aurora#25's criteria: repeated output digests, throughput cost against the
   no-preemption run, compositor concurrency, cancellation and teardown (kill a client mid-dispatch, see #335).
4. **Proper fix, not a gate.** Aurora's hard rules: fix the cause, default command line only, no chip/board/firmware
   gates. The existing `chip_id == 0x6021` selection is itself a gate, so decide from data whether the rule should
   be all G14X, and draft the patch (upstream kernel work in the aurora-silicon/linux(-aurora) trees).
5. **Report** the findings into aurora-silicon/linux#2701 and, if appropriate, open an Aurora PR.
6. **Phase 2:** macOS Metal baseline of gpubench on `MacOSTest` (not `MacOS135`), then the Linux record, then
   `performance-gate.py`; real workloads (llama.cpp/MLX decode and prefill, stable-diffusion.cpp, a game).
   There is no t6020 record in gpubench yet.

## Rules (from the user; these override defaults)

* **Worklog every step.** Add an entry `**G39.**`, `**G40.**`, ... to `docs/worklog.md` before and after each action
  (command, result, data folder, risks, the way back). New entries go before the "Open at the end of this section"
  block, or under a dated heading. Record mistakes and corrections too (see G21, G28, G30).
* **Every number cites a `data/` folder.** Patches state their base commit. A boot with non-default GPU settings is
  logged BEFORE it happens.
* **Commit small and often, straight to `main`. No big PRs, and keep GitHub issues to a minimum** (the many
  DeepIdle issues caused confusion; do not open tracking issues here).
* **No AI trailers on commits** (`Co-Authored-By`, `Claude-Session`, etc.). Commits carry only the human identity
  already configured in git (`user.name`/`user.email`); never set or guess an author. This follows Aurora's contributing
  guide (https://aurorasilicon.org/docs/contributing/). Any PR body ends with `Assisted-by: LLM (<Vendor>:<Model>)`.
  Do not skip git hooks (an earlier `core.hooksPath=/dev/null` shortcut was disclosed and should not be repeated).
* **Aurora reverse-engineering rules apply:** RE output stays private; anything published is clean-room. The repo is
  now public, so do not commit UUIDs, serials, the user's email or other private identifiers (the baseline data
  already redacts UUIDs).
* **Never edit a `main` folder.** Workflow (grove): `cd ~/dev && grove new-quick Aurora-GPUParity <ShortName>
  --no-prompt`, edit in `~/dev/Aurora-GPUParity/quick-<ShortName>`, commit, `grove publish-quick Aurora-GPUParity
  <ShortName>` (the repo is `directPush`, tracker none), then from another folder `grove remove-quick
  Aurora-GPUParity <ShortName>`. See `~/dev/AGENTS.md`.
* **Stopping rules:** keep working until S2R needs the Mac, a hard blocker is hit, or the work is finished. Do not
  ask the user unless you absolutely need them. Tests that risk the root filesystem use the DeepIdle initramfs
  environment.

## S2R coordination (important)

* The S2R work in `Aurora-DeepIdle` runs reboots and has a startup hook that reopens its Claude Code session. Do not
  modify those hooks, units or `/etc/systemd`/GRUB files it owns. If one must change, tell the S2R session exactly what.
* **When you start working, tell the S2R session first**: that work is running, what state you leave on the machine
  (hooks, modprobe files, anything armed for a reboot) and that it may reboot without waiting for you unless you
  say otherwise. This lets S2R reboot many times without waiting on you. Send another message before any
  boot-affecting change (such as the `compute_preempt` modprobe file).
* The S2R session was a Claude Code session named `gh-5-s2r-djnaeempaths-ea`. On 2026-11-24 it was not reachable
  from the previous (Claude) session, probably because it ended with a reboot. If you cannot message it (AGY has
  no Claude cross-session channel), ask the user to relay, or check that no S2R run is armed before any reboot:
  look at `~/dev/Aurora-DeepIdle/main/harness/README.md`, whether `deepidle-runner.service` is enabled, and
  `/boot`/GRUB for a DeepIdle default entry. Never reboot while a DeepIdle plan step is armed.
* When S2R resumes it asks the GPU session to save and stop; then commit and publish the worklog, reply, and say
  when it is good to reboot.

## Pitfalls already hit

* `journalctl -k -b N` only reads the current boot; use `journalctl -t kernel --grep ... -o json`.
* Early-boot clock before the RTC is read gives each boot's first entries the fake time Nov 10 09:39:46 and hostname
  `archlinux`; T6031/G14S journal lines were not a different SoC (G14S is the M2 Pro GPU).
* `kernel-modules-hook` deletes unowned module trees; keep any custom module under a pacman-owned path.
* The merged scaffold PR #1 had left AI trailers in main's history; main was rewritten (G21/G22). GitHub still keeps
  `refs/pull/1/head` at the old commit.

## First actions for you

1. Read `CLAUDE.md`, `README.md`, `docs/status.md`, `docs/plan.md`, `docs/preempt-test.md`, then `docs/worklog.md` in full.
2. Check the machine is as described (`uname -r`, `pacman -Q linux-aurora mesa-aurora`, `lsmod | grep asahi`,
   `journalctl -t kernel --grep 'GPU timeout' --since '-1d'`), and rebuild the probe with `harness/gpuprobe/build.sh`.
3. Tell S2R you are working (see above), log your first step as `**G39.**`, and start with "What to do next" step 1.
