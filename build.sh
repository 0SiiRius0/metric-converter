#!/usr/bin/env bash
# Builds the site into ./site (needs the Emscripten SDK: https://emscripten.org)
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p site
cp web/index.html site/
emcc src/converter.cpp -O2 -std=c++17 --no-entry \
  -sMODULARIZE=1 -sEXPORT_NAME=createConverter -sENVIRONMENT=web -sSINGLE_FILE=1 \
  -sEXPORTED_FUNCTIONS=_catalog_json,_convert -sEXPORTED_RUNTIME_METHODS=cwrap \
  -o site/converter.js
echo "Built site/. Preview with: python3 -m http.server -d site"
