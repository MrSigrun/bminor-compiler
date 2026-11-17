#include "syntax/ast_nodes.hpp"

#include <sstream>

namespace bmfe {

std::string scalarKindName(ScalarKind kind) {
    switch (kind) {
        case ScalarKind::Integer: return "integer";
        case ScalarKind::Boolean: return "boolean";
        case ScalarKind::Char:    return "char";
        case ScalarKind::String:  return "string";
    }
    return "unknown";
}

void ScalarType::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void VoidType::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void ArrayType::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void ScalarInit::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void BraceInit::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void IntegerLit::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void BooleanLit::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void CharacterLit::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void StringLit::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void VarRef::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void AssignOp::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void BinaryOp::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void UnaryOp::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void PostfixOp::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void FuncCall::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void SubscriptExpr::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void MalformedExpr::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void ParamDecl::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void VariableDecl::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void FunctionDecl::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void CompoundStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void IfElseStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void ForLoopStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void ReturnStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void PrintStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void LocalDeclStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void ExpressionStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void MalformedStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void ProgramUnit::accept(AstVisitor& visitor) const { visitor.visit(*this); }

namespace {

class AstPrinter final : public AstVisitor {
public:
    std::string output() const { return buf_.str(); }

    void visit(const ProgramUnit& node) override {
        emitLine("ProgramUnit [" + meta(node) + "]");
        for (std::size_t i = 0; i < node.declarations.size(); ++i) {
            emitChild("decl[" + std::to_string(i) + "]", node.declarations[i].get());
        }
    }

    void visit(const ScalarType& node) override {
        emitLine("ScalarType(" + scalarKindName(node.scalar) + ", " + meta(node) + ")");
    }

    void visit(const VoidType& node) override {
        emitLine("VoidType(" + meta(node) + ")");
    }

    void visit(const ArrayType& node) override {
        emitLine("ArrayType(" + meta(node) + ")");
        if (node.lengthExpr) {
            emitChild("size", node.lengthExpr.get());
        }
        emitChild("elem", node.elementType.get());
    }

    void visit(const ScalarInit& node) override {
        emitLine("ScalarInit(" + meta(node) + ")");
        emitChild("expr", node.value.get());
    }

    void visit(const BraceInit& node) override {
        emitLine("BraceInit(" + meta(node) + ")");
        for (std::size_t i = 0; i < node.items.size(); ++i) {
            emitChild("item[" + std::to_string(i) + "]", node.items[i].get());
        }
    }

    void visit(const IntegerLit& node) override {
        emitLine("IntegerLit(val=" + node.rawText + ", " + meta(node) + ")");
    }

    void visit(const BooleanLit& node) override {
        emitLine(std::string("BooleanLit(val=") + (node.value ? "true" : "false") +
                 ", " + meta(node) + ")");
    }

    void visit(const CharacterLit& node) override {
        emitLine("CharacterLit(raw=" + node.rawText + ", " + meta(node) + ")");
    }

    void visit(const StringLit& node) override {
        emitLine("StringLit(raw=" + node.rawText + ", " + meta(node) + ")");
    }

    void visit(const VarRef& node) override {
        emitLine("VarRef(name='" + node.name + "', " + meta(node) + ")");
    }

    void visit(const AssignOp& node) override {
        emitLine("AssignOp(" + meta(node) + ")");
        emitChild("lhs", node.lhs.get());
        emitChild("rhs", node.rhs.get());
    }

    void visit(const BinaryOp& node) override {
        emitLine("BinaryOp(op='" + node.op + "', " + meta(node) + ")");
        emitChild("left", node.left.get());
        emitChild("right", node.right.get());
    }

    void visit(const UnaryOp& node) override {
        emitLine("UnaryOp(op='" + node.op + "', " + meta(node) + ")");
        emitChild("operand", node.operand.get());
    }

    void visit(const PostfixOp& node) override {
        emitLine("PostfixOp(op='" + node.op + "', " + meta(node) + ")");
        emitChild("target", node.target.get());
    }

    void visit(const FuncCall& node) override {
        std::string label;
        if (const auto* ref = dynamic_cast<const VarRef*>(node.callee.get())) {
            label = "callee='" + ref->name + "', ";
        }
        emitLine("FuncCall(" + label + meta(node) + ")");
        if (label.empty()) {
            emitChild("callee", node.callee.get());
        }
        for (std::size_t i = 0; i < node.args.size(); ++i) {
            emitChild("arg[" + std::to_string(i) + "]", node.args[i].get());
        }
    }

