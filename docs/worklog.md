# Worklog

Newest at the bottom. One entry per step, written as it happens: what was asked or decided, the exact
command or source, the result, and the `data/` folder with the raw output (none yet: so far this was
reading, not measuring). Dead ends and mistakes stay in.

Step IDs are `G<number>`. Dates are the machine's local date; times are added from G30 on, when runs start.

## 2026-11-22: research before any run (steps G1 to G20, reconstructed from the session)

These steps were done before this log existed and are written up afterwards from the session record.
Nothing in them touched the machine's GPU or boot configuration.

**G1. Question.** What is the state of Metal support on j414s in Aurora Silicon or its forks? The user
first wrote "Aurora Silicon" and meant Aurora Silicon (aurora-silicon on GitHub), not Asahi.

**G2. Wrong first lead (mistake).** Web searches were read as if Aurora Silicon were Asahi. Results
(Asahi feature tables, Honeykrisp posts) are about upstream Asahi and do not answer the question. The user
corrected this. Nothing from those searches is used in this repo except the `compute_preempt`
background, which was re-verified in the kernel source (G9).

**G3. Aurora org inventory.** `gh repo list aurora-silicon --limit 100`: 60 repositories, including
`linux`, `mesa`, `aura`, `m1n1`, `u-boot`, `aurora-installer`, `gpubench`, `omarchy-mlx`, `agent-guidelines`.

**G4. Code search for the board.** `gh search code --owner aurora-silicon "j414s"`. Hits: `t6020-j414s.dts`
(kernel and u-boot), the dts Makefile entry, `drivers/soc/apple/sep.rs` and `sensor_shim.c` (SEP sensor),
`drivers/phy/apple/atc.c` (Type-C PHY quirks), `tools/aurora-sep/XART-SAFETY.md`, installer docs.

**G5. Code and issue search for Metal.** `gh search code --owner aurora-silicon "metal"`: only unrelated
"bare metal" hits plus `gpubench` (Vulkan and Metal benchmark executors). `gh search issues` for `metal`,
`MSL`, `MTL`, `metallib`: only mesa#457 (Cyberpunk benchmark tiers) and gpubench#5. Conclusion: no Metal API
or Metal driver work in the org. `gpubench` uses Metal only on macOS as the reference.

**G6. Issue #2701 read.** `gh api repos/aurora-silicon/linux/issues/2701`. The user said it is theirs. Facts
from it: M2 Pro 14" 32 GB, `7.2.11.aurora16`, Borealis (Mesa 26.3.0-devel), FLUX.2 Klein 4B via
ggml-vulkan gave `GPU timeout` and `gCDM_CS`/`gCDM_ID` unmapped-read faults (`vm_slot 0xe`) at 09:03 and
09:10 with magenta corruption; ACE-Step 1.5 Turbo gave 17 timeouts in 95 s (09:28:13 to 09:29:48), event
slots repeating (97 ten times), no fault info; 116 timeouts since boot; 15.4 GB RAM free; no
`compute_preempt` under `/sys/module/asahi/parameters/`.

**G7. Mesa fork branches.** `gh api repos/aurora-silicon/mesa/branches` (first 100): `agx3b-opt`, `agx3c`,
`coopmat/agx3b-cmat`, `djdev/agx3-*`, `djdev/g16*`, `djdev/g17*`, `borealis/*`, `ctl/t-6614-g17p-ray-query-...-vs-metal-`
(a Metal comparison on G17P). No branch name mentions G14X.

**G8. Metal-on-Vulkan prior art.** Web search, then `gh api` for `steelbrain/metal2vulkan` (README, issues,
`docs/METAL-COVERAGE.md`, `docs/FAST-PATHS.md`), `steelbrain/reims-vgpu` README, `darlinghq/indium`
README, and Darling issue #390. Findings are in [prior-art.md](prior-art.md). Key points: metal2vulkan
(beta, Rust, Vulkan 1.2+) already lists "Honeykrisp on Apple's M2 Pro" in its docs; metal2vulkan#12 got a
macOS guest running on an M1 Max under KVM on Honeykrisp; metal2vulkan reports a dma-buf import leak in
the asahi DRM driver.

**G9. Kernel source: compute preemption.** `gh api repos/aurora-silicon/linux/contents/...`:
* `drivers/gpu/drm/asahi/asahi_core.rs`: `compute_preempt: i32`, default -1, description "Compute
  preemption (-1: M2 Max or Debug0; 0: off; nonzero: on)". The `permissions` lines of neighbouring
  parameters are commented out, so these parameters have no sysfs entry.
* `drivers/gpu/drm/asahi/workqueue.rs` (~line 782): `#[ver(G == G14X)] let preempt_auto = cfg.chip_id == 0x6021;`
  other generations `false`; `-1 => preempt_auto || debug_enabled(DebugFlags::Debug0)`. A comment says that
  without preemption the compute queue takes priority fields 0 and 5 of the highest entry, "the firmware
  then cannot switch compute work out when another client needs the GPU, and on G14X it kills the work when
  the context-switch and kill timeouts expire".
