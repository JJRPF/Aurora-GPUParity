#!/bin/bash
# run-ramp.sh OUTDIR [STEPS_MS]
# Ramp the length of one compute dispatch (default 50 100 200 400 800 1500 3000 ms) with a second
# process timing a tiny dispatch every 16 ms (what the compositor needs). Stops at the first step
# that logs a "GPU timeout" or a wrong result, so one run does not pile up timeouts.
# Writes per step: dispatch JSON, latency JSON, the kernel lines logged during the step.
# Compare runs only on the same kernel, module parameters and Mesa; meta.txt records them.
set -u
PROBE=${PROBE:-$HOME/.cache/gpuparity/gpuprobe/gpuprobe}
OUT=${1:?outdir}; STEPS=${2:-"50 100 200 400 800 1500 3000"}; COUNT=${COUNT:-6}; INFLIGHT=${INFLIGHT:-2}
mkdir -p "$OUT"
timeouts_since() { journalctl -t kernel --no-pager --since "$1" -o cat 2>/dev/null | grep -c 'GPU timeout nya'; }
{ date -Is; uname -r; tr '\0' ' ' </proc/cmdline | sed -E 's/(root=UUID=|rd\.luks\.name=)[^ ]+/\1<redacted>/g'; echo
  pacman -Q linux-aurora mesa-aurora vulkan-aurora 2>/dev/null
  echo "compute_preempt/modprobe:"; grep -rhs 'asahi' /etc/modprobe.d/ /proc/cmdline 2>/dev/null | grep -E 'compute_preempt|asahi\.' | head
  echo "COUNT=$COUNT INFLIGHT=$INFLIGHT STEPS=$STEPS"; } > "$OUT/meta.txt"
for ms in $STEPS; do
  START=$(date '+%Y-%m-%d %H:%M:%S'); BEFORE=$(timeouts_since "$START")
  SECS=$(( (COUNT * ms * 2) / 1000 + 4 ))
  "$PROBE" latency --seconds "$SECS" > "$OUT/step-${ms}ms.latency.json" 2> "$OUT/step-${ms}ms.latency.err" &
  LAT=$!
  sleep 1
  "$PROBE" dispatch --ms "$ms" --count "$COUNT" --inflight "$INFLIGHT" > "$OUT/step-${ms}ms.dispatch.json" 2> "$OUT/step-${ms}ms.dispatch.err"; RC=$?
  wait $LAT
  sleep 1
  journalctl -t kernel --no-pager --since "$START" -o cat 2>/dev/null | grep -iE 'asahi|GPU|fault' > "$OUT/step-${ms}ms.kernel.txt"
  AFTER=$(timeouts_since "$START"); NEW=$((AFTER - BEFORE))
  echo "step ${ms}ms: dispatch rc=$RC, new GPU timeouts=$NEW, $(tr -d '\n' < "$OUT/step-${ms}ms.latency.json")"
  if [ "$NEW" -gt 0 ] || [ "$RC" -ne 0 ]; then echo "STOPPED at ${ms}ms (timeouts=$NEW rc=$RC)" | tee "$OUT/STOPPED"; break; fi
done
