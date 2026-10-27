
// AtomData.cpp - CPK colors, radii, electron shell configurations
#include "AtomData.h"
#include <stdexcept>
#include <algorithm>
#include <map>

// hashmap of atomic properties (colors, radii, electron shells)
static const std::unordered_map<std::string, AtomInfo> s_atoms = {
    // symbol - name - Z - mass (u) - radius (Covalent radius, A) - color(R,G,B) - electron per shell (Bohr model)
    {"H",  {"H",  "Hydrogen",   1,  1.008f, 0.31f, {1.00f, 1.00f, 1.00f}, {1}}},
    {"He", {"He", "Helium",     2,  4.003f, 0.28f, {0.85f, 1.00f, 1.00f}, {2}}},
    {"Li", {"Li", "Lithium",    3,  6.940f, 1.28f, {0.80f, 0.50f, 1.00f}, {2,1}}},
    {"Be", {"Be", "Beryllium",  4,  9.012f, 0.96f, {0.76f, 1.00f, 0.00f}, {2,2}}},
    {"B",  {"B",  "Boron",      5, 10.810f, 0.84f, {1.00f, 0.71f, 0.71f}, {2,3}}},
    {"C",  {"C",  "Carbon",     6, 12.011f, 0.77f, {0.20f, 0.20f, 0.20f}, {2,4}}},
    {"N",  {"N",  "Nitrogen",   7, 14.007f, 0.75f, {0.19f, 0.31f, 0.97f}, {2,5}}},
    {"O",  {"O",  "Oxygen",     8, 15.999f, 0.73f, {1.00f, 0.05f, 0.05f}, {2,6}}},
    {"F",  {"F",  "Fluorine",   9, 18.998f, 0.64f, {0.56f, 0.88f, 0.31f}, {2,7}}},
    {"Ne", {"Ne", "Neon",      10, 20.180f, 0.58f, {0.70f, 0.89f, 0.96f}, {2,8}}},
    {"Na", {"Na", "Sodium",    11, 22.990f, 1.54f, {0.67f, 0.36f, 0.95f}, {2,8,1}}},
    {"Mg", {"Mg", "Magnesium", 12, 24.305f, 1.36f, {0.54f, 1.00f, 0.00f}, {2,8,2}}},
    {"Al", {"Al", "Aluminum",  13, 26.982f, 1.18f, {0.75f, 0.65f, 0.65f}, {2,8,3}}},
    {"Si", {"Si", "Silicon",   14, 28.085f, 1.11f, {0.94f, 0.78f, 0.63f}, {2,8,4}}},
    {"P",  {"P",  "Phosphorus",15, 30.974f, 1.06f, {1.00f, 0.50f, 0.00f}, {2,8,5}}},
    {"S",  {"S",  "Sulfur",    16, 32.060f, 1.02f, {1.00f, 1.00f, 0.19f}, {2,8,6}}},
    {"Cl", {"Cl", "Chlorine",  17, 35.450f, 0.99f, {0.12f, 0.94f, 0.12f}, {2,8,7}}},
    {"Ar", {"Ar", "Argon",     18, 39.948f, 0.96f, {0.50f, 0.82f, 0.89f}, {2,8,8}}},
    {"K",  {"K",  "Potassium", 19, 39.098f, 2.03f, {0.56f, 0.25f, 0.83f}, {2,8,8,1}}},
    {"Ca", {"Ca", "Calcium",   20, 40.078f, 1.74f, {0.24f, 1.00f, 0.00f}, {2,8,8,2}}},
    {"Fe", {"Fe", "Iron",      26, 55.845f, 1.25f, {0.88f, 0.40f, 0.20f}, {2,8,14,2}}},
    {"Cu", {"Cu", "Copper",    29, 63.546f, 1.28f, {0.78f, 0.50f, 0.20f}, {2,8,18,1}}},
    {"Zn", {"Zn", "Zinc",      30, 65.380f, 1.22f, {0.49f, 0.50f, 0.69f}, {2,8,18,2}}},
};

