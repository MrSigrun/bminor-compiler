#include "lex/token_kind.hpp"

namespace bmfe {

bool isKeywordKind(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwArray:
        case TokenKind::KwBoolean:
        case TokenKind::KwChar:
        case TokenKind::KwElse:
        case TokenKind::KwFor:
        case TokenKind::KwFunction:
        case TokenKind::KwIf:
        case TokenKind::KwInteger:
        case TokenKind::KwPrint:
        case TokenKind::KwReturn:
        case TokenKind::KwString:
        case TokenKind::KwVoid:
        case TokenKind::KwWhile:
            return true;
        default:
            return false;
    }
}

bool isScalarTypeKind(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwBoolean:
        case TokenKind::KwChar:
        case TokenKind::KwInteger:
        case TokenKind::KwString:
            return true;
        default:
            return false;
    }
}

bool isTriviaKind(TokenKind kind) {
    return kind == TokenKind::Whitespace ||
           kind == TokenKind::LineComment ||
           kind == TokenKind::BlockComment;
}

bool isDelimiterKind(TokenKind kind) {
    switch (kind) {
        case TokenKind::LParen:
        case TokenKind::RParen:
        case TokenKind::LBrace:
        case TokenKind::RBrace:
        case TokenKind::LBracket:
        case TokenKind::RBracket:
        case TokenKind::Colon:
        case TokenKind::Semicolon:
        case TokenKind::Comma:
            return true;
        default:
            return false;
    }
}

std::string formatLexemeToken(const LexemeToken& tok) {
    if (isKeywordKind(tok.kind)) {
        return "KW(" + tok.text + ")";
    }

    switch (tok.kind) {
        case TokenKind::Ident:        return "IDENT(" + tok.text + ")";
        case TokenKind::IntLit:       return "INT(" + tok.text + ")";
        case TokenKind::BoolLit:      return "BOOL(" + tok.text + ")";
        case TokenKind::CharLit:      return "CHAR(" + tok.text + ")";
        case TokenKind::StrLit:       return "STRING(" + tok.text + ")";
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
        case TokenKind::OpOr:         return "OP(" + tok.text + ")";
        case TokenKind::OpAssign:     return "ASSIGN";
        case TokenKind::LParen:       return "LPAREN";
        case TokenKind::RParen:       return "RPAREN";
        case TokenKind::LBrace:       return "LBRACE";
        case TokenKind::RBrace:       return "RBRACE";
        case TokenKind::LBracket:     return "LBRACKET";
        case TokenKind::RBracket:     return "RBRACKET";
        case TokenKind::Colon:        return "COLON";
        case TokenKind::Semicolon:    return "SEMI";
        case TokenKind::Comma:        return "COMMA";
        case TokenKind::LineComment:  return "COMMENT_LINE";
        case TokenKind::BlockComment: return "COMMENT_BLOCK";
        case TokenKind::Whitespace:   return "WHITESPACE";
        case TokenKind::Invalid:      return "INVALID('" + tok.text + "')";
        case TokenKind::EndOfFile:    return "EOF";
        default:                      return "UNKNOWN";
    }
    return "UNKNOWN";
}

} // namespace bmfe
