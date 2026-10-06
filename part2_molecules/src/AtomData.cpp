
// AtomData.cpp - CPK colors, radii, electron shell configurations
#include "AtomData.h"
#include <stdexcept>

// hashmap of atomic properties (colors, radii, electron shells)
static const std::unordered_map<std::string, AtomInfo> s_atoms = {
    // symbol - name - Z - radius (Covalent radius) - color(R,G,B) - electron per shell (Bohr model)
    {"H",  {"H",  "Hydrogen",   1, 0.31f, {1.00f, 1.00f, 1.00f}, {1}}},
    {"He", {"He", "Helium",     2, 0.28f, {0.85f, 1.00f, 1.00f}, {2}}},
    {"Li", {"Li", "Lithium",    3, 1.28f, {0.80f, 0.50f, 1.00f}, {2,1}}},
    {"Be", {"Be", "Beryllium",  4, 0.96f, {0.76f, 1.00f, 0.00f}, {2,2}}},
    {"B",  {"B",  "Boron",      5, 0.84f, {1.00f, 0.71f, 0.71f}, {2,3}}},
    {"C",  {"C",  "Carbon",     6, 0.77f, {0.20f, 0.20f, 0.20f}, {2,4}}},
    {"N",  {"N",  "Nitrogen",   7, 0.75f, {0.19f, 0.31f, 0.97f}, {2,5}}},
    {"O",  {"O",  "Oxygen",     8, 0.73f, {1.00f, 0.05f, 0.05f}, {2,6}}},
    {"F",  {"F",  "Fluorine",   9, 0.64f, {0.56f, 0.88f, 0.31f}, {2,7}}},
    {"Ne", {"Ne", "Neon",      10, 0.58f, {0.70f, 0.89f, 0.96f}, {2,8}}},
    {"Na", {"Na", "Sodium",    11, 1.54f, {0.67f, 0.36f, 0.95f}, {2,8,1}}},
    {"Mg", {"Mg", "Magnesium", 12, 1.36f, {0.54f, 1.00f, 0.00f}, {2,8,2}}},
    {"Al", {"Al", "Aluminum",  13, 1.18f, {0.75f, 0.65f, 0.65f}, {2,8,3}}},
    {"Si", {"Si", "Silicon",   14, 1.11f, {0.94f, 0.78f, 0.63f}, {2,8,4}}},
    {"P",  {"P",  "Phosphorus",15, 1.06f, {1.00f, 0.50f, 0.00f}, {2,8,5}}},
    {"S",  {"S",  "Sulfur",    16, 1.02f, {1.00f, 1.00f, 0.19f}, {2,8,6}}},
    {"Cl", {"Cl", "Chlorine",  17, 0.99f, {0.12f, 0.94f, 0.12f}, {2,8,7}}},
    {"Ar", {"Ar", "Argon",     18, 0.96f, {0.50f, 0.82f, 0.89f}, {2,8,8}}},
    {"K",  {"K",  "Potassium", 19, 2.03f, {0.56f, 0.25f, 0.83f}, {2,8,8,1}}},
    {"Ca", {"Ca", "Calcium",   20, 1.74f, {0.24f, 1.00f, 0.00f}, {2,8,8,2}}},
    {"Fe", {"Fe", "Iron",      26, 1.25f, {0.88f, 0.40f, 0.20f}, {2,8,14,2}}},
    {"Cu", {"Cu", "Copper",    29, 1.28f, {0.78f, 0.50f, 0.20f}, {2,8,18,1}}},
    {"Zn", {"Zn", "Zinc",      30, 1.22f, {0.49f, 0.50f, 0.69f}, {2,8,18,2}}},
};

// Predefined molecules
static const std::vector<MoleculeSpec> s_molecules = {
    // ---- H2O (Water) ----
    {
        "Water", "H2O",
        {
            {"O", {0.000f,  0.000f, 0.000f}},
            {"H", {0.757f,  0.586f, 0.000f}},
            {"H", {-0.757f, 0.586f, 0.000f}},
        },
        {{0,1,1}, {0,2,1}}
    },
    // ---- CO2 (Carbon dioxide) ----
    {
        "Carbon Dioxide", "CO2",
        {
            {"C", { 0.000f, 0.000f, 0.000f}},
            {"O", { 1.163f, 0.000f, 0.000f}},
            {"O", {-1.163f, 0.000f, 0.000f}},
        },
        {{0,1,2}, {0,2,2}}
    },
    // ---- NH3 (Ammonia) ----
    {
        "Ammonia", "NH3",
        {
            {"N", { 0.000f,  0.000f,  0.000f}},
            {"H", { 0.939f, -0.265f, -0.384f}},
            {"H", {-0.939f, -0.265f, -0.384f}},
            {"H", { 0.000f,  0.530f, -0.768f}},
        },
        {{0,1,1}, {0,2,1}, {0,3,1}}
    },
    // ---- CH4 (Methane) ----
    {
        "Methane", "CH4",
        {
            {"C", { 0.000f,  0.000f,  0.000f}},
            {"H", { 0.630f,  0.630f,  0.630f}},
            {"H", {-0.630f, -0.630f,  0.630f}},
            {"H", {-0.630f,  0.630f, -0.630f}},
            {"H", { 0.630f, -0.630f, -0.630f}},
        },
        {{0,1,1}, {0,2,1}, {0,3,1}, {0,4,1}}
    },
    // ---- O2 (Oxygen) ----
    {
        "Oxygen", "O2",
        {
            {"O", { 0.600f, 0.000f, 0.000f}},
            {"O", {-0.600f, 0.000f, 0.000f}},
        },
        {{0,1,2}}
    },
    // ---- N2 (Nitrogen) ----
    {
        "Nitrogen", "N2",
        {
            {"N", { 0.548f, 0.000f, 0.000f}},
            {"N", {-0.548f, 0.000f, 0.000f}},
        },
        {{0,1,3}}
    },
    // ---- HCl (Hydrogen chloride) ----
    {
        "Hydrogen Chloride", "HCl",
        {
            {"H",  { 0.628f, 0.000f, 0.000f}},
            {"Cl", {-0.628f, 0.000f, 0.000f}},
        },
        {{0,1,1}}
    },
    // ---- NaCl (Sodium chloride — ionic, shown as ball-and-stick) ----
    {
        "Sodium Chloride", "NaCl",
        {
            {"Na", { 1.0f, 0.000f, 0.000f}},
            {"Cl", {-1.0f, 0.000f, 0.000f}},
        },
        {{0,1,1}}
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

const std::vector<MoleculeSpec>& MoleculeData::all() {
    return s_molecules;
}

const MoleculeSpec* MoleculeData::find(const std::string& formula) {
    for (const auto& m : s_molecules)
        if (m.formula == formula) return &m;
    return nullptr;
}
