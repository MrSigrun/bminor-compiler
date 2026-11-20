#include "highlight/ast_highlighter.hpp"

#include <map>
#include <set>
#include <utility>

namespace bmfe {
namespace {

using CoordKey = std::pair<std::size_t, std::size_t>;

CoordKey makeCoord(const SourcePos& pos) {
    return {pos.line, pos.col};
}

/// Walks the AST to assign context-sensitive roles to identifier and construct positions.
class RoleCollector final : public AstVisitor {
public:
    std::map<CoordKey, StyleClass> roles;

    void visit(const ProgramUnit& node) override {
        for (const auto& decl : node.declarations) {
            visitNode(decl.get());
        }
    }

    void visit(const ScalarType& node) override {
        assign(node.pos, StyleClass::TypeName);
    }

    void visit(const VoidType& node) override {
        assign(node.pos, StyleClass::TypeName);
    }

    void visit(const ArrayType& node) override {
        assign(node.pos, StyleClass::Keyword);
        visitNode(node.lengthExpr.get());
        visitNode(node.elementType.get());
    }

    void visit(const ScalarInit& node) override {
        visitNode(node.value.get());
    }

    void visit(const BraceInit& node) override {
        for (const auto& item : node.items) {
            visitNode(item.get());
        }
    }

    void visit(const IntegerLit& node) override {
        assign(node.pos, StyleClass::IntLiteral);
    }

    void visit(const BooleanLit& node) override {
        assign(node.pos, StyleClass::BoolLiteral);
    }

    void visit(const CharacterLit& node) override {
        assign(node.pos, StyleClass::TextLiteral);
    }

    void visit(const StringLit& node) override {
        assign(node.pos, StyleClass::TextLiteral);
    }

    void visit(const VarRef& node) override {
        assign(node.pos, StyleClass::Variable);
    }

    void visit(const AssignOp& node) override {
        visitNode(node.lhs.get());
        visitNode(node.rhs.get());
    }

    void visit(const BinaryOp& node) override {
        visitNode(node.left.get());
        visitNode(node.right.get());
    }

    void visit(const UnaryOp& node) override {
        visitNode(node.operand.get());
    }

    void visit(const PostfixOp& node) override {
        visitNode(node.target.get());
    }

    void visit(const FuncCall& node) override {
        if (const auto* calleeIdent = dynamic_cast<const VarRef*>(node.callee.get())) {
            assign(calleeIdent->pos, StyleClass::Function);
        } else {
            visitNode(node.callee.get());
        }
        for (const auto& arg : node.args) {
            visitNode(arg.get());
        }
    }

    void visit(const SubscriptExpr& node) override {
        visitNode(node.base.get());
        visitNode(node.index.get());
    }

    void visit(const MalformedExpr&) override {}

    void visit(const ParamDecl& node) override {
        assign(node.namePos, StyleClass::Variable);
        visitNode(node.type.get());
    }

    void visit(const VariableDecl& node) override {
        assign(node.namePos, StyleClass::Variable);
        visitNode(node.type.get());
        visitNode(node.initializer.get());
    }

    void visit(const FunctionDecl& node) override {
        assign(node.namePos, StyleClass::Function);
        visitNode(node.returnType.get());
        for (const auto& param : node.params) {
            visitNode(param.get());
        }
        visitNode(node.body.get());
    }

    void visit(const CompoundStmt& node) override {
        for (const auto& stmt : node.statements) {
            visitNode(stmt.get());
        }
    }

    void visit(const IfElseStmt& node) override {
        assign(node.pos, StyleClass::Keyword);
        visitNode(node.condition.get());
        visitNode(node.thenStmt.get());
        visitNode(node.elseStmt.get());
    }

    void visit(const ForLoopStmt& node) override {
        assign(node.pos, StyleClass::Keyword);
        visitNode(node.initExpr.get());
        visitNode(node.condExpr.get());
        visitNode(node.stepExpr.get());
        visitNode(node.body.get());
    }

