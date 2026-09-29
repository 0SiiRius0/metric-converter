// Metric unit conversion engine. Compiled to WebAssembly with Emscripten.
// Native build for testing: g++ -std=c++17 -DTEST src/converter.cpp && ./a.out
#include <cmath>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define EXPORT extern "C" EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT extern "C"
#endif

struct Prefix { const char *name, *sym; int exp; };

struct Unit {
    const char* name;
    const char* sym;
    long double factor;  // multiplier to the category's SI base unit
    bool prefixable;     // accepts SI prefixes (km, mL, MW ...)
    int power;           // prefix exponent multiplier: 2 for m^2, 3 for m^3
    long double offset;  // added after scaling (degree Celsius)
};

struct Category { const char* name; std::vector<Unit> units; };

static const std::vector<Prefix> PREFIXES = {
    {"quetta", "Q", 30}, {"ronna", "R", 27}, {"yotta", "Y", 24}, {"zetta", "Z", 21},
    {"exa", "E", 18},    {"peta", "P", 15},  {"tera", "T", 12},  {"giga", "G", 9},
    {"mega", "M", 6},    {"kilo", "k", 3},   {"hecto", "h", 2},  {"deca", "da", 1},
    {"none", "", 0},     {"deci", "d", -1},  {"centi", "c", -2}, {"milli", "m", -3},
    {"micro", "µ", -6},  {"nano", "n", -9},  {"pico", "p", -12}, {"femto", "f", -15},
    {"atto", "a", -18},  {"zepto", "z", -21}, {"yocto", "y", -24}, {"ronto", "r", -27},
    {"quecto", "q", -30}};

#define SI(n, s) Unit{n, s, 1.0L, true, 1, 0.0L}

static const std::vector<Category> CATS = {
    {"Length", {SI("metre", "m")}},
    {"Mass", {Unit{"gram", "g", 1e-3L, true, 1, 0}, Unit{"tonne", "t", 1e3L, false, 1, 0}}},
    {"Time", {SI("second", "s"), Unit{"minute", "min", 60, false, 1, 0},
              Unit{"hour", "h", 3600, false, 1, 0}, Unit{"day", "d", 86400, false, 1, 0}}},
    {"Area", {Unit{"square metre", "m²", 1, true, 2, 0}, Unit{"hectare", "ha", 1e4L, false, 1, 0}}},
    {"Volume", {Unit{"cubic metre", "m³", 1, true, 3, 0}, Unit{"litre", "L", 1e-3L, true, 1, 0}}},
    {"Speed", {SI("metre per second", "m/s"), Unit{"kilometre per hour", "km/h", 1.0L / 3.6L, false, 1, 0}}},
    {"Force", {SI("newton", "N")}},
    {"Pressure", {SI("pascal", "Pa"), Unit{"bar", "bar", 1e5L, true, 1, 0}}},
    {"Energy", {SI("joule", "J"), Unit{"watt-hour", "Wh", 3600, true, 1, 0},
                Unit{"electronvolt", "eV", 1.602176634e-19L, true, 1, 0}}},
    {"Power", {SI("watt", "W")}},
    {"Frequency", {SI("hertz", "Hz")}},
    {"Electric current", {SI("ampere", "A")}},
    {"Voltage", {SI("volt", "V")}},
    {"Resistance", {SI("ohm", "Ω")}},
    {"Conductance", {SI("siemens", "S")}},
    {"Capacitance", {SI("farad", "F")}},
    {"Inductance", {SI("henry", "H")}},
    {"Charge", {SI("coulomb", "C"), Unit{"ampere-hour", "Ah", 3600, true, 1, 0}}},
    {"Magnetic flux density", {SI("tesla", "T"), Unit{"gauss", "G", 1e-4L, true, 1, 0}}},
    {"Temperature", {SI("kelvin", "K"), Unit{"degree Celsius", "°C", 1, false, 1, 273.15L}}},
    {"Amount of substance", {SI("mole", "mol")}},
    {"Luminous intensity", {SI("candela", "cd")}},
    {"Luminous flux", {SI("lumen", "lm")}},
    {"Illuminance", {SI("lux", "lx")}},
    {"Radioactivity", {SI("becquerel", "Bq")}},
    {"Radiation dose", {SI("gray", "Gy"), SI("sievert", "Sv")}},
    {"Catalytic activity", {SI("katal", "kat")}},
};

static long double scale(const Unit& u, int prefix) {
    long double f = u.factor;
    if (u.prefixable && prefix >= 0 && prefix < (int)PREFIXES.size())
        f *= std::pow(10.0L, (long double)(PREFIXES[prefix].exp * u.power));
    return f;
}

// Catalog of prefixes, categories and units as JSON, for building the UI.
EXPORT const char* catalog_json() {
    static std::string s;
    if (!s.empty()) return s.c_str();
    s = "{\"prefixes\":[";
    for (size_t i = 0; i < PREFIXES.size(); ++i) {
        const Prefix& p = PREFIXES[i];
        s += std::string(i ? "," : "") + "{\"n\":\"" + p.name + "\",\"s\":\"" + p.sym +
             "\",\"e\":" + std::to_string(p.exp) + "}";
    }
    s += "],\"categories\":[";
    for (size_t i = 0; i < CATS.size(); ++i) {
        s += std::string(i ? "," : "") + "{\"name\":\"" + CATS[i].name + "\",\"units\":[";
        for (size_t j = 0; j < CATS[i].units.size(); ++j) {
            const Unit& u = CATS[i].units[j];
            s += std::string(j ? "," : "") + "{\"n\":\"" + u.name + "\",\"s\":\"" + u.sym +
                 "\",\"p\":" + (u.prefixable ? "1" : "0") + "}";
        }
        s += "]}";
    }
    s += "]}";
    return s.c_str();
}

// Converts v from (from, fromPrefix) to (to, toPrefix) inside category cat.
// Returns NaN on invalid indices. Prefixes are ignored for non-prefixable units.
EXPORT double convert(int cat, int from, int fromPrefix, int to, int toPrefix, double v) {
    if (cat < 0 || cat >= (int)CATS.size()) return NAN;
    const auto& u = CATS[cat].units;
    if (from < 0 || from >= (int)u.size() || to < 0 || to >= (int)u.size()) return NAN;
    long double base = (long double)v * scale(u[from], fromPrefix) + u[from].offset;
    return (double)((base - u[to].offset) / scale(u[to], toPrefix));
}

#ifdef TEST
#include <cstdio>
int main() {
    std::printf("1 km -> m      : %.10g\n", convert(0, 0, 9, 0, 12, 1));
    std::printf("1 km2 -> m2    : %.10g\n", convert(3, 0, 9, 0, 12, 1));
    std::printf("1 L -> cm3     : %.10g\n", convert(4, 1, 12, 0, 14, 1));
    std::printf("100 C -> K     : %.10g\n", convert(19, 1, 12, 0, 12, 100));
    std::printf("1 kWh -> MJ    : %.10g\n", convert(8, 1, 9, 0, 8, 1));
    std::printf("catalog bytes  : %zu\n", std::string(catalog_json()).size());
}
#endif