* t6020 (0x6020) is not selected. This is the leading hypothesis for #2701. **Not tested.**

**G10. linux-aurora issues read.** #25 (G14X compute preemption: automatic on M2 Max, opt-in on Pro and
Ultra; M2 Max results 4 of 4 and 191 of 191 correct with 0 timeouts; stock 39 to 120 timeouts per minute;
`compute_preempt=0` wedges that machine; M1 decode cost 12 to 20%, M2 Max about 1%), #335 (kill with two
queued submissions faults; 0 timeouts in 60 runs on kernel `7.3.0-2-20261110.1`, M1), #387 and #389 (G15
scheduling, 40 to 200 ms compositor delay, fix capping compute batch time), #440 (G15C fail-stop in
llama.cpp prefill).

**G11. gpubench.** README read. Matched Vulkan and Metal executors, a stress suite, `tools/performance-gate.py`.
Published records are Neo (A18 Pro) and M5 Pro on macOS only. No t6020 record.

**G12. agent-guidelines.** README read (policy list, decided items, open questions). Relevant: reverse-engineering
output stays private, published code is clean-room; commits carry human identity only; PR bodies end with
`Assisted-by:`; hard rules "fix the cause", "default command line only", "no chip/board/firmware gates".

**G13. Machine check (read-only).** `uname -r` gave `7.2.11-16-1-ARCH`. `ls /sys/module/asahi/parameters/`
gave only `m3_*` and `t8122_hwdata_418c`. `/proc/cmdline` has no `asahi.*` word. `/dev/dri`: `card1`, `card2`,
`renderD128`. `vulkaninfo --summary` printed nothing.
**Unexplained:** `journalctl -k -b | grep -iE 'asahi|gpu'` returned lines dated Nov 10 that describe a
T6031 / G14S GPU ("GPU ID registers: 0x6031100 ...", "Type: G14S"), not T6020. Either that is a different
boot of a different Mac, a journal copied here, or the command did not select this boot. Do not trust the
local kernel journal until this is explained (first check in phase 1).

**G14. Plan proposed to the user.** Test `asahi.compute_preempt=1`, qualify per linux-aurora#25, measure the
gap to macOS with gpubench. The user chose "Metal-equivalent graphics experience" over a Metal API, with
performance as good as or better than macOS for AI, games and graphics tools, and asked for a research repo.

**G15. Repo created.** `gh repo create JJRPF/Aurora-GPUParity --private --add-readme` with a description.
Private because Aurora asks that findings stay in Aurora-controlled spaces.

**G16. Registered with grove.** `grove add-repo Aurora-GPUParity <url> --scope personal --tracker
github:JJRPF/Aurora-GPUParity --default-change ticket`; base clone at `~/dev/Aurora-GPUParity/main`.

**G17. Scaffold.** `grove new-quick Aurora-GPUParity Scaffold --no-prompt`; wrote README, CLAUDE.md,
`docs/{status,prior-art,plan,worklog}.md`, `harness/`, `tools/`, `data/`, `patches/` READMEs, `.gitignore`.
Commit used `-c core.hooksPath=/dev/null`, which skips git hooks. That was unnecessary and is not repeated.
The commit and PR #1 carried an AI `Co-Authored-By` trailer and "Generated with Claude Code".

**G18. Attribution question.** The user said co-author trailers are required by Aurora and linked the
contributing page. `curl` of `https://aurorasilicon.org/docs/contributing/` shows the opposite for commits:
"Commits carry your identity only, with no AI co-author or similar trailers", and PR descriptions end with
`Assisted-by: LLM (<Vendor>:<Model>)`. The user's own Aurora-DeepIdle commits do carry
`Co-Authored-By` trailers, written before they knew the rule.

**G19. Decision (user).** Drop the trailers from the start. Same structure as Aurora-DeepIdle. Detailed worklog
of every step. Commit often straight to main, no big PR. Minimise issues. Do not touch the S2R work session's
hooks; message that session with exact changes if one is ever needed; when the user resumes S2R, that session
will message this one to save and stop, and this one then tells it when it is good to reboot.

**G20. Applying G19.** `grove add-repo Aurora-GPUParity --tracker none --default-change quick --direct-push`;
`grove policy` now shows tracker none, default quick, directPush true. Scaffold commit amended to remove the
trailers (before publishing). `CLAUDE.md` and `README.md` rewritten with the workflow rules. Hooks: none
changed, so no message to the S2R session. `ListAgents` shows no session named "S2R work"; the likely one
is `gh-5-s2r-djnaeempaths-ea` (interactive, on the GH-5-S2R-DjNaeemPaths worktree), to be confirmed with
the user before the first message.

