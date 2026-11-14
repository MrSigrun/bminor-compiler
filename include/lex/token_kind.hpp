#ifndef BMFE_LEX_TOKEN_KIND_HPP
#define BMFE_LEX_TOKEN_KIND_HPP

#include "core/source_pos.hpp"

#include <cstddef>
#include <string>

namespace bmfe {

/// Complete token classification for B-Minor lexical scanning.
enum class TokenKind {
    // Identifiers & Literals
    Ident,
    IntLit,
    BoolLit,
    CharLit,
    StrLit,

    // Reserved Keywords
    KwArray,
    KwBoolean,
    KwChar,
    KwElse,
    KwFor,
    KwFunction,
    KwIf,
    KwInteger,
    KwPrint,
    KwReturn,
    KwString,
    KwVoid,
    KwWhile,

    // Operators
    OpPlus,
    OpMinus,
    OpMul,
    OpDiv,
    OpMod,
    OpPow,
    OpInc,
    OpDec,
    OpNot,
    OpLess,
    OpLessEq,
    OpGreater,
    OpGreaterEq,
    OpEq,
    OpNotEq,
    OpAnd,
    OpOr,
    OpAssign,

    // Delimiters
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Colon,
    Semicolon,
    Comma,

    // Preserved Trivia
    LineComment,
    BlockComment,
    Whitespace,

    // Diagnostics / Sentinel
    Invalid,
    EndOfFile
};

/// Single scanned token retaining exact source text, position, and byte span.
struct LexemeToken {
    TokenKind kind = TokenKind::Invalid;
    std::string text;
    SourcePos pos;
    std::size_t offset = 0;
    std::size_t byteLength = 0;
};

/// Returns true for any B-Minor keyword (excluding boolean literals true/false).
bool isKeywordKind(TokenKind kind);

/// Returns true for built-in value type keywords (integer, boolean, char, string).
bool isScalarTypeKind(TokenKind kind);

/// Returns true for whitespace or comment tokens excluded from parser input.
bool isTriviaKind(TokenKind kind);

/// Returns true for structural punctuation/delimiter tokens.
bool isDelimiterKind(TokenKind kind);

/// Formats a token into human-readable specification notation (e.g. "KW(if)", "IDENT(n)", "EOF").
std::string formatLexemeToken(const LexemeToken& tok);

} // namespace bmfe

#endif // BMFE_LEX_TOKEN_KIND_HPP