    void visit(const SubscriptExpr& node) override {
        emitLine("SubscriptExpr(" + meta(node) + ")");
        emitChild("base", node.base.get());
        emitChild("index", node.index.get());
    }

    void visit(const MalformedExpr& node) override {
        emitLine("MalformedExpr(" + meta(node) + ")");
    }

    void visit(const ParamDecl& node) override {
        emitLine("ParamDecl(name='" + node.name + "', " + meta(node) + ")");
        emitChild("type", node.type.get());
    }

    void visit(const VariableDecl& node) override {
        emitLine(std::string("VariableDecl(name='") + node.name +
                 "', scope=" + (node.globalScope ? "global" : "local") +
                 ", " + meta(node) + ")");
        emitChild("type", node.type.get());
        if (node.initializer) {
            emitChild("init", node.initializer.get());
        }
    }

    void visit(const FunctionDecl& node) override {
        emitLine(std::string("FunctionDecl(name='") + node.name +
                 "', kind=" + (node.isPrototype() ? "prototype" : "definition") +
                 ", " + meta(node) + ")");
        emitChild("returnType", node.returnType.get());
        for (std::size_t i = 0; i < node.params.size(); ++i) {
            emitChild("param[" + std::to_string(i) + "]", node.params[i].get());
        }
        if (node.body) {
            emitChild("body", node.body.get());
        }
    }

    void visit(const CompoundStmt& node) override {
        emitLine("CompoundStmt(" + meta(node) + ")");
        for (std::size_t i = 0; i < node.statements.size(); ++i) {
            emitChild("stmt[" + std::to_string(i) + "]", node.statements[i].get());
        }
    }

    void visit(const IfElseStmt& node) override {
        emitLine("IfElseStmt(" + meta(node) + ")");
        emitChild("cond", node.condition.get());
        emitChild("then", node.thenStmt.get());
        if (node.elseStmt) {
            emitChild("else", node.elseStmt.get());
        }
    }

    void visit(const ForLoopStmt& node) override {
        emitLine("ForLoopStmt(" + meta(node) + ")");
        if (node.initExpr) {
            emitChild("init", node.initExpr.get());
        }
        if (node.condExpr) {
            emitChild("cond", node.condExpr.get());
        }
        if (node.stepExpr) {
            emitChild("step", node.stepExpr.get());
        }
        emitChild("body", node.body.get());
    }

    void visit(const ReturnStmt& node) override {
        emitLine("ReturnStmt(" + meta(node) + ")");
        if (node.returnValue) {
            emitChild("value", node.returnValue.get());
        }
    }

    void visit(const PrintStmt& node) override {
        emitLine("PrintStmt(" + meta(node) + ")");
        for (std::size_t i = 0; i < node.items.size(); ++i) {
            emitChild("item[" + std::to_string(i) + "]", node.items[i].get());
        }
    }

    void visit(const LocalDeclStmt& node) override {
        if (node.varDecl) {
            node.varDecl->accept(*this);
        }
    }

    void visit(const ExpressionStmt& node) override {
        emitLine("ExpressionStmt(" + meta(node) + ")");
        emitChild("expr", node.expr.get());
    }

    void visit(const MalformedStmt& node) override {
        emitLine("MalformedStmt(" + meta(node) + ")");
    }

private:
    std::ostringstream buf_;
    std::size_t depth_ = 0;
    std::string fieldLabel_;

    static std::string meta(const AstNode& node) {
        std::string info = "pos=" + node.pos.lineCol();
        if (node.resolvedType.has_value()) {
            info += ", type=" + *node.resolvedType;
        }
        return info;
    }

    void emitLine(const std::string& text) {
        for (std::size_t i = 0; i < depth_; ++i) {
            buf_ << "  ";
        }
        if (!fieldLabel_.empty()) {
            buf_ << fieldLabel_ << ": ";
            fieldLabel_.clear();
        }
        buf_ << text << '\n';
    }

    void emitChild(const std::string& label, const AstNode* child) {
        ++depth_;
        fieldLabel_ = label;
        if (child != nullptr) {
            child->accept(*this);
        } else {
            emitLine("<null>");
        }
        fieldLabel_.clear();
        --depth_;
    }
};

} // namespace

std::string printAst(const AstNode& root) {
    AstPrinter printer;
    root.accept(printer);
    return printer.output();
}

} // namespace bmfe
