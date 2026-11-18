#include "syntax/rd_parser.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace bmfe {
namespace {

char resolveEscape(char code) {
    switch (code) {
        case 'n':  return '\n';
        case 't':  return '\t';
        case 'r':  return '\r';
        case '0':  return '\0';
        case '\\': return '\\';
        case '\'': return '\'';
        case '"':  return '"';
        default:   return code;
    }
}

std::string stripAndDecodeQuotes(const std::string& raw) {
    if (raw.size() < 2) {
        return {};
    }
    const std::string inner = raw.substr(1, raw.size() - 2);
    std::string decoded;
    decoded.reserve(inner.size());
    for (std::size_t i = 0; i < inner.size(); ++i) {
        if (inner[i] == '\\' && i + 1 < inner.size()) {
            decoded.push_back(resolveEscape(inner[++i]));
        } else {
            decoded.push_back(inner[i]);
        }
    }
    return decoded;
}

} // namespace

RdParser::RdParser(const std::vector<LexemeToken>& tokens, std::string filePath)
    : filePath_(std::move(filePath)) {
    tokens_.reserve(tokens.size() + 1);
    for (const auto& tok : tokens) {
        // Keep only syntactic tokens; trivia and scanner-flagged Invalid tokens are omitted.
        if (!isTriviaKind(tok.kind) && tok.kind != TokenKind::Invalid) {
            tokens_.push_back(tok);
        }
    }
    if (tokens_.empty() || tokens_.back().kind != TokenKind::EndOfFile) {
        LexemeToken eofTok;
        eofTok.kind = TokenKind::EndOfFile;
        eofTok.pos = tokens_.empty() ? SourcePos{filePath_, 1, 1} : tokens_.back().pos;
        tokens_.push_back(eofTok);
    }
}

ParseOutput RdParser::parseProgram() {
    ParseOutput result;
    std::vector<std::unique_ptr<Decl>> decls;
    const SourcePos rootPos = peek().pos;

    while (!atEnd()) {
        const std::size_t prevCursor = cursor_;
        auto decl = parseDecl();
        if (decl) {
            decls.push_back(std::move(decl));
        }
        if (cursor_ == prevCursor && !atEnd()) {
            consume();
        }
        recovering_ = false;
    }

    result.root = std::make_unique<ProgramUnit>(rootPos, std::move(decls));
    result.errors = std::move(errors_);
    return result;
}

const LexemeToken& RdParser::peek(std::size_t offset) const {
    const std::size_t idx = cursor_ + offset;
    return (idx < tokens_.size()) ? tokens_[idx] : tokens_.back();
}

const LexemeToken& RdParser::last() const {
    return (cursor_ == 0) ? tokens_.front() : tokens_[cursor_ - 1];
}

bool RdParser::atEnd() const {
    return peek().kind == TokenKind::EndOfFile;
}

bool RdParser::check(TokenKind kind) const {
    return peek().kind == kind;
}

bool RdParser::accept(TokenKind kind) {
    if (!check(kind)) {
        return false;
    }
    consume();
    return true;
}

const LexemeToken& RdParser::consume() {
    if (!atEnd()) {
        ++cursor_;
    }
    return last();
}

std::string RdParser::tokenLabel(const LexemeToken& tok) {
    return (tok.kind == TokenKind::EndOfFile) ? "EOF" : ("'" + tok.text + "'");
}

void RdParser::emitError(const SourcePos& pos, const std::string& msg) {
    if (recovering_) {
        return;
    }
    errors_.push_back(DiagMessage{DiagPhase::Syntactic, pos, msg});
    recovering_ = true;
}

bool RdParser::requireToken(TokenKind kind, const std::string& expectedDesc) {
    if (accept(kind)) {
        return true;
    }
    const LexemeToken& tok = peek();
    emitError(tok.pos, "expected " + expectedDesc + ", got " + tokenLabel(tok));
    return false;
}

bool RdParser::requireRightParenBeforeBody() {
    if (accept(TokenKind::RParen)) {
        return true;
    }
    const LexemeToken& tok = peek();
    if (tok.kind == TokenKind::LBrace) {
        emitError(tok.pos, "missing ')' before '{'");
        // Preserve '{' so the following block body parses intact.
        recovering_ = false;
        return false;
    }
    emitError(tok.pos, "expected ')', got " + tokenLabel(tok));
    syncUntilParenOrStmt();
    return false;
}

