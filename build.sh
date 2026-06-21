#!/usr/bin/env bash
set -euo pipefail
make project
./project --help >/dev/null
