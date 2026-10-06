// =============================================================================
// ModelLoader.cpp
// =============================================================================
#include "ModelLoader.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <tuple>

namespace {

const float PI = 3.14159265358979323846f;

std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// Centre + scale, generate whatever the file did not provide, build the edge list.
void finalize(MeshData& m, bool hasNormals, bool hasUV, bool hasColors) {
    MeshUtil::normalizeToUnit(m, 1.0f);
    if (!hasNormals) MeshUtil::computeSmoothNormals(m);
    if (!hasUV) {
        for (auto& v : m.vertices) {   // spherical projection around the centre
            glm::vec3 d = glm::normalize(v.position + glm::vec3(1e-6f));
            v.uv = glm::vec2(0.5f + std::atan2(d.z, d.x) / (2.0f * PI),
                             0.5f - std::asin(std::max(-1.0f, std::min(1.0f, d.y))) / PI);
        }
    }
    if (!hasColors) MeshUtil::assignVertexColors(m, MeshUtil::ColorScheme::Rainbow);
    MeshUtil::buildWireIndices(m);
}

} // namespace

namespace ModelLoader {

bool load(const std::string& path, MeshData& out, std::string& error) {
    std::string ext = lower(path.substr(path.find_last_of('.') == std::string::npos ? path.size() : path.find_last_of('.')));
    if (ext == ".obj") return loadOBJ(path, out, error);
    if (ext == ".ply") return loadPLY(path, out, error);
    error = "unsupported file type '" + ext + "' (expected .obj or .ply)";
    return false;
}

// -----------------------------------------------------------------------------
// OBJ
// -----------------------------------------------------------------------------
bool loadOBJ(const std::string& path, MeshData& out, std::string& error) {
    std::ifstream in(path);
    if (!in) { error = "cannot open " + path; return false; }

    std::vector<glm::vec3> positions, colors, normals;
    std::vector<glm::vec2> uvs;
    std::map<std::tuple<int, int, int>, unsigned> lookup;
    MeshData m;
    bool allNormals = true, anyUV = false, anyColor = false;
    std::string line;
    int lineNo = 0;

    auto resolve = [](int idx, size_t count) { return idx > 0 ? idx - 1 : (int)count + idx; };

    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ss(line);
        std::string tag;
        ss >> tag;
        if (tag == "v") {
            glm::vec3 p(0.0f), c(1.0f);
            ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
            if (ss >> c.x >> c.y >> c.z) anyColor = true;   // optional per-vertex colour extension
            colors.push_back(c);
        } else if (tag == "vt") {
            glm::vec2 t(0.0f);
            ss >> t.x >> t.y;
            uvs.push_back(t);
        } else if (tag == "vn") {
            glm::vec3 n(0.0f);
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (tag == "f") {
            std::vector<unsigned> face;
            std::string tok;
            while (ss >> tok) {
                int vi = 0, ti = 0, ni = 0;
                std::string parts[3];
                int k = 0;
                for (char ch : tok) { if (ch == '/') { if (++k > 2) break; } else parts[k] += ch; }
                if (parts[0].empty()) continue;
                vi = std::atoi(parts[0].c_str());
                if (!parts[1].empty()) ti = std::atoi(parts[1].c_str());
                if (!parts[2].empty()) ni = std::atoi(parts[2].c_str());

                int v = resolve(vi, positions.size());
                int t = ti ? resolve(ti, uvs.size())     : -1;
                int n = ni ? resolve(ni, normals.size()) : -1;
                if (v < 0 || v >= (int)positions.size() || t >= (int)uvs.size() || n >= (int)normals.size() ||
                    (ti && t < 0) || (ni && n < 0)) {
                    error = "line " + std::to_string(lineNo) + ": face index out of range";
                    return false;
                }
                auto key = std::make_tuple(v, t, n);
                auto it = lookup.find(key);
                if (it == lookup.end()) {
                    Vertex vert;
                    vert.position = positions[v];
                    vert.color    = colors[v];
                    if (t >= 0) { vert.uv = uvs[t]; anyUV = true; }
                    if (n >= 0) vert.normal = glm::normalize(normals[n]); else allNormals = false;
                    it = lookup.emplace(key, (unsigned)m.vertices.size()).first;
                    m.vertices.push_back(vert);
                }
                face.push_back(it->second);
            }
            for (size_t i = 1; i + 1 < face.size(); ++i) {   // fan triangulation
                m.indices.push_back(face[0]);
                m.indices.push_back(face[i]);
                m.indices.push_back(face[i + 1]);
            }
        }
    }
    if (m.indices.empty()) { error = "no faces found in " + path; return false; }
    finalize(m, allNormals, anyUV, anyColor);
    out = std::move(m);
    error.clear();
    return true;
}

// -----------------------------------------------------------------------------
// PLY (ascii, binary_little_endian, binary_big_endian)
// -----------------------------------------------------------------------------
namespace {

enum class PType { I8, U8, I16, U16, I32, U32, F32, F64, Invalid };

PType parseType(const std::string& s) {
    if (s == "char"   || s == "int8")    return PType::I8;
    if (s == "uchar"  || s == "uint8")   return PType::U8;
    if (s == "short"  || s == "int16")   return PType::I16;
    if (s == "ushort" || s == "uint16")  return PType::U16;
    if (s == "int"    || s == "int32")   return PType::I32;
    if (s == "uint"   || s == "uint32")  return PType::U32;
    if (s == "float"  || s == "float32") return PType::F32;
    if (s == "double" || s == "float64") return PType::F64;
    return PType::Invalid;
}

struct Prop    { std::string name; PType type = PType::F32; bool isList = false; PType countType = PType::U8; };
struct Element { std::string name; size_t count = 0; std::vector<Prop> props; };

class PlyReader {
public:
    PlyReader(std::istream& in, bool ascii, bool swapBytes) : in_(in), ascii_(ascii), swap_(swapBytes) {}

