# Worklog

Newest at the bottom. Each entry: what was run, on what, and where the data is.

## 2026-11-22: repo created

* Read the Aurora org (kernel, Mesa fork, gpubench, agent-guidelines), linux-aurora issues #25, #335, #387,
  #440, and the Metal-on-Vulkan projects. Results in [prior-art.md](prior-art.md).
* Found the `compute_preempt` selection logic in `drivers/gpu/drm/asahi/workqueue.rs`: automatic only for
  `chip_id == 0x6021`. t6020 is not covered.
* Nothing has been run on the machine for this repo yet.
