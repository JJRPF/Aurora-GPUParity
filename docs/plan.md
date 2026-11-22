# Plan

## Phase 1: fix #2701 (robustness)

1. **Baseline.** Reproduce on the current kernel and record: `gpubench-stress-vk`, a FLUX.2 run and an
   ACE-Step run, plus `journalctl -k` counts of `GPU timeout`. Data in `data/`.
2. **Test preemption.** One boot with `asahi.compute_preempt=1` on the kernel command line, same
   workloads. The way back is removing the parameter. The M2 Max report says `compute_preempt=0` wedged
   that machine, so keep a second way into the machine (ssh) before booting. Log the boot in the worklog first.
3. **Qualify** with the criteria from linux-aurora#25: repeated output digests, throughput against the
   no-preemption run, compositor concurrency, cancellation and teardown (kill a client mid-dispatch).
4. **Proper fix, not a gate.** Aurora's hard rule is no chip/board gates, but the current auto-selection
   is `chip_id == 0x6021`. Decide with the data whether the right rule is all G14X, and draft the patch.
5. **Report** into aurora-silicon/linux#2701 and, if accepted, an Aurora PR: commits carry the human's
   identity only, the PR body ends with `Assisted-by: LLM (<Vendor>:<Model>)`.

## Phase 2: measure the gap to macOS

1. Boot macOS on this machine (the installer keeps a stub) and record `gpubench` with the Metal executor.
2. Record the same suite on Linux with Borealis.
3. Run `tools/performance-gate.py` and rank the rows where Linux loses.
4. Real workloads next to the synthetic ones: llama.cpp / MLX decode and prefill, stable-diffusion.cpp
   image time, a game benchmark (aurora-silicon/mesa#457 Cyberpunk tiers).

## Phase 3: close the largest gaps

Driven by phase 2: scheduling (compute vs compositor, as in #389), Borealis compiler and dispatch
performance, memory. Each fix is its own ticket with before/after data.

## Out of scope for now

A Metal API on Linux. If it becomes useful (Metal-only software, macOS guests), metal2vulkan and
reims-vgpu are the starting point; see [prior-art.md](prior-art.md).
