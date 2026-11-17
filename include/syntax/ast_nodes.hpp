#ifndef BMFE_SYNTAX_AST_NODES_HPP
#define BMFE_SYNTAX_AST_NODES_HPP

#include "core/source_pos.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bmfe {

class AstVisitor;

/// Built-in scalar B-Minor types.
enum class ScalarKind {
    Integer,
    Boolean,
    Char,
    String
};

/// Returns the B-Minor keyword spelling for a scalar type.
std::string scalarKindName(ScalarKind kind);

/// Abstract base class for every node in the B-Minor AST.
class AstNode {
public:
    SourcePos pos;
    /// Populated during future semantic type checking; left empty in Phase 1.
    std::optional<std::string> resolvedType;

    explicit AstNode(SourcePos sourcePos) : pos(std::move(sourcePos)) {}
    virtual ~AstNode() = default;

    AstNode(const AstNode&) = delete;
    AstNode& operator=(const AstNode&) = delete;

    virtual void accept(AstVisitor& visitor) const = 0;
};

class TypeSpec : public AstNode {
public:
    using AstNode::AstNode;
};

class InitNode : public AstNode {
public:
    using AstNode::AstNode;
};

class Expr : public AstNode {
public:
    using AstNode::AstNode;
};

class Stmt : public AstNode {
public:
    using AstNode::AstNode;
};

class Decl : public AstNode {
public:
    using AstNode::AstNode;
};

// ---------------------------------------------------------------------------
// Type Nodes
// ---------------------------------------------------------------------------

class ScalarType final : public TypeSpec {
public:
    ScalarKind scalar;

    ScalarType(SourcePos sourcePos, ScalarKind kind)
        : TypeSpec(std::move(sourcePos)), scalar(kind) {}

    void accept(AstVisitor& visitor) const override;
};

class VoidType final : public TypeSpec {
public:
    explicit VoidType(SourcePos sourcePos) : TypeSpec(std::move(sourcePos)) {}

    void accept(AstVisitor& visitor) const override;
};

class ArrayType final : public TypeSpec {
public:
    std::unique_ptr<Expr> lengthExpr; // Null for unsized array parameters
    std::unique_ptr<TypeSpec> elementType;

    ArrayType(SourcePos sourcePos,
              std::unique_ptr<Expr> lenExpr,
              std::unique_ptr<TypeSpec> elemType)
        : TypeSpec(std::move(sourcePos)),
          lengthExpr(std::move(lenExpr)),
          elementType(std::move(elemType)) {}

