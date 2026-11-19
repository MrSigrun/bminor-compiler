#include "lex/scanner.hpp"
#include "syntax/ast_nodes.hpp"
#include "syntax/rd_parser.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace bmfe;

namespace {

ParseOutput parseText(const std::string& src, ScanOutput* outScan = nullptr) {
    Scanner scanner(src, "sample.bminor");
    ScanOutput scanOut = scanner.scanAll();
    if (outScan != nullptr) {
        *outScan = scanOut;
    }
    RdParser parser(scanOut.syntacticTokens, "sample.bminor");
    return parser.parseProgram();
}

void verifyAstPositionsAndTypeSlot() {
    const std::string code =
        "fib: function integer ( n: integer ) = {\n"
        "    if (n <= 1) return n;\n"
        "    return n * fib(n - 1);\n"
        "}\n";

    ScanOutput scanOut;
    ParseOutput res = parseText(code, &scanOut);
    assert(scanOut.errors.empty());
    assert(res.ok());
    assert(res.root != nullptr);
    assert(!res.root->resolvedType.has_value());
    assert(res.root->declarations.size() == 1);

    const auto* fn = dynamic_cast<const FunctionDecl*>(res.root->declarations[0].get());
    assert(fn != nullptr && fn->name == "fib" && !fn->isPrototype());
    assert(!fn->resolvedType.has_value());
    assert(fn->namePos.line == 1 && fn->namePos.col == 1);
    assert(fn->params.size() == 1 && fn->params[0]->name == "n");

    const auto* ret = dynamic_cast<const ReturnStmt*>(fn->body->statements[1].get());
    assert(ret != nullptr);
    const auto* mul = dynamic_cast<const BinaryOp*>(ret->returnValue.get());
    assert(mul != nullptr && mul->op == "*");
    const auto* lhsVar = dynamic_cast<const VarRef*>(mul->left.get());
    assert(lhsVar != nullptr && lhsVar->pos.line == 3 && lhsVar->pos.col == 12);

    const auto* call = dynamic_cast<const FuncCall*>(mul->right.get());
    assert(call != nullptr && call->pos.line == 3 && call->pos.col == 16);
    const auto* callee = dynamic_cast<const VarRef*>(call->callee.get());
    assert(callee != nullptr && callee->name == "fib");

    const std::string printed = printAst(*res.root);
    assert(printed.find("FuncCall(callee='fib', pos=3:16)") != std::string::npos);
}

void verifyLanguageConstructs() {
    const std::string code =
        "put_line: function void ( s: string );\n"
        "mat: array [2] array [2] integer = { {1, 2}, {3, 4} };\n"
        "walk: function integer ( arr: array [] integer ) = {\n"
        "    i: integer = 0;\n"
        "    for (i = 0; i < 4; i++) print arr[i];\n"
        "    for (;;) return 0;\n"
        "    if (true) if (false) return 1; else return 2;\n"
        "    return;\n"
        "}\n";

    ParseOutput res = parseText(code);
    assert(res.ok());
    assert(res.root->declarations.size() == 3);

    const auto* proto = dynamic_cast<const FunctionDecl*>(res.root->declarations[0].get());
    assert(proto != nullptr && proto->isPrototype());
    assert(dynamic_cast<const VoidType*>(proto->returnType.get()) != nullptr);

    const auto* mat = dynamic_cast<const VariableDecl*>(res.root->declarations[1].get());
    assert(mat != nullptr && mat->globalScope);
    const auto* outerArr = dynamic_cast<const ArrayType*>(mat->type.get());
    assert(outerArr != nullptr && outerArr->lengthExpr != nullptr);
    const auto* innerArr = dynamic_cast<const ArrayType*>(outerArr->elementType.get());
    assert(innerArr != nullptr && innerArr->lengthExpr != nullptr);
    const auto* init = dynamic_cast<const BraceInit*>(mat->initializer.get());
    assert(init != nullptr && init->items.size() == 2);

    const auto* walk = dynamic_cast<const FunctionDecl*>(res.root->declarations[2].get());
    assert(walk != nullptr && walk->body->statements.size() == 5);
    assert(dynamic_cast<const LocalDeclStmt*>(walk->body->statements[0].get()) != nullptr);

    const auto* outerIf = dynamic_cast<const IfElseStmt*>(walk->body->statements[3].get());
    assert(outerIf != nullptr && outerIf->elseStmt == nullptr);
    const auto* innerIf = dynamic_cast<const IfElseStmt*>(outerIf->thenStmt.get());
    assert(innerIf != nullptr && innerIf->elseStmt != nullptr);
}

void verifyPrecedenceAndAssociativity() {
    const std::string code =
        "expr_test: function void () = {\n"
        "    x = y = 2 + 3 * 4 - 2 ^ 3 ^ 2;\n"
        "    z = ! -a || b && fn(1)[0]--;\n"
        "}\n";

    ParseOutput res = parseText(code);
    assert(res.ok());
    const auto* fn = dynamic_cast<const FunctionDecl*>(res.root->declarations[0].get());
    assert(fn != nullptr && fn->body->statements.size() == 2);

    const auto* s0 = dynamic_cast<const ExpressionStmt*>(fn->body->statements[0].get());
    const auto* a0 = dynamic_cast<const AssignOp*>(s0->expr.get());
    assert(a0 != nullptr);
    const auto* a1 = dynamic_cast<const AssignOp*>(a0->rhs.get());
    assert(a1 != nullptr);
    const auto* sub = dynamic_cast<const BinaryOp*>(a1->rhs.get());
    assert(sub != nullptr && sub->op == "-");
    const auto* pow1 = dynamic_cast<const BinaryOp*>(sub->right.get());
    assert(pow1 != nullptr && pow1->op == "^");
    const auto* pow2 = dynamic_cast<const BinaryOp*>(pow1->right.get());
    assert(pow2 != nullptr && pow2->op == "^");

    const auto* s1 = dynamic_cast<const ExpressionStmt*>(fn->body->statements[1].get());
    const auto* assign1 = dynamic_cast<const AssignOp*>(s1->expr.get());
    const auto* orOp = dynamic_cast<const BinaryOp*>(assign1->rhs.get());
    assert(orOp != nullptr && orOp->op == "||");
    const auto* andOp = dynamic_cast<const BinaryOp*>(orOp->right.get());
    assert(andOp != nullptr && andOp->op == "&&");
    const auto* post = dynamic_cast<const PostfixOp*>(andOp->right.get());
    assert(post != nullptr && post->op == "--");
    const auto* subsc = dynamic_cast<const SubscriptExpr*>(post->target.get());
    assert(subsc != nullptr);
    assert(dynamic_cast<const FuncCall*>(subsc->base.get()) != nullptr);
}

void verifyPanicModeRecoveryAndEdgeCases() {
    const std::string code =
        "bad_top 99;\n"
        "main: function integer () = {\n"
        "    a: integer = ;\n"
        "    if (a > 0 {\n"
        "        print a;\n"
        "    }\n"
        "    b: integer = 1 + * 2;\n"
        "    ok: integer = 11;\n"
        "    return ok;\n"
        "}\n"
        "after: function integer () = { return 1; }\n";

    ParseOutput res = parseText(code);
    assert(!res.ok());
    assert(res.errors.size() == 4);
    assert(res.root->declarations.size() == 2);

    const auto* mainFn = dynamic_cast<const FunctionDecl*>(res.root->declarations[0].get());
    assert(mainFn != nullptr && mainFn->name == "main");
    assert(mainFn->body->statements.size() == 5);
    const auto* ifStmt = dynamic_cast<const IfElseStmt*>(mainFn->body->statements[1].get());
    assert(ifStmt != nullptr);
    const auto* ifBody = dynamic_cast<const CompoundStmt*>(ifStmt->thenStmt.get());
    assert(ifBody != nullptr && ifBody->statements.size() == 1);

    const auto* afterFn = dynamic_cast<const FunctionDecl*>(res.root->declarations[1].get());
    assert(afterFn != nullptr && afterFn->name == "after");

    // Empty and truncated streams must safely terminate without crashing
    RdParser emptyParser({}, "empty.bminor");
    ParseOutput emptyOut = emptyParser.parseProgram();
    assert(emptyOut.ok() && emptyOut.root->declarations.empty());

    ParseOutput truncOut = parseText("f: function integer (");
    assert(!truncOut.ok() && truncOut.root != nullptr);
}

} // namespace

int main() {
    verifyAstPositionsAndTypeSlot();
    verifyLanguageConstructs();
    verifyPrecedenceAndAssociativity();
    verifyPanicModeRecoveryAndEdgeCases();

    std::cout << "Parser and AST unit tests passed.\n";
    return 0;
}
