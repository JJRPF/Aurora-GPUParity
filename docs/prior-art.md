# Prior art (researched 2026-11-22)

Everything here was read from the public repositories on that date. Status words ("beta", "active") are
the projects' own or the repository's last-push date; none of it has been run on t6020 yet.

## Aurora Silicon

| Item | What it tells us |
|---|---|
| [linux#2701](https://github.com/aurora-silicon/linux/issues/2701) | Our issue. j414s timeouts under long Vulkan compute. Open. |
| [omacom/linux-aurora#25](https://github.com/omacom/linux-aurora/issues/25) | G14X compute preemption: automatic on M2 Max, opt-in on Pro/Ultra pending qualification. Completion criteria listed there. Two M2 Max owners reported 0 timeouts with it. |
| [omacom/linux-aurora#335](https://github.com/omacom/linux-aurora/issues/335) | Killing a process with two queued compute submissions faults (unmapped read) and times out. Not reproduced in 60 runs on an M1 with kernel 7.3.0-2-20261110.1. |
| [omacom/linux-aurora#387](https://github.com/omacom/linux-aurora/issues/387), [#389](https://github.com/omacom/linux-aurora/pull/389) | G15 (M3 Pro): long compute submissions delay the compositor 40 to 200 ms; HIGH priority has no effect. #389 caps compute batch time while another VM waits. Different backend than G14X. |
| [omacom/linux-aurora#440](https://github.com/omacom/linux-aurora/issues/440) | G15C: llama.cpp prefill trips the compute deadline and the GPU fail-stops until reboot. |
| [gpubench](https://github.com/aurora-silicon/gpubench) | Matched Vulkan and Metal benchmarks, a stress suite, and `tools/performance-gate.py` (Linux must beat the macOS reference on every comparable row). |
| Mesa fork branches | `agx3b-opt`, `djdev/agx3-*`, `djdev/g16*`, `djdev/g17*`, `coopmat/agx3b-cmat`. Mostly newer GPUs; none checked against G14X yet. |
| [agent-guidelines](https://github.com/aurora-silicon/agent-guidelines) | Hard rules: fix the cause, default command line only, no chip/board/firmware gates. RE output stays private; published code is clean-room. |

## Metal on Vulkan (outside Aurora)

| Project | What it is | Relevance |
|---|---|---|
| [steelbrain/metal2vulkan](https://github.com/steelbrain/metal2vulkan) | AIR (metallib) to SPIR-V translator plus a Metal API runtime on Vulkan 1.2+. Beta, Rust. | Its docs already list "Honeykrisp on Apple's M2 Pro" as lacking rasterization-order attachment access and pixel interlock. |
| [steelbrain/reims-vgpu](https://github.com/steelbrain/reims-vgpu) | Virtual GPU for macOS guests in QEMU/KVM on Vulkan. | A macOS 26 guest rendered on an M1 Max under KVM on Honeykrisp (metal2vulkan#12). |
| [darlinghq/indium](https://github.com/darlinghq/indium) | Metal-shaped C++ API on Vulkan 1.3, for Darling. | Ran a native Metal app on Linux; no depth buffer or texture sampling at the time. |

Reported by metal2vulkan's docs, worth checking in our kernel: the asahi DRM driver keeps freed dma-buf
imports and their pages past process exit (reproducer in `scripts/dma-buf-leak/` of that repo).

## Not found

* No Metal API or Metal driver for Apple GPUs on Linux in any Aurora repository.
* No gpubench record for t6020, on macOS or Linux.
* No M2 Pro result for `compute_preempt`.