// Predefined molecules: experimental equilibrium geometries (Angstrom) and the experimental
// wavenumber of the symmetric stretching vibration (cm^-1)
static const std::vector<MoleculeSpec> s_molecules = {
    // ---- H2O (Water): r(OH) = 0.957 A, angle HOH = 104.5 deg ----
    {
        "Water", "H2O",
        {
            {"O", {0.000f,  0.000f, 0.000f}},
            {"H", {0.757f,  0.586f, 0.000f}},
            {"H", {-0.757f, 0.586f, 0.000f}},
        },
        {{0,1,1}, {0,2,1}},
        3657.0f
    },
    // ---- CO2 (Carbon dioxide): linear, r(CO) = 1.160 A ----
    {
        "Carbon Dioxide", "CO2",
        {
            {"C", { 0.000f, 0.000f, 0.000f}},
            {"O", { 1.160f, 0.000f, 0.000f}},
            {"O", {-1.160f, 0.000f, 0.000f}},
        },
        {{0,1,2}, {0,2,2}},
        1333.0f
    },
    // ---- NH3 (Ammonia): trigonal pyramid, r(NH) = 1.012 A, angle HNH = 106.7 deg ----
    {
        "Ammonia", "NH3",
        {
            {"N", { 0.0000f,  0.0000f,  0.0000f}},
            {"H", { 0.9373f, -0.3815f,  0.0000f}},
            {"H", {-0.4687f, -0.3815f,  0.8118f}},
            {"H", {-0.4687f, -0.3815f, -0.8118f}},
        },
        {{0,1,1}, {0,2,1}, {0,3,1}},
        3337.0f
    },
    // ---- CH4 (Methane): tetrahedron, r(CH) = 1.087 A, angle HCH = 109.47 deg ----
    {
        "Methane", "CH4",
        {
            {"C", { 0.000f,  0.000f,  0.000f}},
            {"H", { 0.628f,  0.628f,  0.628f}},
            {"H", {-0.628f, -0.628f,  0.628f}},
            {"H", {-0.628f,  0.628f, -0.628f}},
            {"H", { 0.628f, -0.628f, -0.628f}},
        },
        {{0,1,1}, {0,2,1}, {0,3,1}, {0,4,1}},
        2917.0f
    },
    // ---- O2 (Oxygen): r = 1.2075 A ----
    {
        "Oxygen", "O2",
        {
            {"O", { 0.6038f, 0.000f, 0.000f}},
            {"O", {-0.6038f, 0.000f, 0.000f}},
        },
        {{0,1,2}},
        1556.0f
    },
    // ---- N2 (Nitrogen): r = 1.0977 A ----
    {
        "Nitrogen", "N2",
        {
            {"N", { 0.5488f, 0.000f, 0.000f}},
            {"N", {-0.5488f, 0.000f, 0.000f}},
        },
        {{0,1,3}},
        2330.0f
    },
    // ---- HCl (Hydrogen chloride): r = 1.2746 A ----
    {
        "Hydrogen Chloride", "HCl",
        {
            {"H",  { 0.6373f, 0.000f, 0.000f}},
            {"Cl", {-0.6373f, 0.000f, 0.000f}},
        },
        {{0,1,1}},
        2886.0f
    },
    // ---- NaCl (Sodium chloride — ionic, shown as ball-and-stick): gas-phase monomer, r = 2.361 A ----
    {
        "Sodium Chloride", "NaCl",
        {
            {"Na", { 1.1805f, 0.000f, 0.000f}},
            {"Cl", {-1.1805f, 0.000f, 0.000f}},
        },
        {{0,1,1}},
        366.0f
    },
};

const AtomInfo& AtomData::get(const std::string& symbol) {
    auto it = s_atoms.find(symbol);
    if (it == s_atoms.end())
        throw std::runtime_error("Unknown element: " + symbol);
    return it->second;
}

const std::unordered_map<std::string, AtomInfo>& AtomData::all() {
    return s_atoms;
}

std::vector<std::string> AtomData::symbolsByZ() {
    std::vector<std::string> symbols;
    for (const auto& kv : s_atoms) symbols.push_back(kv.first);
    std::sort(symbols.begin(), symbols.end(), [](const std::string& a, const std::string& b) {
        return s_atoms.at(a).atomicNumber < s_atoms.at(b).atomicNumber;
    });
    return symbols;
}

const std::vector<MoleculeSpec>& MoleculeData::all() {
    return s_molecules;
}

const MoleculeSpec* MoleculeData::find(const std::string& formula) {
    for (const auto& m : s_molecules)
        if (m.formula == formula) return &m;
    return nullptr;
}

std::vector<BondSpec> MoleculeData::detectBonds(const std::vector<AtomSpec>& atoms, float tolerance) {
    std::vector<BondSpec> bonds;
    for (int i = 0; i < (int)atoms.size(); ++i)
        for (int j = i + 1; j < (int)atoms.size(); ++j) {
            float d    = glm::length(atoms[i].position - atoms[j].position);
            float dMax = tolerance * (AtomData::get(atoms[i].symbol).radius + AtomData::get(atoms[j].symbol).radius);
            if (d > 1e-3f && d <= dMax) bonds.push_back({i, j, 1});
        }
    return bonds;
}

std::string MoleculeData::hillFormula(const std::vector<AtomSpec>& atoms) {
    std::map<std::string, int> count;   // std::map keeps the symbols in alphabetical order
    for (const auto& a : atoms) ++count[a.symbol];

    std::string f;
    auto emit = [&](const std::string& sym) {
        auto it = count.find(sym);
        if (it == count.end()) return;
        f += sym;
        if (it->second > 1) f += std::to_string(it->second);
        count.erase(it);
    };
    if (count.count("C")) { emit("C"); emit("H"); }
    while (!count.empty()) emit(count.begin()->first);
    return f;
}
