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
    float        radius;          // CPK radius (scaled for visualization)
    glm::vec3    color;           // CPK color
    ShellConfig  shells;          // electrons per shell (for Bohr model)
};

class AtomData {
public:
    static const AtomInfo& get(const std::string& symbol);
    static const std::unordered_map<std::string, AtomInfo>& all();
};


// // MoleculeData.h - Predefined molecule descriptions


struct AtomSpec {
    std::string  symbol;
    glm::vec3    position;        // Angstrom-scale, will be auto-scaled
};

struct BondSpec {
    int a, b;                     // indices into atoms array
    int order;                    // 1 = single, 2 = double
};

struct MoleculeSpec {
    std::string             name;
    std::string             formula;
    std::vector<AtomSpec>   atoms;
    std::vector<BondSpec>   bonds;
};

class MoleculeData {
public:
    static const std::vector<MoleculeSpec>& all();
    static const MoleculeSpec* find(const std::string& formula);
};
