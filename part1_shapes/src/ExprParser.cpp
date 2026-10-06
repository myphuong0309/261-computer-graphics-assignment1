// =============================================================================
// ExprParser.cpp - recursive-descent parser building a small AST
// =============================================================================
#include "ExprParser.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

namespace {

struct Func1Def { const char* name; double (*f)(double); };
struct Func2Def { const char* name; double (*f)(double, double); };

const Func1Def FUNCS1[] = {
    {"sin", [](double v) { return std::sin(v); }},   {"cos", [](double v) { return std::cos(v); }},
    {"tan", [](double v) { return std::tan(v); }},   {"asin", [](double v) { return std::asin(v); }},
    {"acos", [](double v) { return std::acos(v); }}, {"atan", [](double v) { return std::atan(v); }},
    {"sinh", [](double v) { return std::sinh(v); }}, {"cosh", [](double v) { return std::cosh(v); }},
    {"tanh", [](double v) { return std::tanh(v); }}, {"sqrt", [](double v) { return std::sqrt(v); }},
    {"abs", [](double v) { return std::fabs(v); }},  {"exp", [](double v) { return std::exp(v); }},
    {"log", [](double v) { return std::log(v); }},   {"ln", [](double v) { return std::log(v); }},
    {"floor", [](double v) { return std::floor(v); }}, {"ceil", [](double v) { return std::ceil(v); }},
};
const Func2Def FUNCS2[] = {
    {"pow", [](double a, double b) { return std::pow(a, b); }},
    {"atan2", [](double a, double b) { return std::atan2(a, b); }},
    {"min", [](double a, double b) { return std::min(a, b); }},
    {"max", [](double a, double b) { return std::max(a, b); }},
};

} // namespace

// Parser state lives in a helper class so Expression's header stays small.
class ExprParserImpl {
public:
    ExprParserImpl(const std::string& s, std::vector<Expression::Node>& nodes) : s_(s), nodes_(nodes) {}

    int parse() {
        int r = expr();
        skipWs();
        if (pos_ < s_.size()) fail(std::string("unexpected '") + s_[pos_] + "'");
        return r;
    }

private:
    using Node = Expression::Node;

    [[noreturn]] void fail(const std::string& msg) const {
        throw std::runtime_error(msg + " (at position " + std::to_string(pos_ + 1) + ")");
    }
    void skipWs() { while (pos_ < s_.size() && std::isspace((unsigned char)s_[pos_])) ++pos_; }
    bool accept(char c) { skipWs(); if (pos_ < s_.size() && s_[pos_] == c) { ++pos_; return true; } return false; }
    char peek() { skipWs(); return pos_ < s_.size() ? s_[pos_] : '\0'; }

    int add(Node n) { nodes_.push_back(n); return (int)nodes_.size() - 1; }
    int binary(Node::Type t, int a, int b) { Node n; n.type = t; n.a = a; n.b = b; return add(n); }

    int expr() {
        int lhs = term();
        for (;;) {
            if      (accept('+')) lhs = binary(Node::Add, lhs, term());
            else if (accept('-')) lhs = binary(Node::Sub, lhs, term());
            else return lhs;
        }
    }

    int term() {
        int lhs = unary();
        for (;;) {
            char c = peek();
            if      (accept('*')) lhs = binary(Node::Mul, lhs, unary());
            else if (accept('/')) lhs = binary(Node::Div, lhs, unary());
            else if (std::isalpha((unsigned char)c) || c == '(') lhs = binary(Node::Mul, lhs, unary()); // 2x, 3sin(x), x(y+1)
            else return lhs;
        }
    }

    int unary() {
        if (accept('-')) { Node n; n.type = Node::Neg; n.a = unary(); return add(n); }
        if (accept('+')) return unary();
        return power();
    }

    int power() {
        int base = primary();
        if (accept('^')) return binary(Node::Pow, base, unary());
        return base;
    }

