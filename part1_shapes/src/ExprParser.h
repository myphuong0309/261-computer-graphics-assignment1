// =============================================================================
// ExprParser.h - tiny expression compiler for user-provided z = f(x, y)
//
// Grammar (case-insensitive):
//   expr    := term (('+' | '-') term)*
//   term    := unary (('*' | '/') unary | implicit-multiply)*
//   unary   := ('-' | '+') unary | power
//   power   := primary ('^' unary)?                  (right associative, -x^2 == -(x^2))
//   primary := number | x | y | pi | e | name '(' expr (',' expr)* ')' | '(' expr ')'
// Functions: sin cos tan asin acos atan sinh cosh tanh sqrt abs exp log ln floor ceil
//            pow(a,b) atan2(a,b) min(a,b) max(a,b)
// A leading "z =" / "f(x,y) =" is ignored.
// =============================================================================
#pragma once
#include <string>
#include <vector>

class Expression {
public:
    // Returns true on success; otherwise `error` describes the problem.
    bool compile(const std::string& text, std::string& error);

    double eval(double x, double y) const;
    bool   valid() const { return root_ >= 0; }

private:
    struct Node {
        enum Type { Num, VarX, VarY, Neg, Add, Sub, Mul, Div, Pow, Func1, Func2 } type = Num;
        double value = 0.0;
        int    a = -1, b = -1;   // child node indices
        int    fn = -1;          // function table index
    };

    double evalNode(int idx, double x, double y) const;

    std::vector<Node> nodes_;
    int root_ = -1;
    friend class ExprParserImpl;
};