bool RdParser::requireSemicolonOrSync() {
    if (accept(TokenKind::Semicolon)) {
        recovering_ = false;
        return true;
    }
    const LexemeToken& tok = peek();
    emitError(tok.pos, "expected ';', got " + tokenLabel(tok));
    syncStatement();
    return false;
}

bool RdParser::isStatementSyncToken() const {
    switch (peek().kind) {
        case TokenKind::Semicolon:
        case TokenKind::LBrace:
        case TokenKind::RBrace:
        case TokenKind::KwIf:
        case TokenKind::KwFor:
        case TokenKind::KwReturn:
        case TokenKind::KwPrint:
        case TokenKind::EndOfFile:
            return true;
        case TokenKind::Ident:
            return peek(1).kind == TokenKind::Colon;
        default:
            return false;
    }
}

void RdParser::syncStatement() {
    while (!atEnd()) {
        if (accept(TokenKind::Semicolon)) {
            recovering_ = false;
            return;
        }
        if (isStatementSyncToken()) {
            recovering_ = false;
            return;
        }
        consume();
    }
    recovering_ = false;
}

void RdParser::syncUntilParenOrStmt() {
    while (!atEnd()) {
        if (accept(TokenKind::RParen)) {
            recovering_ = false;
            return;
        }
        if (isStatementSyncToken()) {
            recovering_ = false;
            return;
        }
        consume();
    }
    recovering_ = false;
}

void RdParser::syncDeclaration() {
    int braceDepth = 0;
    while (!atEnd()) {
        const TokenKind k = peek().kind;
        if (k == TokenKind::LBrace) {
            ++braceDepth;
            consume();
            continue;
        }
        if (k == TokenKind::RBrace) {
            consume();
            if (braceDepth > 0) {
                --braceDepth;
            }
            if (braceDepth == 0) {
                recovering_ = false;
                return;
            }
            continue;
        }
        if (braceDepth == 0) {
            if (k == TokenKind::Semicolon) {
                consume();
                recovering_ = false;
                return;
            }
            if (k == TokenKind::Ident && peek(1).kind == TokenKind::Colon) {
                recovering_ = false;
                return;
            }
        }
        consume();
    }
    recovering_ = false;
}

void RdParser::skipForHeaderClauseUntilSemi() {
    while (!atEnd() && !check(TokenKind::Semicolon) && !check(TokenKind::RParen) &&
           !check(TokenKind::LBrace)) {
        consume();
    }
    accept(TokenKind::Semicolon);
    recovering_ = false;
}

// ---------------------------------------------------------------------------
// Declarations & Types
// ---------------------------------------------------------------------------

std::unique_ptr<Decl> RdParser::parseDecl() {
    if (!check(TokenKind::Ident)) {
        const LexemeToken& tok = peek();
        emitError(tok.pos, "expected declaration, got " + tokenLabel(tok));
        syncDeclaration();
        return nullptr;
    }

    const LexemeToken identTok = consume();
    if (!requireToken(TokenKind::Colon, "':'")) {
        syncDeclaration();
        return nullptr;
    }

    if (check(TokenKind::KwFunction)) {
        return parseFunctionDeclTail(identTok);
    }
    return parseVariableDeclTail(identTok, /*isGlobal=*/true);
}