    void accept(AstVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Initializer Nodes
// ---------------------------------------------------------------------------

class ScalarInit final : public InitNode {
public:
    std::unique_ptr<Expr> value;

    ScalarInit(SourcePos sourcePos, std::unique_ptr<Expr> expr)
        : InitNode(std::move(sourcePos)), value(std::move(expr)) {}

    void accept(AstVisitor& visitor) const override;
};

class BraceInit final : public InitNode {
public:
    std::vector<std::unique_ptr<InitNode>> items;

    BraceInit(SourcePos sourcePos, std::vector<std::unique_ptr<InitNode>> initItems)
        : InitNode(std::move(sourcePos)), items(std::move(initItems)) {}

    void accept(AstVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Leaf Expressions
// ---------------------------------------------------------------------------

class IntegerLit final : public Expr {
public:
    std::string rawText;
    long long numericValue = 0;
    bool outOfRange = false;

    IntegerLit(SourcePos sourcePos, std::string raw, long long val, bool overflow = false)
        : Expr(std::move(sourcePos)),
          rawText(std::move(raw)),
          numericValue(val),
          outOfRange(overflow) {}

    void accept(AstVisitor& visitor) const override;
};

class BooleanLit final : public Expr {
public:
    bool value = false;

    BooleanLit(SourcePos sourcePos, bool val)
        : Expr(std::move(sourcePos)), value(val) {}

    void accept(AstVisitor& visitor) const override;
};

class CharacterLit final : public Expr {
public:
    std::string rawText;
    char value = '\0';

    CharacterLit(SourcePos sourcePos, std::string raw, char val)
        : Expr(std::move(sourcePos)), rawText(std::move(raw)), value(val) {}

    void accept(AstVisitor& visitor) const override;
};

class StringLit final : public Expr {
public:
    std::string rawText;
    std::string decoded;

    StringLit(SourcePos sourcePos, std::string raw, std::string val)
        : Expr(std::move(sourcePos)), rawText(std::move(raw)), decoded(std::move(val)) {}

    void accept(AstVisitor& visitor) const override;
};

class VarRef final : public Expr {
public:
    std::string name;

    VarRef(SourcePos sourcePos, std::string identName)
        : Expr(std::move(sourcePos)), name(std::move(identName)) {}

    void accept(AstVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Internal Expressions
// ---------------------------------------------------------------------------

class AssignOp final : public Expr {
public:
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

    AssignOp(SourcePos sourcePos, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : Expr(std::move(sourcePos)), lhs(std::move(left)), rhs(std::move(right)) {}

    void accept(AstVisitor& visitor) const override;
};

class BinaryOp final : public Expr {
public:
    std::string op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryOp(SourcePos sourcePos,
             std::string oper,
             std::unique_ptr<Expr> lhs,
             std::unique_ptr<Expr> rhs)
        : Expr(std::move(sourcePos)),
          op(std::move(oper)),
          left(std::move(lhs)),
          right(std::move(rhs)) {}

    void accept(AstVisitor& visitor) const override;
};

class UnaryOp final : public Expr {
public:
    std::string op;
    std::unique_ptr<Expr> operand;

    UnaryOp(SourcePos sourcePos, std::string oper, std::unique_ptr<Expr> expr)
        : Expr(std::move(sourcePos)), op(std::move(oper)), operand(std::move(expr)) {}

    void accept(AstVisitor& visitor) const override;
};

class PostfixOp final : public Expr {
public:
    std::string op;
    std::unique_ptr<Expr> target;

    PostfixOp(SourcePos sourcePos, std::string oper, std::unique_ptr<Expr> expr)
        : Expr(std::move(sourcePos)), op(std::move(oper)), target(std::move(expr)) {}

    void accept(AstVisitor& visitor) const override;
};

class FuncCall final : public Expr {
public:
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;

    FuncCall(SourcePos sourcePos,
             std::unique_ptr<Expr> calleeExpr,
             std::vector<std::unique_ptr<Expr>> argList)
        : Expr(std::move(sourcePos)),
          callee(std::move(calleeExpr)),
          args(std::move(argList)) {}

    void accept(AstVisitor& visitor) const override;
};

class SubscriptExpr final : public Expr {
public:
    std::unique_ptr<Expr> base;
    std::unique_ptr<Expr> index;

    SubscriptExpr(SourcePos sourcePos, std::unique_ptr<Expr> baseExpr, std::unique_ptr<Expr> idxExpr)
        : Expr(std::move(sourcePos)), base(std::move(baseExpr)), index(std::move(idxExpr)) {}

    void accept(AstVisitor& visitor) const override;
};

/// Placeholder emitted during panic-mode expression recovery.
class MalformedExpr final : public Expr {
public:
    explicit MalformedExpr(SourcePos sourcePos) : Expr(std::move(sourcePos)) {}

    void accept(AstVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Declarations & Parameters
// ---------------------------------------------------------------------------

class ParamDecl final : public AstNode {
public:
    std::string name;
    SourcePos namePos;
    std::unique_ptr<TypeSpec> type;

    ParamDecl(SourcePos sourcePos,
              std::string paramName,
              SourcePos identPos,
              std::unique_ptr<TypeSpec> paramType)
        : AstNode(std::move(sourcePos)),
          name(std::move(paramName)),
          namePos(std::move(identPos)),
          type(std::move(paramType)) {}

    void accept(AstVisitor& visitor) const override;
};

class VariableDecl final : public Decl {
public:
    std::string name;
    SourcePos namePos;
    std::unique_ptr<TypeSpec> type;
    std::unique_ptr<InitNode> initializer; // Optional
    bool globalScope = false;

    VariableDecl(SourcePos sourcePos,
                 std::string varName,
                 SourcePos identPos,
                 std::unique_ptr<TypeSpec> varType,
                 std::unique_ptr<InitNode> init,
                 bool isGlobal)
        : Decl(std::move(sourcePos)),
          name(std::move(varName)),
          namePos(std::move(identPos)),
          type(std::move(varType)),
          initializer(std::move(init)),
          globalScope(isGlobal) {}

    void accept(AstVisitor& visitor) const override;
};

class CompoundStmt;

class FunctionDecl final : public Decl {
public:
    std::string name;
    SourcePos namePos;
    std::unique_ptr<TypeSpec> returnType;
    std::vector<std::unique_ptr<ParamDecl>> params;
    std::unique_ptr<CompoundStmt> body; // Null when prototype ends with ';'

    FunctionDecl(SourcePos sourcePos,
                 std::string funcName,
                 SourcePos identPos,
                 std::unique_ptr<TypeSpec> retType,
                 std::vector<std::unique_ptr<ParamDecl>> paramList,
                 std::unique_ptr<CompoundStmt> funcBody)
        : Decl(std::move(sourcePos)),
          name(std::move(funcName)),
          namePos(std::move(identPos)),
          returnType(std::move(retType)),
          params(std::move(paramList)),
          body(std::move(funcBody)) {}

    bool isPrototype() const { return body == nullptr; }

    void accept(AstVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Statements & Program Root
// ---------------------------------------------------------------------------

class CompoundStmt final : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    CompoundStmt(SourcePos sourcePos, std::vector<std::unique_ptr<Stmt>> stmts)
        : Stmt(std::move(sourcePos)), statements(std::move(stmts)) {}

    void accept(AstVisitor& visitor) const override;
};

class IfElseStmt final : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenStmt;
    std::unique_ptr<Stmt> elseStmt; // Optional

    IfElseStmt(SourcePos sourcePos,
               std::unique_ptr<Expr> cond,
               std::unique_ptr<Stmt> thenBranch,
               std::unique_ptr<Stmt> elseBranch)
        : Stmt(std::move(sourcePos)),
          condition(std::move(cond)),
          thenStmt(std::move(thenBranch)),
          elseStmt(std::move(elseBranch)) {}

    void accept(AstVisitor& visitor) const override;
};

class ForLoopStmt final : public Stmt {
public:
    std::unique_ptr<Expr> initExpr; // Optional
    std::unique_ptr<Expr> condExpr; // Optional
    std::unique_ptr<Expr> stepExpr; // Optional
    std::unique_ptr<Stmt> body;

    ForLoopStmt(SourcePos sourcePos,
                std::unique_ptr<Expr> init,
                std::unique_ptr<Expr> cond,
                std::unique_ptr<Expr> step,
                std::unique_ptr<Stmt> loopBody)
        : Stmt(std::move(sourcePos)),
          initExpr(std::move(init)),
          condExpr(std::move(cond)),
          stepExpr(std::move(step)),
          body(std::move(loopBody)) {}

    void accept(AstVisitor& visitor) const override;
};

class ReturnStmt final : public Stmt {
public:
    std::unique_ptr<Expr> returnValue; // Optional

    ReturnStmt(SourcePos sourcePos, std::unique_ptr<Expr> expr)
        : Stmt(std::move(sourcePos)), returnValue(std::move(expr)) {}

    void accept(AstVisitor& visitor) const override;
};

class PrintStmt final : public Stmt {
public:
    std::vector<std::unique_ptr<Expr>> items;

    PrintStmt(SourcePos sourcePos, std::vector<std::unique_ptr<Expr>> exprs)
        : Stmt(std::move(sourcePos)), items(std::move(exprs)) {}

    void accept(AstVisitor& visitor) const override;
};

class LocalDeclStmt final : public Stmt {
public:
    std::unique_ptr<VariableDecl> varDecl;

    LocalDeclStmt(SourcePos sourcePos, std::unique_ptr<VariableDecl> decl)
        : Stmt(std::move(sourcePos)), varDecl(std::move(decl)) {}

    void accept(AstVisitor& visitor) const override;
};

class ExpressionStmt final : public Stmt {
public:
    std::unique_ptr<Expr> expr;

    ExpressionStmt(SourcePos sourcePos, std::unique_ptr<Expr> expression)
        : Stmt(std::move(sourcePos)), expr(std::move(expression)) {}

    void accept(AstVisitor& visitor) const override;
};

/// Placeholder emitted during panic-mode statement recovery.
class MalformedStmt final : public Stmt {
public:
    explicit MalformedStmt(SourcePos sourcePos) : Stmt(std::move(sourcePos)) {}

    void accept(AstVisitor& visitor) const override;
};

class ProgramUnit final : public AstNode {
public:
    std::vector<std::unique_ptr<Decl>> declarations;

    ProgramUnit(SourcePos sourcePos, std::vector<std::unique_ptr<Decl>> decls)
        : AstNode(std::move(sourcePos)), declarations(std::move(decls)) {}

    void accept(AstVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Visitor & Pretty Printer
// ---------------------------------------------------------------------------

class AstVisitor {
public:
    virtual ~AstVisitor() = default;

    virtual void visit(const ProgramUnit& node) = 0;
    virtual void visit(const ScalarType& node) = 0;
    virtual void visit(const VoidType& node) = 0;
    virtual void visit(const ArrayType& node) = 0;
    virtual void visit(const ScalarInit& node) = 0;
    virtual void visit(const BraceInit& node) = 0;
    virtual void visit(const IntegerLit& node) = 0;
    virtual void visit(const BooleanLit& node) = 0;
    virtual void visit(const CharacterLit& node) = 0;
    virtual void visit(const StringLit& node) = 0;
    virtual void visit(const VarRef& node) = 0;
    virtual void visit(const AssignOp& node) = 0;
    virtual void visit(const BinaryOp& node) = 0;
    virtual void visit(const UnaryOp& node) = 0;
    virtual void visit(const PostfixOp& node) = 0;
    virtual void visit(const FuncCall& node) = 0;
    virtual void visit(const SubscriptExpr& node) = 0;
    virtual void visit(const MalformedExpr& node) = 0;
    virtual void visit(const ParamDecl& node) = 0;
    virtual void visit(const VariableDecl& node) = 0;
    virtual void visit(const FunctionDecl& node) = 0;
    virtual void visit(const CompoundStmt& node) = 0;
    virtual void visit(const IfElseStmt& node) = 0;
    virtual void visit(const ForLoopStmt& node) = 0;
    virtual void visit(const ReturnStmt& node) = 0;
    virtual void visit(const PrintStmt& node) = 0;
    virtual void visit(const LocalDeclStmt& node) = 0;
    virtual void visit(const ExpressionStmt& node) = 0;
    virtual void visit(const MalformedStmt& node) = 0;
};

/// Formats an AST subtree as an indented textual tree with source coordinates.
std::string printAst(const AstNode& root);

} // namespace bmfe

#endif // BMFE_SYNTAX_AST_NODES_HPP