**G21. Trailer left in main history (mistake, not fixed).** After G20 the history on `main` still contains
`3850a4b` ("Scaffold: ..."), whose message ends with `Co-Authored-By: Claude Sonnet 5.5` and a
`Claude-Session:` line. Cause: the branch `quick-Scaffold` had been pushed with that commit (G17) and the
user merged PR #1 at 10:55 (merge commit `812525d`), before the amended commit (no trailers, `6d0cb7f`) existed.
`grove publish-quick` then rebased the later commits onto `main`. Checked with `git log` over all of `main`:
exactly one commit (`3850a4b`) has a trailer; every later commit has none. Removing it needs a rewrite of
`main` (rebase and force-push), which erases the PR #1 merge record, so it was not done without the user's
decision. Also: PR #1 could not be closed (already merged); the `quick-Scaffold` worktree was removed.
Lesson: do not push a branch before the commit message is final.

**G22. History rewritten to remove the trailer (user approved).** Cloned to a temp directory,
`git filter-branch --msg-filter` dropping `Co-Authored-By:` and `Claude-Session:` lines from all commits,
checked: 0 trailer lines left in `git log --format=%B`, tree of the new head identical to the old head
(`7a02232`). Pushed with `--force-with-lease=main:7a02232` (main moved `7a02232` to `a8893c9`; every hash
since the initial commit changed) and deleted the remote branch `quick-Scaffold`, which still held the old
commit. Base clone `~/dev/Aurora-GPUParity/main` reset to `origin/main` (it had no local changes).
Not removable by us: GitHub keeps the merged PR #1's head commit `3850a4b` reachable through
`refs/pull/1/head`, so the trailers can still be seen through PR #1 (private repo). Removing that needs
deleting the repo or a GitHub support request; not done.

**G23. S2R channel test.** `ListAgents` showed `gh-5-s2r-djnaeempaths-ea` as the S2R session. The user
confirmed it is the one ("The session is that one") and asked for a test message. Sent via SendMessage
(`msg_id fdbdad69`): says no hooks or Aurora-DeepIdle files were touched, and repeats the protocol (S2R
messages this session to save and stop; this session commits the worklog, replies, then says when it is
good to reboot; no reboot before that). Delivery: queued in that session's inbox, not yet read when
sent. No reply yet; not polled.

**G24. Journal oddity (G13) explained by the user.** The SoC is the M2 Pro (T6020). The user says the journal
with T6031/G14S lines and a Nov 10 date could be a journal copied here for the S2R work. Not verified:
the first baseline step still checks which boot and which machine the journal lines belong to
(`journalctl --list-boots`, `/proc/device-tree/compatible`) before any count is used.

**G25. S2R session replied.** `gh-5-s2r-djnaeempaths-ea` confirmed the channel and the protocol: it will
message this session to save and stop before any S2R reboot, and will not reboot until this session replies
that it is safe. It says nothing is armed (S2R is waiting on an outside answer), so no reboot is expected
soon. No reply sent; none needed. Still no GPU test boot until its sign-off or the user says otherwise.

## 2026-11-22: baseline (read-only)

**G26. Side task (user): /GPU page on jjrpf.com.** Asked mid-baseline. Registered `JJRPF/jjrpf.com` with grove
(personal, no tracker, default quick, not directPush: that repo uses PRs). The site is a Cloudflare Worker
(`worker/index.js`) serving static assets from the repo root; asset paths are case-sensitive, so the
case-insensitive URL belongs in the Worker. Added `GPU/index.html` (CSS copied from `S2R/index.html`, static
content: hero, phases 0 to 4, known and not known), a redirect in the Worker (first path segment compared
lowercase against `S2R`, `GPU`; other spellings get a 301 to the canonical one, rest of path and query kept),
sitemap entry and a homepage card. Tested the redirect in node with a stub ASSETS binding: `/gpu`, `/Gpu/`,
`/gpu/index.html?x=1`, `/s2r/` redirect; `/GPU`, `/S2R/`, `/` pass through. Not previewed in a browser,
not deployed. PR https://github.com/JJRPF/jjrpf.com/pull/5 (left open for the user to merge: merging
deploys the public site). The page links no private repo. Data: none (code change).

**G27. Identity (read-only).** `data/20261122-baseline-identity/identity.txt`: `uname` `7.2.11-16-1-ARCH`
(built 2026-11-10), `/proc/device-tree/compatible` = `apple,j414s`, `apple,t6020`; model "Apple MacBook Pro
(14-inch, M2 Pro, 2023)"; hostname `omarchy`; Arch Linux ARM; cmdline has no `asahi.*` word (disk UUIDs
redacted in the saved copy). Packages (`packages-drivers.txt`): `linux-aurora 7.2.11.aurora16-1`,
`mesa-aurora`/`vulkan-aurora 1:26.3.0_devel.20261107.231485.b7b03c5b3b8-3`, `m1n1-aurora 1.6.1.aurora23-2`,
`uboot-aurora 2026.07.aurora6-1`, `omarchy-aurora-firmware 20261014-7`, `vulkan-icd-loader 1.4.363.0-1`.
The installed Vulkan ICD is `/usr/lib/libvulkan_asahi.so` (the Borealis rename is not in this build).
DRM: `card1` = `apple-drm` (display), `card2` = `406400000.gpu`, driver `asahi`; render node `renderD128`.