std::unique_ptr<FunctionDecl> RdParser::parseFunctionDeclTail(const LexemeToken& nameTok) {
    consume(); // Consume 'function'

    auto retType = parseReturnType();
    if (!requireToken(TokenKind::LParen, "'('")) {
        syncUntilParenOrStmt();
    }

    std::vector<std::unique_ptr<ParamDecl>> params;
    if (!check(TokenKind::RParen) && !check(TokenKind::LBrace) &&
        !check(TokenKind::Semicolon) && !check(TokenKind::OpAssign) && !atEnd()) {
        params = parseParamList();
    }

    if (!accept(TokenKind::RParen)) {
        const LexemeToken& tok = peek();
        if (tok.kind == TokenKind::LBrace) {
            emitError(tok.pos, "missing ')' before '{'");
            recovering_ = false;
        } else {
            emitError(tok.pos, "expected ')', got " + tokenLabel(tok));
            while (!atEnd() && !check(TokenKind::RParen) && !check(TokenKind::OpAssign) &&
                   !check(TokenKind::Semicolon) && !check(TokenKind::LBrace)) {
                consume();
            }
            accept(TokenKind::RParen);
            recovering_ = false;
        }
    }

    std::unique_ptr<CompoundStmt> body;
    if (accept(TokenKind::OpAssign)) {
        if (check(TokenKind::LBrace)) {
            body = parseBlockStmt();
        } else {
            const LexemeToken& tok = peek();
            emitError(tok.pos, "expected '{', got " + tokenLabel(tok));
            syncDeclaration();
        }
    } else if (accept(TokenKind::Semicolon)) {
        // Prototype declaration
    } else if (check(TokenKind::LBrace)) {
        emitError(peek().pos, "expected '=' or ';' before '{'");
        recovering_ = false;
        body = parseBlockStmt();
    } else {
        const LexemeToken& tok = peek();
        emitError(tok.pos, "expected '=' or ';', got " + tokenLabel(tok));
        syncDeclaration();
    }

    return std::make_unique<FunctionDecl>(nameTok.pos, nameTok.text, nameTok.pos,
                                          std::move(retType), std::move(params), std::move(body));
}

std::unique_ptr<VariableDecl> RdParser::parseVariableDeclTail(const LexemeToken& nameTok,
                                                              bool isGlobal) {
    auto typeSpec = parseTypeSpec();

    std::unique_ptr<InitNode> init;
    if (accept(TokenKind::OpAssign)) {
        init = parseInitializer();
    }

    if (isGlobal) {
        if (!accept(TokenKind::Semicolon)) {
            const LexemeToken& tok = peek();
            emitError(tok.pos, "expected ';', got " + tokenLabel(tok));
            syncDeclaration();
        } else {
            recovering_ = false;
        }
    } else {
        requireSemicolonOrSync();
    }

    return std::make_unique<VariableDecl>(nameTok.pos, nameTok.text, nameTok.pos,
                                          std::move(typeSpec), std::move(init), isGlobal);
}

std::vector<std::unique_ptr<ParamDecl>> RdParser::parseParamList() {
    std::vector<std::unique_ptr<ParamDecl>> params;
    if (auto first = parseParam()) {
        params.push_back(std::move(first));
    }
    while (accept(TokenKind::Comma)) {
        if (auto next = parseParam()) {
            params.push_back(std::move(next));
        } else {
            break;
        }
    }
    return params;
}

std::unique_ptr<ParamDecl> RdParser::parseParam() {
    if (!check(TokenKind::Ident)) {
        const LexemeToken& tok = peek();
        emitError(tok.pos, "expected parameter name, got " + tokenLabel(tok));
        return nullptr;
    }
    const LexemeToken identTok = consume();
    if (!requireToken(TokenKind::Colon, "':'")) {
        return nullptr;
    }
    auto type = parseTypeSpec();
    return std::make_unique<ParamDecl>(identTok.pos, identTok.text, identTok.pos, std::move(type));
}

std::unique_ptr<TypeSpec> RdParser::parseReturnType() {
    if (check(TokenKind::KwVoid)) {
        const LexemeToken tok = consume();
        return std::make_unique<VoidType>(tok.pos);
    }
    return parseTypeSpec();
}

std::unique_ptr<TypeSpec> RdParser::parseTypeSpec() {
    const LexemeToken& tok = peek();
    switch (tok.kind) {
        case TokenKind::KwInteger:
            consume();
            return std::make_unique<ScalarType>(tok.pos, ScalarKind::Integer);
        case TokenKind::KwBoolean:
            consume();
            return std::make_unique<ScalarType>(tok.pos, ScalarKind::Boolean);
        case TokenKind::KwChar:
            consume();
            return std::make_unique<ScalarType>(tok.pos, ScalarKind::Char);
        case TokenKind::KwString:
            consume();
            return std::make_unique<ScalarType>(tok.pos, ScalarKind::String);
        case TokenKind::KwArray:
            return parseArrayType();
        default:
            emitError(tok.pos, "expected type specifier, got " + tokenLabel(tok));
            return nullptr;
    }
}

