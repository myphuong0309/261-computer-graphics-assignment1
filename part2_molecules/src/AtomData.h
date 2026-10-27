// AtomData.h - Atomic properties table (CPK color scheme + Bohr shells)
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

// Electron shell configuration (number of electrons per shell)
using ShellConfig = std::vector<int>;

struct AtomInfo {
    std::string  symbol;
    std::string  name;
    int          atomicNumber;      // Z
    float        mass;            // standard atomic weight (u), for the centre of mass
    float        radius;          // covalent radius (Angstrom)
    glm::vec3    color;           // CPK color
    ShellConfig  shells;          // electrons per shell (for Bohr model)
};

class AtomData {
public:
    static const AtomInfo& get(const std::string& symbol);
    static const std::unordered_map<std::string, AtomInfo>& all();
    static std::vector<std::string> symbolsByZ();   // all symbols sorted by atomic number
};


// // MoleculeData.h - Predefined molecule descriptions


struct AtomSpec {
    std::string  symbol;
    glm::vec3    position;        // Angstrom-scale, will be auto-scaled
};

struct BondSpec {
    int a, b;                     // indices into atoms array
    int order;                    // 1 = single, 2 = double, 3 = triple
};

struct MoleculeSpec {
    std::string             name;
    std::string             formula;
    std::vector<AtomSpec>   atoms;
    std::vector<BondSpec>   bonds;
    float                   stretchWavenumber = 0.0f;  // symmetric stretch (cm^-1, experimental), 0 = unknown
};

class MoleculeData {
public:
    static const std::vector<MoleculeSpec>& all();
    static const MoleculeSpec* find(const std::string& formula);

    // Single bonds between every pair closer than tolerance * (sum of covalent radii)
    static std::vector<BondSpec> detectBonds(const std::vector<AtomSpec>& atoms, float tolerance = 1.2f);

    // Hill-system formula of a list of atoms ("C" first, then "H", then alphabetical), e.g. "CH4", "H2O"
    static std::string hillFormula(const std::vector<AtomSpec>& atoms);
};
