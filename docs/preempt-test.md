# The compute-preemption test: how to run it, and why it has not run yet

Question: does `compute_preempt=1` stop the t6020 compute timeouts of aurora-silicon/linux#2701, and what
does it cost in throughput?

## Facts this plan rests on (all from this machine, 2026-11-22)

* `asahi` is a loadable module (`CONFIG_DRM_ASAHI=m`); `modinfo -p asahi` lists `compute_preempt`
  (default -1: automatic for the M2 Max chip id 0x6021, or with Debug0). It has no sysfs file, so it cannot be
  changed while the module is loaded. [data/20261122-baseline-identity/packages-drivers.txt](../data/20261122-baseline-identity/packages-drivers.txt)
* The module has 46 users (compositor and clients). Reloading it means stopping the whole graphical
  session. This Claude session runs in Ghostty under Hyprland, so it would end too.
* Passwordless `sudo` works.
* A normal boot with the option set in `/etc/modprobe.d/` is enough. The kernel command line is not needed.

## Ways to run it

| Way | What it takes | Cost and risk |
|---|---|---|
| A. Reboot with the option | `echo 'options asahi compute_preempt=1' > /etc/modprobe.d/zz-gpuparity.conf`, reboot, run `harness/gpuprobe/run-ramp.sh`, remove the file, reboot | Reboots the shared test Mac. DeepIdle's startup and resume hooks may fire on boot. Needs the S2R session's go-ahead. |
| B. Stop the session, reload the module, restart | A detached systemd unit stops `display-manager`, `rmmod asahi`, `modprobe asahi compute_preempt=1`, restarts the session | No reboot, but it kills the desktop and this session mid-run. The unit must write its results to `data/` by itself. |
| C. DeepIdle's initramfs environment | Boot the test environment with the option, run the probe without a compositor | Isolates the filesystem risk, but has no compositor, which is the other client in #2701. Good for the pure throughput cost. |

The no-preemption side of the A/B needs none of this: it is the current boot.

## Safety before any of A, B or C

1. Log the step in docs/worklog.md first, with the way back.
2. The S2R session agrees (see CLAUDE.md, "Coordination with the S2R work session").
3. The way back is deleting `/etc/modprobe.d/zz-gpuparity.conf` and rebooting. M2 Max reports say
   `compute_preempt=0` wedges that machine; the opposite setting on M2 Pro is untested, so a hung GPU
   is possible. The kernel recovers from single timeouts (116 on the #2701 boot) but a fail-stop needs a reboot.
4. Results are written to `data/` by the harness, not only to a terminal that may die.
