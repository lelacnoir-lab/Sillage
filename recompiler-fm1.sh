#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
: "${JIELI_TOOLCHAIN:?Set JIELI_TOOLCHAIN to the Linux x86-64 JieLi toolchain directory}"
export JIELI_TOOLCHAIN SILLAGE_SAMPLE_RATE=13230 FELUCCA_CDC=0 JIELI_DOCKER=0
mkdir -p build/gen
cp generated/*.h build/gen/
python3 - <<'PY'
import os, sys
sys.path.insert(0, 'tools')
import build
build.generate = lambda: print('Using the exact generated headers from Sillage V3 FM-1_946301')
build.BOARD_PRODUCT['fm1'] = 'FM-1_946301'
sys.argv = ['build.py', '--board', 'fm1', '--sdk', os.environ.get('AC79_SDK', 'sdk')]
sys.exit(build.main())
PY