    bool read(PType t, double& v) {
        if (ascii_) { return (bool)(in_ >> v); }
        switch (t) {
            case PType::I8:  return rd<int8_t>(v);
            case PType::U8:  return rd<uint8_t>(v);
            case PType::I16: return rd<int16_t>(v);
            case PType::U16: return rd<uint16_t>(v);
            case PType::I32: return rd<int32_t>(v);
            case PType::U32: return rd<uint32_t>(v);
            case PType::F32: return rd<float>(v);
            case PType::F64: return rd<double>(v);
            default: return false;
        }
    }

private:
    template <class T> bool rd(double& v) {
        unsigned char buf[sizeof(T)];
        if (!in_.read((char*)buf, sizeof(T))) return false;
        if (swap_) std::reverse(buf, buf + sizeof(T));
        T val;
        std::memcpy(&val, buf, sizeof(T));
        v = (double)val;
        return true;
    }
    std::istream& in_;
    bool ascii_, swap_;
};

bool hostIsLittleEndian() { const uint16_t x = 1; return *(const unsigned char*)&x == 1; }

} // namespace

bool loadPLY(const std::string& path, MeshData& out, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) { error = "cannot open " + path; return false; }

    std::string line;
    std::getline(in, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "ply") { error = "not a PLY file"; return false; }

    bool ascii = true, bigEndian = false;
    std::vector<Element> elements;
    bool headerDone = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ss(line);
        std::string tag;
        ss >> tag;
        if (tag == "format") {
            std::string f; ss >> f;
            if      (f == "ascii")                ascii = true;
            else if (f == "binary_little_endian") { ascii = false; bigEndian = false; }
            else if (f == "binary_big_endian")    { ascii = false; bigEndian = true; }
            else { error = "unknown PLY format " + f; return false; }
        } else if (tag == "element") {
            Element e; ss >> e.name >> e.count;
            elements.push_back(e);
        } else if (tag == "property") {
            if (elements.empty()) { error = "property before element"; return false; }
            Prop p; std::string t; ss >> t;
            if (t == "list") {
                std::string ct, it;
                ss >> ct >> it >> p.name;
                p.isList = true; p.countType = parseType(ct); p.type = parseType(it);
                if (p.countType == PType::Invalid) { error = "bad list type"; return false; }
            } else {
                p.type = parseType(t);
                ss >> p.name;
            }
            if (p.type == PType::Invalid) { error = "unsupported property type in header"; return false; }
            elements.back().props.push_back(p);
        } else if (tag == "end_header") { headerDone = true; break; }
    }
    if (!headerDone) { error = "PLY header not terminated"; return false; }

    PlyReader reader(in, ascii, !ascii && (bigEndian == hostIsLittleEndian()));
    MeshData m;
    bool hasNormals = false, hasUV = false, hasColors = false;

    for (const Element& el : elements) {
        const bool isVertex = el.name == "vertex", isFace = el.name == "face";
        if (isVertex) m.vertices.assign(el.count, Vertex());

        for (size_t i = 0; i < el.count; ++i) {
            for (const Prop& p : el.props) {
                double v = 0.0;
                if (p.isList) {
                    double cnt = 0.0;
                    if (!reader.read(p.countType, cnt)) { error = "unexpected end of file"; return false; }
                    std::vector<unsigned> idx;
                    for (int k = 0; k < (int)cnt; ++k) {
                        if (!reader.read(p.type, v)) { error = "unexpected end of file"; return false; }
                        idx.push_back((unsigned)v);
                    }
                    if (isFace && (p.name == "vertex_indices" || p.name == "vertex_index")) {
                        for (unsigned id : idx)
                            if (id >= m.vertices.size()) { error = "face index out of range"; return false; }
                        for (size_t k = 1; k + 1 < idx.size(); ++k) {
                            m.indices.push_back(idx[0]);
                            m.indices.push_back(idx[k]);
                            m.indices.push_back(idx[k + 1]);
                        }
                    }
                    continue;
                }
                if (!reader.read(p.type, v)) { error = "unexpected end of file"; return false; }
                if (!isVertex) continue;

                Vertex& vt = m.vertices[i];
                const std::string& n = p.name;
                float f = (float)v;
                bool byteColor = (p.type == PType::U8);
                if      (n == "x")  vt.position.x = f;
                else if (n == "y")  vt.position.y = f;
                else if (n == "z")  vt.position.z = f;
                else if (n == "nx") { vt.normal.x = f; hasNormals = true; }
                else if (n == "ny") vt.normal.y = f;
                else if (n == "nz") vt.normal.z = f;
                else if (n == "s" || n == "u" || n == "texture_u" || n == "texture_s") { vt.uv.x = f; hasUV = true; }
                else if (n == "t" || n == "v" || n == "texture_v" || n == "texture_t") vt.uv.y = f;
                else if (n == "red")   { vt.color.r = byteColor ? f / 255.0f : f; hasColors = true; }
                else if (n == "green") vt.color.g = byteColor ? f / 255.0f : f;
                else if (n == "blue")  vt.color.b = byteColor ? f / 255.0f : f;
            }
        }
    }
    if (m.vertices.empty() || m.indices.empty()) { error = "PLY file has no triangles"; return false; }
    finalize(m, hasNormals, hasUV, hasColors);
    out = std::move(m);
    error.clear();
    return true;
}

} // namespace ModelLoader