    int primary() {
        char c = peek();
        if (c == '(') {
            ++pos_;
            int e = expr();
            if (!accept(')')) fail("missing ')'");
            return e;
        }
        if (std::isdigit((unsigned char)c) || c == '.') {
            const char* start = s_.c_str() + pos_;
            char* end = nullptr;
            double v = std::strtod(start, &end);
            if (end == start) fail("bad number");
            pos_ += (size_t)(end - start);
            Node n; n.type = Node::Num; n.value = v;
            return add(n);
        }
        if (std::isalpha((unsigned char)c)) {
            size_t st = pos_;
            while (pos_ < s_.size() && (std::isalnum((unsigned char)s_[pos_]) || s_[pos_] == '_')) ++pos_;
            std::string id = s_.substr(st, pos_ - st);
            for (auto& ch : id) ch = (char)std::tolower((unsigned char)ch);
            return identifier(id);
        }
        if (c == '\0') fail("unexpected end of expression");
        fail(std::string("unexpected '") + c + "'");
    }

    int identifier(const std::string& id) {
        Node n;
        if (id == "x") { n.type = Node::VarX; return add(n); }
        if (id == "y") { n.type = Node::VarY; return add(n); }
        if (id == "pi") { n.type = Node::Num; n.value = 3.14159265358979323846; return add(n); }
        if (id == "e")  { n.type = Node::Num; n.value = 2.71828182845904523536; return add(n); }

        if (peek() == '(') {
            ++pos_;
            int a = expr();
            int b = -1;
            if (accept(',')) b = expr();
            if (!accept(')')) fail("missing ')' after arguments of " + id);

            for (int i = 0; i < (int)(sizeof(FUNCS1) / sizeof(FUNCS1[0])); ++i)
                if (id == FUNCS1[i].name) {
                    if (b >= 0) fail(id + "() takes one argument");
                    n.type = Node::Func1; n.fn = i; n.a = a; return add(n);
                }
            for (int i = 0; i < (int)(sizeof(FUNCS2) / sizeof(FUNCS2[0])); ++i)
                if (id == FUNCS2[i].name) {
                    if (b < 0) fail(id + "() takes two arguments");
                    n.type = Node::Func2; n.fn = i; n.a = a; n.b = b; return add(n);
                }
            fail("unknown function '" + id + "'");
        }
        fail("unknown name '" + id + "' (use x, y, pi, e)");
    }

    const std::string& s_;
    std::vector<Node>& nodes_;
    size_t pos_ = 0;
};

bool Expression::compile(const std::string& text, std::string& error) {
    std::string src = text;
    size_t eq = src.rfind('=');
    if (eq != std::string::npos) src = src.substr(eq + 1);   // accept "z = ..." / "f(x,y) = ..."

    std::vector<Node> nodes;
    try {
        ExprParserImpl p(src, nodes);
        int root = p.parse();
        nodes_ = std::move(nodes);
        root_  = root;
        error.clear();
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

double Expression::eval(double x, double y) const {
    return root_ >= 0 ? evalNode(root_, x, y) : 0.0;
}

double Expression::evalNode(int idx, double x, double y) const {
    const Node& n = nodes_[idx];
    switch (n.type) {
        case Node::Num:   return n.value;
        case Node::VarX:  return x;
        case Node::VarY:  return y;
        case Node::Neg:   return -evalNode(n.a, x, y);
        case Node::Add:   return evalNode(n.a, x, y) + evalNode(n.b, x, y);
        case Node::Sub:   return evalNode(n.a, x, y) - evalNode(n.b, x, y);
        case Node::Mul:   return evalNode(n.a, x, y) * evalNode(n.b, x, y);
        case Node::Div:   return evalNode(n.a, x, y) / evalNode(n.b, x, y);
        case Node::Pow:   return std::pow(evalNode(n.a, x, y), evalNode(n.b, x, y));
        case Node::Func1: return FUNCS1[n.fn].f(evalNode(n.a, x, y));
        case Node::Func2: return FUNCS2[n.fn].f(evalNode(n.a, x, y), evalNode(n.b, x, y));
    }
    return 0.0;
}
