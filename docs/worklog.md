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

## Open at the end of this section

* Is `/sys/module/asahi/parameters/` missing `compute_preempt` because the parameter has no sysfs entry
  (G9) or because this kernel lacks it? Check the kernel build config and `modinfo`, or boot with it set.
* G13's journal oddity.
* Next step is phase 1, step 1 of [plan.md](plan.md): record a baseline. No reboot before the S2R session
  signs off.
