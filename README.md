# Metric converter

Unit converter for the SI unit families and all 25 SI prefixes (quecto to quetta).
The engine is C++ (`src/converter.cpp`), compiled to WebAssembly with Emscripten.
`web/index.html` is the UI.

## Deploy
1. Push this repo to GitHub (branch `main`).
2. In the repo, open Settings > Pages and set Source to **GitHub Actions**.
3. Every push builds the WASM and publishes the site.

## Build locally
Install the [Emscripten SDK](https://emscripten.org), then run `./build.sh`
and open `site/index.html` (or `python3 -m http.server -d site`).

## Test the engine natively
`g++ -std=c++17 -DTEST src/converter.cpp -o t && ./t`

## Add a unit
Add a `Unit{name, symbol, factor to SI base, prefixable, power, offset}` to a category in `CATS`.
