#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p work/evidence
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build build -j 10
killall -9 AudioComponentRegistrar 2>/dev/null || true
auval -v aufx Utow Rcwl | tee work/evidence/auval.txt
build/UndertowHostTest_artefacts/Release/UndertowHostTest | tee work/evidence/host.txt
render=build/UndertowRender_artefacts/Release/UndertowRender
for mode in --mutate --space --text --gestures --bank --ui --null --anchor --clicks --state --xy --pulse --sync --rt --cpu --tails --stability; do
    "$render" "$mode" | tee "work/evidence/${mode#--}.txt"
done
"$render" --presets work/default | tee work/evidence/presets.txt