    void visit(const ReturnStmt& node) override {
        assign(node.pos, StyleClass::Keyword);
        visitNode(node.returnValue.get());
    }

    void visit(const PrintStmt& node) override {
        assign(node.pos, StyleClass::Keyword);
        for (const auto& item : node.items) {
            visitNode(item.get());
        }
    }

    void visit(const LocalDeclStmt& node) override {
        visitNode(node.varDecl.get());
    }

    void visit(const ExpressionStmt& node) override {
        visitNode(node.expr.get());
    }

    void visit(const MalformedStmt&) override {}

private:
    void assign(const SourcePos& pos, StyleClass style) {
        roles[makeCoord(pos)] = style;
    }

    void visitNode(const AstNode* node) {
        if (node != nullptr) {
            node->accept(*this);
        }
    }
};

StyleClass fallbackForToken(const LexemeToken& tok) {
    if (tok.kind == TokenKind::Whitespace || isDelimiterKind(tok.kind)) {
        return StyleClass::Plain;
    }
    if (tok.kind == TokenKind::Invalid) {
        return StyleClass::Error;
    }
    if (tok.kind == TokenKind::LineComment || tok.kind == TokenKind::BlockComment) {
        return StyleClass::Comment;
    }
    if (isScalarTypeKind(tok.kind) || tok.kind == TokenKind::KwVoid) {
        return StyleClass::TypeName;
    }
    if (isKeywordKind(tok.kind)) {
        return StyleClass::Keyword;
    }

    switch (tok.kind) {
        case TokenKind::IntLit:  return StyleClass::IntLiteral;
        case TokenKind::BoolLit: return StyleClass::BoolLiteral;
        case TokenKind::CharLit:
        case TokenKind::StrLit:  return StyleClass::TextLiteral;
        case TokenKind::Ident:   return StyleClass::Variable;
        case TokenKind::OpPlus:
        case TokenKind::OpMinus:
        case TokenKind::OpMul:
        case TokenKind::OpDiv:
        case TokenKind::OpMod:
        case TokenKind::OpPow:
        case TokenKind::OpInc:
        case TokenKind::OpDec:
        case TokenKind::OpNot:
        case TokenKind::OpLess:
        case TokenKind::OpLessEq:
        case TokenKind::OpGreater:
        case TokenKind::OpGreaterEq:
        case TokenKind::OpEq:
        case TokenKind::OpNotEq:
        case TokenKind::OpAnd:
        case TokenKind::OpOr:
        case TokenKind::OpAssign:
            return StyleClass::Operator;
        default:
            return StyleClass::Plain;
    }
}

} // namespace

std::vector<StyledToken> AstHighlighter::classify(const std::vector<LexemeToken>& fullStream,
                                                  const ProgramUnit* program,
                                                  const std::vector<DiagMessage>& errors) const {
    RoleCollector collector;
    if (program != nullptr) {
        program->accept(collector);
    }

    std::set<CoordKey> errCoords;
    for (const auto& err : errors) {
        errCoords.insert(makeCoord(err.pos));
    }

    std::vector<StyledToken> result;
    result.reserve(fullStream.size());

    for (const auto& tok : fullStream) {
        if (tok.kind == TokenKind::EndOfFile) {
            continue;
        }

        const CoordKey key = makeCoord(tok.pos);
        StyleClass style = StyleClass::Plain;

        if (tok.kind == TokenKind::Invalid ||
            (!isTriviaKind(tok.kind) && errCoords.find(key) != errCoords.end())) {
            style = StyleClass::Error;
        } else {
            const auto it = collector.roles.find(key);
            style = (it != collector.roles.end()) ? it->second : fallbackForToken(tok);
        }

        result.push_back(StyledToken{tok.text, style, tok.kind, tok.pos});
    }

    return result;
}

} // namespace bmfe