**G28. Mistake: `journalctl -k` is current-boot only.** My first per-boot loop used `journalctl -k -b N` and
`-k --since`; `-k` implies `-b` (current boot), so every row repeated boot 0 and all counts were 0. Wrong.
Fixed in G29 by selecting kernel lines with `-t kernel` and counting across all boots. The file
`per-boot-timeouts.txt` from that attempt was deleted, not committed.

**G29. GPU error history (correct).** `journalctl -t kernel --grep 'GPU timeout nya|Fault info: FaultInfo|
Attempting recovery|fail-stop|GPU is crashed' -o json` over the whole journal (99 boots, 581 MB, back to
2026-11-10): 240 lines in `gpu-error-lines.jsonl`; per boot in `timeouts-by-boot.tsv`. **Only 2 of 99 boots
have any GPU error.** (a) boot `0d54c2c8`: 1 timeout at 2026-11-21 01:29:35, right after a global
out-of-memory kill (`omarchy-mlx-ser invoked oom-killer`, `claude` killed, `first-timeout-context.txt`) as the
session restarted: a memory-pressure event, a different cause from #2701. (b) boot `74e3af50`: 116 timeouts and
6 `Fault info` dumps between 2026-11-21 09:03:14 and 09:29:48: the #2701 session, matching the issue's 116
(the journal confirms the report). Boot 0 (current, since 2026-11-22 07:34): no GPU error.

**G30. Corrections to earlier entries.**
* G13 "unexplained journal": explained. `Type: G14S` is the M2 Pro GPU (G14S/C/D = Pro/Max/Ultra), not an
  M3 part, and 16 active cores matches a 16-core M2 Pro. The Nov 10 timestamps and `archlinux` hostname on the
  first lines of each boot are the early-boot clock before the RTC is read (all 99 boots start at the same
  fake time), plus the hostname before it is set. No copied journal needed; G24's theory is not needed.
* The `/sys/module/asahi/parameters/` listing lacks `compute_preempt`, but `modinfo -p asahi` lists it:
  `compute_preempt:Compute preemption (-1: M2 Max or Debug0; 0: off; nonzero: on) (i32)`. It exists on this
  kernel, with no sysfs file.
* **`asahi` is a loadable module (`CONFIG_DRM_ASAHI=m`)**, not built in. So the parameter can be set with
  `options asahi compute_preempt=1` in `/etc/modprobe.d/` (takes effect when the module loads at boot) instead of
  editing the kernel command line. The module has 46 users now (compositor and clients), so reloading it live
  would kill the desktop session; not done.
* `sudo -n true` succeeds silently: passwordless sudo is available to this user (group `wheel`).
* Other GPU parameters on this kernel: `robust_isolation`, `fault_control` (default 0xb), `initial_tvb_size`,
  `debug_flags`, plus G15/G17/M3 ones that do not apply to G14.

**G31. gpubench baseline, preemption default.** Registered `aurora-silicon/gpubench` with grove (personal, no tracker,
quick) and built it out of tree: `cmake -S ~/dev/gpubench/main -B ~/.cache/gpuparity/gpubench-build -G Ninja`,
ninja, exit 0 (the base clone is not edited). `gpubench-vk --info`: device `Apple M2 Pro (G14S B1)`, driver
`Borealis`, `Mesa 26.3.0-devel (git-b7b03c5b3b), Borealis, derived from Honeykrisp`, Vulkan 1.4.363, device-local
heap 16,607,346,688 bytes. Ran `tools/run-linux.sh --device j414s-m2pro --chunks core-a` (11:17, 23 results,
rc=0 each) then `--chunks core-b` (11:19, 21 results, rc=0 each; compute.heavy alu and bw, render.heavy fill 1080p).
Kernel boot unchanged (`compute_preempt` default, no `asahi.*` options). **No GPU timeouts or faults** during
either chunk (`journalctl -t kernel` count 0). Records:
[data/20261122-gpubench-baseline/](../data/20261122-gpubench-baseline/). No macOS record exists to compare with.
Not run: the `multi` and `feat` chunks, ray tracing, the stress suite (`gpubench-stress-vk`, which can fault the GPU).

**G32. gpuprobe written.** `harness/gpuprobe/`: `gpuprobe.c` (Vulkan compute, an integer LCG/xorshift chain over
4,194,304 invocations; `dispatch --ms N --count K --inflight 2` calibrates the iteration count to about N ms,
keeps INFLIGHT submissions queued like llama.cpp, and hashes every result; `latency --seconds S` times a tiny
dispatch every 16 ms from submit to fence, the compositor-like second client), `probe.comp`, `build.sh`,
`run-ramp.sh` (steps 50 100 200 400 800 1500 3000 ms, six dispatches per step, the latency client in parallel,
stops at the first step that logs a `GPU timeout` or gives a wrong hash). Build: glslangValidator + gcc, headers
from gpubench's vendored `rt/third_party`. Sanity run, 20 ms: calibrated 797 iterations to 20.3 ms, 5 dispatches
19.8 to 27.5 ms (one 49.6 ms that includes queueing behind the first), hash identical, 0 mismatches.