std::unique_ptr<TypeSpec> RdParser::parseArrayType() {
    const LexemeToken arrTok = consume(); // Consume 'array'
    requireToken(TokenKind::LBracket, "'['");

    std::unique_ptr<Expr> lenExpr;
    if (!check(TokenKind::RBracket) && !atEnd()) {
        lenExpr = parseExpr();
    }
    requireToken(TokenKind::RBracket, "']'");

    auto elemType = parseTypeSpec();
    return std::make_unique<ArrayType>(arrTok.pos, std::move(lenExpr), std::move(elemType));
}

std::unique_ptr<InitNode> RdParser::parseInitializer() {
    if (accept(TokenKind::LBrace)) {
        const SourcePos bracePos = last().pos;
        std::vector<std::unique_ptr<InitNode>> items;
        if (!check(TokenKind::RBrace)) {
            items.push_back(parseInitializer());
            while (accept(TokenKind::Comma)) {
                items.push_back(parseInitializer());
            }
        }
        requireToken(TokenKind::RBrace, "'}'");
        return std::make_unique<BraceInit>(bracePos, std::move(items));
    }

    auto expr = parseExpr();
    return std::make_unique<ScalarInit>(expr->pos, std::move(expr));
}

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------

std::unique_ptr<CompoundStmt> RdParser::parseBlockStmt() {
    const LexemeToken lbrace = peek();
    if (!requireToken(TokenKind::LBrace, "'{'")) {
        return std::make_unique<CompoundStmt>(lbrace.pos, std::vector<std::unique_ptr<Stmt>>{});
    }

    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!check(TokenKind::RBrace) && !atEnd()) {
        const std::size_t prevCursor = cursor_;
        if (auto stmt = parseStatement()) {
            stmts.push_back(std::move(stmt));
        }
        if (cursor_ == prevCursor && !atEnd() && !check(TokenKind::RBrace)) {
            consume();
        }
        recovering_ = false;
    }

    requireToken(TokenKind::RBrace, "'}'");
    return std::make_unique<CompoundStmt>(lbrace.pos, std::move(stmts));
}

std::unique_ptr<Stmt> RdParser::parseStatement() {
    switch (peek().kind) {
        case TokenKind::KwIf:     return parseIfStmt();
        case TokenKind::KwFor:    return parseForStmt();
        case TokenKind::KwReturn: return parseReturnStmt();
        case TokenKind::KwPrint:  return parsePrintStmt();
        case TokenKind::LBrace:   return parseBlockStmt();
        case TokenKind::Ident:
            // Single LL(2) lookahead separates local declaration from expression statement.
            if (peek(1).kind == TokenKind::Colon) {
                return parseLocalDeclStmt();
            }
            return parseExprStmt();
        default:
            return parseExprStmt();
    }
}

