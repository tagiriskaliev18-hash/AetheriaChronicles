#!/usr/bin/env bash
# usage: unity-rc.sh [timeout-iterations]  -> triggers script recompile and reports status/errors
cd /d/AetheriaUnity || exit 1
unity --no-color command script recompile >/dev/null 2>&1
for i in $(seq 1 "${1:-50}"); do
  s=$(unity --no-color command script recompile_status 2>&1 | tr '\n' ' ')
  case "$s" in
    *completed*|*up_to_date*) echo "$s" | cut -c1-1800; exit 0;;
  esac
  sleep 3
done
echo "TIMEOUT: $s" | cut -c1-600
exit 1
