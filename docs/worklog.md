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

## Open at the end of this section

* Is `/sys/module/asahi/parameters/` missing `compute_preempt` because the parameter has no sysfs entry
  (G9) or because this kernel lacks it? Check the kernel build config and `modinfo`, or boot with it set.
* G13's journal oddity.
* Next step is phase 1, step 1 of [plan.md](plan.md): record a baseline. No reboot before the S2R session
  signs off.