std::unique_ptr<Stmt> RdParser::parseIfStmt() {
    const LexemeToken ifTok = consume();
    requireToken(TokenKind::LParen, "'('");
    auto cond = parseExpr();
    requireRightParenBeforeBody();

    auto thenBranch = parseStatement();
    std::unique_ptr<Stmt> elseBranch;
    if (accept(TokenKind::KwElse)) {
        elseBranch = parseStatement();
    }

    return std::make_unique<IfElseStmt>(ifTok.pos, std::move(cond),
                                        std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Stmt> RdParser::parseForStmt() {
    const LexemeToken forTok = consume();
    requireToken(TokenKind::LParen, "'('");

    std::unique_ptr<Expr> init;
    if (!check(TokenKind::Semicolon)) {
        init = parseExpr();
    }
    if (!requireToken(TokenKind::Semicolon, "';'")) {
        skipForHeaderClauseUntilSemi();
    }

    std::unique_ptr<Expr> cond;
    if (!check(TokenKind::Semicolon) && !check(TokenKind::RParen) && !check(TokenKind::LBrace)) {
        cond = parseExpr();
    }
    if (!requireToken(TokenKind::Semicolon, "';'")) {
        skipForHeaderClauseUntilSemi();
    }

    std::unique_ptr<Expr> step;
    if (!check(TokenKind::RParen) && !check(TokenKind::LBrace) && !check(TokenKind::Semicolon)) {
        step = parseExpr();
    }
    requireRightParenBeforeBody();

    auto body = parseStatement();
    return std::make_unique<ForLoopStmt>(forTok.pos, std::move(init), std::move(cond),
                                         std::move(step), std::move(body));
}

std::unique_ptr<Stmt> RdParser::parseReturnStmt() {
    const LexemeToken retTok = consume();
    std::unique_ptr<Expr> val;
    if (!check(TokenKind::Semicolon) && !check(TokenKind::RBrace) && !atEnd()) {
        val = parseExpr();
    }
    requireSemicolonOrSync();
    return std::make_unique<ReturnStmt>(retTok.pos, std::move(val));
}

std::unique_ptr<Stmt> RdParser::parsePrintStmt() {
    const LexemeToken printTok = consume();
    std::vector<std::unique_ptr<Expr>> items;
    if (!check(TokenKind::Semicolon) && !check(TokenKind::RBrace) && !atEnd()) {
        items = parseExprList();
    }
    requireSemicolonOrSync();
    return std::make_unique<PrintStmt>(printTok.pos, std::move(items));
}

std::unique_ptr<Stmt> RdParser::parseLocalDeclStmt() {
    const LexemeToken identTok = consume();
    requireToken(TokenKind::Colon, "':'");
    auto decl = parseVariableDeclTail(identTok, /*isGlobal=*/false);
    const SourcePos pos = decl->pos;
    return std::make_unique<LocalDeclStmt>(pos, std::move(decl));
}

std::unique_ptr<Stmt> RdParser::parseExprStmt() {
    const SourcePos startPos = peek().pos;
    if (accept(TokenKind::Semicolon)) {
        emitError(startPos, "expected statement, got ';'");
        recovering_ = false;
        return std::make_unique<MalformedStmt>(startPos);
    }

    auto expr = parseExpr();
    const bool hadErr = recovering_;
    requireSemicolonOrSync();

    if (hadErr && dynamic_cast<const MalformedExpr*>(expr.get()) != nullptr) {
        return std::make_unique<MalformedStmt>(startPos);
    }
    return std::make_unique<ExpressionStmt>(expr->pos, std::move(expr));
}

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

std::vector<std::unique_ptr<Expr>> RdParser::parseExprList() {
    std::vector<std::unique_ptr<Expr>> list;
    list.push_back(parseExpr());
    while (accept(TokenKind::Comma)) {
        list.push_back(parseExpr());
    }
    return list;
}

template <typename SubRuleFn>
std::unique_ptr<Expr> RdParser::parseLeftAssoc(SubRuleFn nextRule,
                                               std::initializer_list<TokenKind> ops) {
    auto expr = (this->*nextRule)();
    while (true) {
        bool found = false;
        LexemeToken opTok;
        for (const TokenKind kind : ops) {
            if (check(kind)) {
                opTok = consume();
                found = true;
                break;
            }
        }
        if (!found) {
            break;
        }
        auto rhs = (this->*nextRule)();
        const SourcePos pos = expr->pos;
        expr = std::make_unique<BinaryOp>(pos, opTok.text, std::move(expr), std::move(rhs));
    }
    return expr;
}

std::unique_ptr<Expr> RdParser::parseExpr() {
    return parseAssignment();
}

std::unique_ptr<Expr> RdParser::parseAssignment() {
    auto lhs = parseLogicOr();
    if (accept(TokenKind::OpAssign)) {
        auto rhs = parseAssignment();
        const SourcePos pos = lhs->pos;
        return std::make_unique<AssignOp>(pos, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> RdParser::parseLogicOr() {
    return parseLeftAssoc(&RdParser::parseLogicAnd, {TokenKind::OpOr});
}

std::unique_ptr<Expr> RdParser::parseLogicAnd() {
    return parseLeftAssoc(&RdParser::parseRelation, {TokenKind::OpAnd});
}

std::unique_ptr<Expr> RdParser::parseRelation() {
    return parseLeftAssoc(&RdParser::parseSum, {
        TokenKind::OpLess,
        TokenKind::OpLessEq,
        TokenKind::OpGreater,
        TokenKind::OpGreaterEq,
        TokenKind::OpEq,
        TokenKind::OpNotEq
    });
}

std::unique_ptr<Expr> RdParser::parseSum() {
    return parseLeftAssoc(&RdParser::parseProduct, {TokenKind::OpPlus, TokenKind::OpMinus});
}

std::unique_ptr<Expr> RdParser::parseProduct() {
    return parseLeftAssoc(&RdParser::parsePower,
                          {TokenKind::OpMul, TokenKind::OpDiv, TokenKind::OpMod});
}

std::unique_ptr<Expr> RdParser::parsePower() {
    auto base = parseUnary();
    if (accept(TokenKind::OpPow)) {
        auto exp = parsePower();
        const SourcePos pos = base->pos;
        return std::make_unique<BinaryOp>(pos, "^", std::move(base), std::move(exp));
    }
    return base;
}

std::unique_ptr<Expr> RdParser::parseUnary() {
    if (check(TokenKind::OpMinus) || check(TokenKind::OpNot)) {
        const LexemeToken opTok = consume();
        auto operand = parseUnary();
        return std::make_unique<UnaryOp>(opTok.pos, opTok.text, std::move(operand));
    }
    return parsePostfix();
}

std::unique_ptr<Expr> RdParser::parsePostfix() {
    auto expr = parseAtom();

    while (true) {
        if (accept(TokenKind::LBracket)) {
            auto idx = parseExpr();
            requireToken(TokenKind::RBracket, "']'");
            const SourcePos pos = expr->pos;
            expr = std::make_unique<SubscriptExpr>(pos, std::move(expr), std::move(idx));
        } else if (accept(TokenKind::LParen)) {
            std::vector<std::unique_ptr<Expr>> args;
            if (!check(TokenKind::RParen) && !check(TokenKind::Semicolon) && !atEnd()) {
                args = parseExprList();
            }
            requireToken(TokenKind::RParen, "')'");
            const SourcePos pos = expr->pos;
            expr = std::make_unique<FuncCall>(pos, std::move(expr), std::move(args));
        } else if (check(TokenKind::OpInc) || check(TokenKind::OpDec)) {
            const LexemeToken opTok = consume();
            const SourcePos pos = expr->pos;
            expr = std::make_unique<PostfixOp>(pos, opTok.text, std::move(expr));
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Expr> RdParser::parseAtom() {
    const LexemeToken& tok = peek();
    switch (tok.kind) {
        case TokenKind::Ident: {
            const LexemeToken ident = consume();
            return std::make_unique<VarRef>(ident.pos, ident.text);
        }
        case TokenKind::IntLit:
            return makeIntegerLit(consume());
        case TokenKind::BoolLit: {
            const LexemeToken boolTok = consume();
            return std::make_unique<BooleanLit>(boolTok.pos, boolTok.text == "true");
        }
        case TokenKind::CharLit:
            return makeCharLit(consume());
        case TokenKind::StrLit:
            return makeStringLit(consume());
        case TokenKind::LParen: {
            consume();
            auto inner = parseExpr();
            requireToken(TokenKind::RParen, "')'");
            return inner;
        }
        default:
            emitError(tok.pos, "expected expression, got " + tokenLabel(tok));
            return std::make_unique<MalformedExpr>(tok.pos);
    }
}

std::unique_ptr<Expr> RdParser::makeIntegerLit(const LexemeToken& tok) {
    long long val = 0;
    bool overflow = false;
    try {
        std::size_t parsedLen = 0;
        val = std::stoll(tok.text, &parsedLen, 10);
        if (parsedLen != tok.text.size()) {
            overflow = true;
        }
    } catch (const std::out_of_range&) {
        overflow = true;
        val = std::numeric_limits<long long>::max();
    } catch (const std::invalid_argument&) {
        overflow = true;
    }
    return std::make_unique<IntegerLit>(tok.pos, tok.text, val, overflow);
}

std::unique_ptr<Expr> RdParser::makeCharLit(const LexemeToken& tok) {
    const std::string decoded = stripAndDecodeQuotes(tok.text);
    const char ch = decoded.empty() ? '\0' : decoded.front();
    return std::make_unique<CharacterLit>(tok.pos, tok.text, ch);
}

std::unique_ptr<Expr> RdParser::makeStringLit(const LexemeToken& tok) {
    return std::make_unique<StringLit>(tok.pos, tok.text, stripAndDecodeQuotes(tok.text));
}

} // namespace bmfe
