#ifndef BMFE_SYNTAX_RD_PARSER_HPP
#define BMFE_SYNTAX_RD_PARSER_HPP

#include "core/diagnostics.hpp"
#include "lex/token_kind.hpp"
#include "syntax/ast_nodes.hpp"

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace bmfe {

/// Result produced by parsing a B-Minor syntactic token stream.
struct ParseOutput {
    std::unique_ptr<ProgramUnit> root;
    std::vector<DiagMessage> errors;

    bool ok() const { return errors.empty(); }
};

/// LL(1)/LL(2) Recursive-Descent Parser with panic-mode error recovery.
class RdParser {
public:
    explicit RdParser(const std::vector<LexemeToken>& tokens, std::string filePath = "<stdin>");

    /// Parses the token stream into a ProgramUnit AST while collecting syntax diagnostics.
    ParseOutput parseProgram();

private:
    std::vector<LexemeToken> tokens_;
    std::string filePath_;
    std::size_t cursor_ = 0;
    std::vector<DiagMessage> errors_;
    bool recovering_ = false;

    // Stream navigation
    const LexemeToken& peek(std::size_t offset = 0) const;
    const LexemeToken& last() const;
    bool atEnd() const;
    bool check(TokenKind kind) const;
    bool accept(TokenKind kind);
    const LexemeToken& consume();

    // Error handling and panic-mode synchronization
    void emitError(const SourcePos& pos, const std::string& msg);
    bool requireToken(TokenKind kind, const std::string& expectedDesc);
    bool requireRightParenBeforeBody();
    bool requireSemicolonOrSync();
    void syncDeclaration();
    void syncStatement();
    void syncUntilParenOrStmt();
    bool isStatementSyncToken() const;
    void skipForHeaderClauseUntilSemi();
    static std::string tokenLabel(const LexemeToken& tok);

    // Declarations and types
    std::unique_ptr<Decl> parseDecl();
    std::unique_ptr<FunctionDecl> parseFunctionDeclTail(const LexemeToken& nameTok);
    std::unique_ptr<VariableDecl> parseVariableDeclTail(const LexemeToken& nameTok, bool isGlobal);
    std::vector<std::unique_ptr<ParamDecl>> parseParamList();
    std::unique_ptr<ParamDecl> parseParam();
    std::unique_ptr<TypeSpec> parseReturnType();
    std::unique_ptr<TypeSpec> parseTypeSpec();
    std::unique_ptr<TypeSpec> parseArrayType();
    std::unique_ptr<InitNode> parseInitializer();

    // Statements
    std::unique_ptr<CompoundStmt> parseBlockStmt();
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<Stmt> parseReturnStmt();
    std::unique_ptr<Stmt> parsePrintStmt();
    std::unique_ptr<Stmt> parseLocalDeclStmt();
    std::unique_ptr<Stmt> parseExprStmt();

    // Expressions
    std::vector<std::unique_ptr<Expr>> parseExprList();
    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseLogicOr();
    std::unique_ptr<Expr> parseLogicAnd();
    std::unique_ptr<Expr> parseRelation();
    std::unique_ptr<Expr> parseSum();
    std::unique_ptr<Expr> parseProduct();
    std::unique_ptr<Expr> parsePower();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parseAtom();

    template <typename SubRuleFn>
    std::unique_ptr<Expr> parseLeftAssoc(SubRuleFn nextRule,
                                         std::initializer_list<TokenKind> ops);

    // Literal builders
    std::unique_ptr<Expr> makeIntegerLit(const LexemeToken& tok);
    std::unique_ptr<Expr> makeCharLit(const LexemeToken& tok);
    std::unique_ptr<Expr> makeStringLit(const LexemeToken& tok);
};

} // namespace bmfe

#endif // BMFE_SYNTAX_RD_PARSER_HPP