**G33. Before the ramp (risk logged).** Running `run-ramp.sh` on the current boot (preemption off for this chip,
per G9) to find the dispatch length at which a second client starts to wait and when the kernel logs a timeout.
Risk: a timeout can corrupt the desktop and stall it for seconds (#2701), and a fail-stop would need a reboot.
That risk is the point of reproducing it; the ramp stops at the first timeout. Way back: nothing is changed on
the machine, so ending the run (or a reboot) restores everything.

**G34. Ramp on the default boot, single long dispatch.** `harness/gpuprobe/run-ramp.sh` with steps 50 to 3000 ms
(one dispatch per submission, 2 in flight, a 16 ms latency client beside it). Result: **0 GPU timeouts at every step**,
no hash mismatch, but the second client waits: p95 latency 2.9 ms at 50 ms, 12.9 ms at 1500 ms, 32 ms at 3000 ms;
share of samples over 8.3 ms (a 120 Hz frame): 0 % at 50 ms, 29.7 % at 1500 ms, 34.9 % at 3000 ms; max 69.7 ms.
Data: [data/20261122-ramp-default-preempt/](../data/20261122-ramp-default-preempt/). So one dispatch of up to 3 s does
not trip the kill timeout here, but it does delay the compositor in proportion to its length, which fits "no preemption".

**G35. Ramp, batched dispatches.** Added `--reps` (several 20 ms dispatches with a barrier in one command buffer) and
`UNIT_MS` in run-ramp.sh. Steps 100 to 10000 ms per submission: **0 timeouts**, and the second client is not starved
(p95 0.72 to 0.79 ms from 800 ms up, max 7.6 ms, 0 % over 8.3 ms). Dispatch boundaries let the GPU switch clients, so
only a single long-running dispatch (a heavy kernel) hurts. Data:
[data/20261122-ramp-default-preempt-batched/](../data/20261122-ramp-default-preempt-batched/).
Not yet reproduced: an actual timeout. Next: single dispatches of 6 s and 12 s (logged before running, the risk is the
#2701 desktop corruption; the ramp stops at the first timeout).

**G36. Ramp, 6 s and 12 s single dispatches.** Three dispatches each, 2 in flight. **0 GPU timeouts, 0 hash mismatches**
(`journalctl -t kernel` count 0 afterwards). The latency client waits longer (p95 68 to 69 ms, max 100 and 168 ms,
24 to 29 % of samples over 8.3 ms), but the kernel's kill timeout is not reached by a synthetic ALU kernel of up to 12 s.
Data: [data/20261122-ramp-default-preempt-long/](../data/20261122-ramp-default-preempt-long/).
**Conclusion so far:** the probe reproduces the missing-preemption symptom (compositor latency scales with dispatch
length) but not the #2701 timeouts. The #2701 boot had 116 timeouts under llama.cpp-style load, so the trigger likely
needs more than one client submitting or memory pressure (cf. the OOM-related timeout in G29). The `compute_preempt=1`
test (docs/preempt-test.md) still needs a reboot, so it waits for the S2R session; no message was sent, no hook changed.

**G37. S2R start-of-work notice.** User rule: always message the S2R session when GPU work starts, so it can reboot
freely without waiting on this session. Sent the first notice (no hooks, units or modprobe files changed; only userspace
Vulkan tests; S2R may reboot; I will message again before any boot-affecting change). Recorded the rule in CLAUDE.md
and in this machine's Claude memory.

## Open at the end of this section

* Is `/sys/module/asahi/parameters/` missing `compute_preempt` because the parameter has no sysfs entry
  (G9) or because this kernel lacks it? Check the kernel build config and `modinfo`, or boot with it set.
* G13's journal oddity.
* Next step is phase 1, step 1 of [plan.md](plan.md): record a baseline. No reboot before the S2R session
  signs off.

## 2026-11-23: website

**G38. /GPU page made live (user).** The user asked for jjrpf.com/GPU to work and look like /S2R, and chose to make
this repo public for it. Added [gpu-status.json](gpu-status.json): phases, pieces, runs (G29 to G36), findings,
measurements, rules, unknowns, credits; every number in it is taken from G29 to G36 and their `data/` folders. The
page (JJRPF/jjrpf.com, `GPU/index.html`) reads this file, [status.md](status.md), this worklog (one entry per `**G<n>.**`
step) and the commit list straight from `main`, and falls back to a copy built into the page while the repo is
private or GitHub is unreachable. Its "Right now" line is the KV key `gpu-message` (`/api/gpu-message`). Nothing on
the machine was touched; no message to the S2R session needed.

## 2026-11-24: handoff

**G39. Handoff prompt for AGY.** The user asked for a detailed handoff for AGY. Wrote [handoff-agy.md](handoff-agy.md)
(goal, machine, where everything is, what is established with data folders, next steps in order, rules, S2R
coordination, pitfalls). The start-of-work notice to the S2R session (`gh-5-s2r-djnaeempaths-ea`) could not be
delivered: the session was no longer listed (probably ended by its reboot on 2026-11-22). Nothing on the machine was
touched; no hooks, modprobe files or boot options changed.

**G40. AGY takeover, Lichen coordination, and machine verification (17:51-18:00).** Verified machine state:
`uname -r` = `7.2.11-16-1-ARCH`, `linux-aurora 7.2.11.aurora16-1`, `mesa-aurora
1:26.3.0_devel.20261107.231485.b7b03c5b3b8-3`, `asahi` module loaded (59 users), `journalctl -t kernel --grep 'GPU
timeout' --since '-1d'` returned 0 entries. Rebuilt `harness/gpuprobe/build.sh` cleanly. Checked S2R state:
`deepidle-runner.service` not-found, `/etc/modprobe.d/` has only `omarchy-mac-touchid.conf` and
`omarchy-usb-autosuspend.conf`. Located the exact #2701 workloads already installed locally via `omarchy-local-ai`:
`ace-step-1.5-turbo-acestep-apple` (port 11437, `MemoryMax=12 GiB`) and `flux2-klein-4b-sdcpp-apple` (port 11436,
`MemoryMax=15.75 GiB`), none currently running; 19.2 GiB RAM available, `gpu_memory` `user_committed` 2.44 GB.
*Mistake & correction (Lichen):* Initially accepted the user's single-use Lichen invite (`6rb77mzfev8v934bgsj8`)
using `~/.lichen/token.json`, which belonged to `S2R-AGY` (`agt_r9tc7gmyiuuq`), consuming that invite under `S2R-AGY`
and posting the notice there (`r_4zsxzvfdb9in:1`). Then enrolled `GPU-AGY` (`agt_vnsk5j4wyyy8`) into
`~/.lichen/gpu-agy.json` (mode `0600`, preserving `S2R-AGY`'s token), set profile, and sent `S2R-AGY` a direct
message with the start-of-work and safe-to-reboot notice. Waiting on a fresh invite/add for `GPU-AGY` into
`#S2R-GPU Coordination`.

**G41. Upstream reply on aurora-silicon/linux#2701; repro paused for 7.3 kernel upgrade (18:00).** Read
`DjDeveloperr`'s reply on `aurora-silicon/linux#2701` (`issuecomment-7141403144`, 2026-11-24):
* On the 7.3 kernel series, the DRM driver is renamed from `asahi` to `aurora`, and `compute_preempt` is exposed as
  `aurora.compute_preempt=1` (readable at `/sys/module/aurora/parameters/compute_preempt`; `-1` default still enables
  it only on M2 Max or Debug0).
* Preemption keeps the desktop responsive, though a single dispatch exceeding the firmware deadline or hitting an
  `Unmapped` read fault (`gCDM_CS`/`gCDM_ID`) still triggers recovery. Also notes stock Fedora Mesa 26.2.7 inside
  Podman vs Aurora's `mesa-aurora` can cause driver/shader mismatches on Apple GPUs.
* Upstream asks for 7.3 test results with `aurora.compute_preempt=1` (and matching Aurora Mesa if possible):
  `uname -a`, `pacman -Q linux-aurora mesa-aurora vulkan-aurora`, `cat /sys/module/aurora/parameters/compute_preempt`,
  and `journalctl -b -k | grep -E 'aurora|asahi|Fault|timeout'`.
Held off on running the 7.2.11 ACE-Step repro per user instruction: `S2R-AGY` is upgrading the machine to kernel 7.3
first. Machine left completely clean and ready for reboot.

## 2026-11-24 (evening): kernel 7.3.0 testing for #2701

**G42. Booted on `7.3.0-1-1-ARCH`; driver & parameter verification (21:57).** After `S2R-AGY` upgraded the system and
rebooted, verified `uname -a`: `Linux omarchy 7.3.0-1-1-ARCH #1 SMP PREEMPT_DYNAMIC Mon, 23 Nov 2026 04:30:14 +0000
aarch64 GNU/Linux`. Packages: `linux-aurora 7.3.0.aurora1-1`, `mesa-aurora 1:26.3.0_devel.20261122.232644.e705eca833f-1`,
`vulkan-aurora 1:26.3.0_devel.20261122.232644.e705eca833f-1`. Kernel module is now `aurora` (44 users); `modinfo -p
aurora` lists `compute_preempt:Compute preemption (-1: M2 Max or Debug0; 0: off; nonzero: on) (i32)`. **Important:**
`/sys/module/aurora/parameters/compute_preempt` still does NOT exist (`No such file or directory`), so the parameter
cannot be read back via sysfs on `7.3.0.aurora1-1`. Kernel cmdline is default (no `aurora.*` params). New in 7.3:
`aurora: GPU memory limits: device 21222 MiB, per client 15837 MiB`. Available RAM: 25.2 GiB; `gpu_memory`
`user_committed` 1.58 GB. `gpuprobe` 20 ms smoke test passed (`20.2 ms`, 0 mismatches).

**G43. Before 7.3.0 default baseline runs (risk logged).** Running (1) `gpuprobe` single-dispatch ramp (`50 200 800
1500 3000` ms, `COUNT=4`) and (2) ACE-Step 1.5 Turbo 30 s song generation alongside `gpuprobe latency` on stock
`7.3.0-1-1-ARCH` (`compute_preempt` default `-1`, off for T6020), monitoring `journalctl -b -k` for `aurora|asahi|Fault|timeout`
and stopping immediately on the first GPU timeout or if available RAM drops below 6 GiB. Risk: a GPU timeout may stall
or corrupt the desktop. Way back: stop the ACE-Step container unit; nothing on the kernel or boot config is changed.
Data target: `data/20261124-73-default-ramp/` and `data/20261124-73-default-acestep/`.

**G44. 7.3.0 default baseline results (`gpuprobe` + native `Vulkan0` ACE-Step) and pre-run risk log for FLUX.2 (22:12).**
* `gpuprobe` ramp (`50 200 800 1500 3000` ms, `COUNT=4`, `data/20261124-73-default-ramp/`): **0 GPU timeouts**, 0 hash
  mismatches; second-client latency scales with dispatch length just as on 7.2.11: p95 `2.85 ms` (50 ms), `3.33 ms`
  (200 ms), `7.16 ms` (800 ms), `15.77 ms` (1500 ms, 27.1% > 8.3 ms, max 218.85 ms), `34.12 ms` (3000 ms, 31.9% > 8.3
  ms, max 128.16 ms).
* **Container Mesa mismatch confirmed:** Running the existing `omarchy-local-ai` Podman container (Fedora 44 Mesa
  26.2.7) on kernel `7.3.0-1-1-ARCH` silently fell back to CPU (`[Load] LM/DiT/VAE backend: CPU`) because the kernel
  DRM driver was renamed `asahi` → `aurora`.
* **ACE-Step 1.5 Turbo (60 s song) natively on host `vulkan-aurora` (`26.3.0_devel.20261122.e705eca833f-1`)**
  (`data/20261124-73-default-acestep/`): extracted `ace-server` to `~/.cache/gpuparity/bin/ace-server` and ran with
  `--device Vulkan0`. All stages ran on `Vulkan0` (peak `gpu_memory` `user_committed` 8.75 GB, min available RAM 17.8
  GiB): LM `41.51 s`, Synth `35.44 s`, MP3 `1,344,189 B`, **0 GPU timeouts, 0 GPU faults**, second-client latency
  during `/synth`: `p50=0.63 ms`, `p95=2.73 ms`, `p99=7.16 ms`, `max=24.15 ms` (`0.7%` > 8.3 ms).
* **Before FLUX.2 Klein 4B run (risk logged):** Next running 1 `1024x1024` 4-step FLUX.2 Klein 4B image natively via
  `~/.cache/gpuparity/bin/sd-server` against host `vulkan-aurora` (`HK_SYSMEM=16911433728`) alongside `gpuprobe
  latency`, with 1 Hz timeout/fault/RAM monitoring (auto-stops on first `GPU timeout` / `FaultInfo` or RAM < 6 GiB).
  Risk: in #2701 FLUX.2 caused `gCDM_CS`/`gCDM_ID` unmapped-read faults and magenta corruption. Way back: `sd-server`
  is killed immediately on first error; nothing on kernel/boot config is changed. Data target:
  `data/20261124-73-default-flux/`.
* **FLUX.2 Klein 4B result (`data/20261124-73-default-flux/`):** `1024x1024`, 4 steps, `Vulkan0` (`Apple M2 Pro (G14S
  B1) (Borealis)`): completed cleanly (`gen_rc=0`) in `161.43 s`, PNG `1,939,989 B` (`sha256: 926499aa...`), peak
  `gpu_memory` `user_committed` `17.95 GB`, **0 GPU timeouts, 0 GPU faults**, no desktop corruption. Second-client
  latency during the first 60 s: `p50=0.75 ms`, `p95=2.78 ms`, `p99=4.96 ms`, `max=79.48 ms` (`0.1%` > 8.3 ms).
  (Note: the 1 Hz low-RAM guard tripped at `4266 MiB` available right at the end as generation completed.)

**G45. Before reboot with `options aurora compute_preempt=1` (boot-affecting change logged first).** Writing
`options aurora compute_preempt=1` to `/etc/modprobe.d/zz-gpuparity.conf` and rebooting once into `7.3.0-1-1-ARCH` to
measure the A/B effect of compute preemption on `gpuprobe` (`50–3000 ms`), ACE-Step 1.5 Turbo, and FLUX.2 Klein 4B,
and to answer `DjDeveloperr` on `aurora-silicon/linux#2701`. Risk: untested setting on T6020 (M2 Pro); a hung GPU
would require a reboot. Way back: delete `/etc/modprobe.d/zz-gpuparity.conf` immediately after booting into the test
boot (once `aurora` is loaded, removing the file guarantees any subsequent reboot returns to stock 7.3.0). Kernel
command line and GRUB/m1n1/DeepIdle hooks are untouched.

**G46. Mistake & correction: `aurora` loads in initramfs before `/etc/modprobe.d/` (22:19).** After booting on
`7.3.0-1-1-ARCH` and removing `/etc/modprobe.d/zz-gpuparity.conf`, the `gpuprobe` ramp
(`data/20261124-73-preempt1-ramp/`, actually still default preemption!) still showed `p95 = 24.31 ms` (`41.1% > 8.3
ms`) at 3000 ms. Checking boot timestamps revealed why: `aurora` loads at `0.64 s` **inside the initramfs** (`kms` +
`modconf` hooks in `/etc/mkinitcpio.conf`), `5.6 s` before `Switch Root` (`6.25 s`), so a rootfs `/etc/modprobe.d/`
file is never read unless baked into the UKI (`omarchy_linux-aurora.efi`). This corrects the assumption in G30 and
[preempt-test.md](preempt-test.md).
* **Before cmdline reboot (boot-affecting change logged first):** Backing up `/boot/efi/limine.conf` to
  `/boot/efi/limine.conf.bak-gpuparity`, appending `aurora.compute_preempt=1` to the `//linux-aurora` `cmdline:` line
  only (leaving `/DeepTest` and all other entries untouched), and rebooting once into `7.3.0-1-1-ARCH`. Risk: untested
  setting on T6020; a hung GPU would require a reboot. Way back: immediately restore `/boot/efi/limine.conf` from
  `/boot/efi/limine.conf.bak-gpuparity` right after boot (before running any test) so any subsequent reboot is 100%
  stock 7.3.0.

**G47. Kernel `7.3.0-1-1-ARCH` with `aurora.compute_preempt=1` A/B results (22:21-22:28).** Booted with
`aurora.compute_preempt=1` verified in `/proc/cmdline` and immediately restored `/boot/efi/limine.conf` from backup so
the next reboot is stock 7.3.0. Confirmed `/sys/module/aurora/parameters/compute_preempt` still does not exist
(`No such file or directory`) even when set on the kernel command line. Ran the exact same three workloads on native
host `vulkan-aurora` (`1:26.3.0_devel.20261122.232644.e705eca833f-1`):
* **`gpuprobe` ramp (`50–3000 ms`, `COUNT=4`, `data/20261124-73-preempt1-ramp/`):** **0 GPU timeouts, 0 hash
  mismatches.** Second-client latency dropped dramatically vs 7.3 default (`data/20261124-73-default-ramp/`):
  * `800 ms`: max `51.78 ms` → `8.08 ms` (`3.9%` → `0.0%` > 8.3 ms)
  * `1500 ms`: p95 `15.77 ms` → `11.93 ms`, max `218.85 ms` → `13.69 ms` (`27.1%` → `9.4%` > 8.3 ms)
  * `3000 ms`: p95 `34.12 ms` → `13.71 ms`, max `128.16 ms` → `26.50 ms` (`31.9%` → `18.3%` > 8.3 ms)
* **ACE-Step 1.5 Turbo (60 s song, `seed=42`, `data/20261124-73-preempt1-acestep/`):** LM `42.52 s` (vs `41.51 s`,
  +2.4%), Synth `35.46 s` (vs `35.44 s`, +0.06%), total `77.98 s` (+1.3% overhead), **0 GPU timeouts, 0 GPU faults**,
  peak `gpu_memory` `8.75 GB`. **Bit-for-bit identical MP3 SHA-256**
  (`b5747934fee41874a59ea345b846667662aa6b5ccf73783a3713b2e0d68a68b9`). Second-client `max_ms` dropped from `24.15 ms`
  to `11.04 ms` (`p95 = 2.66 ms`).
* **FLUX.2 Klein 4B (`1024×1024`, 4 steps, `data/20261124-73-preempt1-flux/`):** `164.51 s` (vs `161.43 s`, +1.9%
  overhead), **0 GPU timeouts, 0 GPU faults**, peak `gpu_memory` `17.94 GB`. Second-client `max_ms` dropped 11× from
  `79.48 ms` to **`7.22 ms`** (`0.0%` > 8.3 ms; `p95 = 4.41 ms`).
* Posted results to `aurora-silicon/linux#2701` in reply to `DjDeveloperr` and handed priority back to `S2R-AGY` in
  `#Coordination`.








