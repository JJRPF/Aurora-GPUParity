# Where the work stands (2026-11-22, start of the repo)

Machine: MacBook Pro 14" M2 Pro (T6020, `apple,j414s`, G14X, 32 GB), kernel `7.2.11-16-1-ARCH`
(`linux-aurora 7.2.11.aurora16`), Mesa `mesa-aurora` 26.3.0-devel (Borealis).

## Known

* **#2701 symptom** (filed 2026-11-21): FLUX.2 Klein 4B via ggml-vulkan gave `GPU timeout` events and
  `gCDM_CS` / `gCDM_ID` unmapped-read faults with magenta desktop corruption. ACE-Step 1.5 Turbo gave
  17 timeouts in 95 s with no fault info. 116 timeouts in one boot.
* **Compute preemption is not on for t6020.** `drivers/gpu/drm/asahi/workqueue.rs` selects it
  automatically only for `chip_id == 0x6021` (M2 Max) or with the Debug0 flag. `asahi.compute_preempt`
  (default -1) is a kernel command-line parameter with no sysfs entry, which is why
  `/sys/module/asahi/parameters/` shows none.
* **Without preemption the firmware cannot switch compute out** when another client needs the GPU, and on
  G14X it kills the work when the context-switch and kill timeouts expire (code comment in `workqueue.rs`).
* **M2 Max evidence** (linux-aurora#25): with preemption on, 0 timeouts and correct results beside an
  animated window; stock gave 39 to 120 timeouts per minute. `compute_preempt=0` wedged that machine.
  Throughput cost about 1% on M2 Max, 12 to 20% on M1 MoE decode.
* **No Metal baseline for t6020 exists** in [gpubench](https://github.com/aurora-silicon/gpubench).
  Published macOS records are Neo (A18 Pro) and M5 Pro only.

## Not yet known

* Whether `asahi.compute_preempt=1` fixes #2701 on t6020 (untested).
* The M2 Pro throughput cost of preemption.
* How far Borealis is from Metal on this chip, per workload.
* Whether the #335 kill/teardown fault, fixed on M1 in kernel 7.3.0-2-20261110.1, also applies to t6020.

## Next

See [plan.md](plan.md).
