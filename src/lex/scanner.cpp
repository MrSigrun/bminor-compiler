#include "lex/scanner.hpp"

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

namespace bmfe {
namespace {

struct KeywordEntry {
    std::string_view text;
    TokenKind kind;
};

// Sorted lexicographically for binary search via std::lower_bound.
constexpr std::array<KeywordEntry, 15> kSortedKeywords = {{
    {"array",    TokenKind::KwArray},
    {"boolean",  TokenKind::KwBoolean},
    {"char",     TokenKind::KwChar},
    {"else",     TokenKind::KwElse},
    {"false",    TokenKind::BoolLit},
    {"for",      TokenKind::KwFor},
    {"function", TokenKind::KwFunction},
    {"if",       TokenKind::KwIf},
    {"integer",  TokenKind::KwInteger},
    {"print",    TokenKind::KwPrint},
    {"return",   TokenKind::KwReturn},
    {"string",   TokenKind::KwString},
    {"true",     TokenKind::BoolLit},
    {"void",     TokenKind::KwVoid},
    {"while",    TokenKind::KwWhile}
}};

TokenKind resolveWordKind(std::string_view word) {
    const auto it = std::lower_bound(
        kSortedKeywords.begin(),
        kSortedKeywords.end(),
        word,
        [](const KeywordEntry& entry, std::string_view key) {
            return entry.text < key;
        });
    if (it != kSortedKeywords.end() && it->text == word) {
        return it->kind;
    }
    return TokenKind::Ident;
}

bool isAlpha(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

bool isDigit(char ch) {
    return ch >= '0' && ch <= '9';
}

bool isWordStart(char ch) {
    return isAlpha(ch) || ch == '_';
}

bool isWordBody(char ch) {
    return isWordStart(ch) || isDigit(ch);
}

bool isSpaceChar(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

TokenKind matchSingleDelimiter(char ch) {
    switch (ch) {
        case '(': return TokenKind::LParen;
        case ')': return TokenKind::RParen;
        case '{': return TokenKind::LBrace;
        case '}': return TokenKind::RBrace;
        case '[': return TokenKind::LBracket;
        case ']': return TokenKind::RBracket;
        case ':': return TokenKind::Colon;
        case ';': return TokenKind::Semicolon;
        case ',': return TokenKind::Comma;
        default:  return TokenKind::Invalid;
    }
}

} // namespace

Scanner::Scanner(std::string input, std::string filePath)
    : input_(std::move(input)), filePath_(std::move(filePath)) {}

bool Scanner::atEnd() const {
    return index_ >= input_.size();
}

char Scanner::peekChar(std::size_t lookahead) const {
    const std::size_t target = index_ + lookahead;
    return (target < input_.size()) ? input_[target] : '\0';
}

char Scanner::nextChar() {
    const char ch = input_[index_++];
    if (ch == '\n') {
        ++line_;
        col_ = 1;
    } else {
        ++col_;
    }
    return ch;
}

SourcePos Scanner::here() const {
    return SourcePos{filePath_, line_, col_};
}

LexemeToken Scanner::makeToken(TokenKind kind,
                               std::size_t startIdx,
                               const SourcePos& startPos) const {
    const std::size_t len = index_ - startIdx;
    return LexemeToken{kind, input_.substr(startIdx, len), startPos, startIdx, len};
}

void Scanner::pushError(std::vector<DiagMessage>& errors,
                        const SourcePos& pos,
                        const std::string& msg) {
    errors.push_back(DiagMessage{DiagPhase::Lexical, pos, msg});
}

ScanOutput Scanner::scanAll() {
    ScanOutput output;
    index_ = 0;
    line_ = 1;
    col_ = 1;

    while (!atEnd()) {
        LexemeToken tok = readNextToken(output.errors);
        if (!isTriviaKind(tok.kind)) {
            output.syntacticTokens.push_back(tok);
        }
        output.stream.push_back(std::move(tok));
    }

    LexemeToken eofTok{TokenKind::EndOfFile, "", here(), index_, 0};
    output.stream.push_back(eofTok);
    output.syntacticTokens.push_back(std::move(eofTok));
    return output;
}

LexemeToken Scanner::readNextToken(std::vector<DiagMessage>& errors) {
    const char ch = peekChar();
    if (isSpaceChar(ch)) {
        return readWhitespace();
    }
    if (isWordStart(ch)) {
        return readWordOrKeyword(errors);
    }
    if (isDigit(ch)) {
        return readNumber();
    }
    if (ch == '"') {
        return readStringLiteral(errors);
    }
    if (ch == '\'') {
        return readCharLiteral(errors);
    }
    if (ch == '/' && peekChar(1) == '/') {
        return readLineComment();
    }
    if (ch == '/' && peekChar(1) == '*') {
        return readBlockComment(errors);
    }
    return readSymbolOrInvalid(errors);
}

LexemeToken Scanner::readWhitespace() {
    const std::size_t startIdx = index_;
    const SourcePos startPos = here();
    while (!atEnd() && isSpaceChar(peekChar())) {
        nextChar();
    }
    return makeToken(TokenKind::Whitespace, startIdx, startPos);
}

LexemeToken Scanner::readWordOrKeyword(std::vector<DiagMessage>& errors) {
    const std::size_t startIdx = index_;
    const SourcePos startPos = here();
    while (!atEnd() && isWordBody(peekChar())) {
        nextChar();
    }

    const std::size_t len = index_ - startIdx;
    if (len > kMaxIdentLen) {
        pushError(errors, startPos, "Identifier length exceeds 256 characters");
        return makeToken(TokenKind::Invalid, startIdx, startPos);
    }

    const std::string_view slice(input_.data() + startIdx, len);
    return makeToken(resolveWordKind(slice), startIdx, startPos);
}

LexemeToken Scanner::readNumber() {
    const std::size_t startIdx = index_;
    const SourcePos startPos = here();
    while (!atEnd() && isDigit(peekChar())) {
        nextChar();
    }
    return makeToken(TokenKind::IntLit, startIdx, startPos);
}

LexemeToken Scanner::readStringLiteral(std::vector<DiagMessage>& errors) {
    const std::size_t startIdx = index_;
    const SourcePos startPos = here();
    nextChar(); // Consume opening '"'

    std::size_t charCount = 0;
    bool closed = false;

    while (!atEnd()) {
        const char ch = peekChar();
        if (ch == '\n' || ch == '\r') {
            break;
        }
        if (ch == '"') {
            nextChar();
            closed = true;
            break;
        }
        if (ch == '\\') {
            nextChar();
            if (atEnd() || peekChar() == '\n' || peekChar() == '\r') {
                break;
            }
            nextChar();
            ++charCount;
            continue;
        }
        nextChar();
        ++charCount;
    }

    if (!closed) {
        pushError(errors, startPos, "Unterminated string literal");
        return makeToken(TokenKind::Invalid, startIdx, startPos);
    }
    if (charCount > kMaxStringLen) {
        pushError(errors, startPos, "String literal exceeds 256 characters");
        return makeToken(TokenKind::Invalid, startIdx, startPos);
    }
    return makeToken(TokenKind::StrLit, startIdx, startPos);
}

LexemeToken Scanner::readCharLiteral(std::vector<DiagMessage>& errors) {
    const std::size_t startIdx = index_;
    const SourcePos startPos = here();
    nextChar(); // Consume opening '\''

    if (atEnd() || peekChar() == '\n' || peekChar() == '\r') {
        pushError(errors, startPos, "Unterminated character literal");
        return makeToken(TokenKind::Invalid, startIdx, startPos);
    }

    if (peekChar() == '\'') {
        nextChar();
        pushError(errors, startPos, "Empty character literal");
        return makeToken(TokenKind::Invalid, startIdx, startPos);
    }

    // Read a single character or escape pair.
    if (peekChar() == '\\') {
        nextChar();
        if (atEnd() || peekChar() == '\n' || peekChar() == '\r') {
            pushError(errors, startPos, "Unterminated character literal");
            return makeToken(TokenKind::Invalid, startIdx, startPos);
        }
        nextChar();
    } else {
        nextChar();
    }

    if (!atEnd() && peekChar() == '\'') {
        nextChar();
        return makeToken(TokenKind::CharLit, startIdx, startPos);
    }

    bool foundClose = false;
    while (!atEnd() && peekChar() != '\n' && peekChar() != '\r') {
        if (peekChar() == '\'') {
            nextChar();
            foundClose = true;
            break;
        }
        if (peekChar() == '\\' && peekChar(1) != '\0' &&
            peekChar(1) != '\n' && peekChar(1) != '\r') {
            nextChar();
        }
        nextChar();
    }

    pushError(errors,
              startPos,
              foundClose ? "Character literal contains more than one character"
                         : "Unterminated character literal");
    return makeToken(TokenKind::Invalid, startIdx, startPos);
}

LexemeToken Scanner::readLineComment() {
    const std::size_t startIdx = index_;
    const SourcePos startPos = here();
    nextChar(); // '/'
    nextChar(); // '/'
    while (!atEnd() && peekChar() != '\n' && peekChar() != '\r') {
        nextChar();
    }
    return makeToken(TokenKind::LineComment, startIdx, startPos);
}

LexemeToken Scanner::readBlockComment(std::vector<DiagMessage>& errors) {
    const std::size_t startIdx = index_;
    const SourcePos startPos = here();
    nextChar(); // '/'
    nextChar(); // '*'

    bool closed = false;
    while (!atEnd()) {
        if (peekChar() == '*' && peekChar(1) == '/') {
            nextChar();
            nextChar();
            closed = true;
            break;
        }
        nextChar();
    }

    if (!closed) {
        pushError(errors, startPos, "Unterminated block comment");
        return makeToken(TokenKind::Invalid, startIdx, startPos);
    }
    return makeToken(TokenKind::BlockComment, startIdx, startPos);
}

LexemeToken Scanner::readSymbolOrInvalid(std::vector<DiagMessage>& errors) {
    const std::size_t startIdx = index_;
    const SourcePos startPos = here();
    const char ch = nextChar();

    const TokenKind delim = matchSingleDelimiter(ch);
    if (delim != TokenKind::Invalid) {
        return makeToken(delim, startIdx, startPos);
    }

    auto takeIf = [&](char expected, TokenKind twoChar, TokenKind oneChar) -> LexemeToken {
        if (peekChar() == expected) {
            nextChar();
            return makeToken(twoChar, startIdx, startPos);
        }
        return makeToken(oneChar, startIdx, startPos);
    };

    switch (ch) {
        case '+': return takeIf('+', TokenKind::OpInc, TokenKind::OpPlus);
        case '-': return takeIf('-', TokenKind::OpDec, TokenKind::OpMinus);
        case '*': return makeToken(TokenKind::OpMul, startIdx, startPos);
        case '/': return makeToken(TokenKind::OpDiv, startIdx, startPos);
        case '%': return makeToken(TokenKind::OpMod, startIdx, startPos);
        case '^': return makeToken(TokenKind::OpPow, startIdx, startPos);
        case '<': return takeIf('=', TokenKind::OpLessEq, TokenKind::OpLess);
        case '>': return takeIf('=', TokenKind::OpGreaterEq, TokenKind::OpGreater);
        case '=': return takeIf('=', TokenKind::OpEq, TokenKind::OpAssign);
        case '!': return takeIf('=', TokenKind::OpNotEq, TokenKind::OpNot);
        case '&':
            if (peekChar() == '&') {
                nextChar();
                return makeToken(TokenKind::OpAnd, startIdx, startPos);
            }
            pushError(errors, startPos, "Invalid operator '&'; expected '&&'");
            return makeToken(TokenKind::Invalid, startIdx, startPos);
        case '|':
            if (peekChar() == '|') {
                nextChar();
                return makeToken(TokenKind::OpOr, startIdx, startPos);
            }
            pushError(errors, startPos, "Invalid operator '|'; expected '||'");
            return makeToken(TokenKind::Invalid, startIdx, startPos);
        default:
            pushError(errors, startPos, std::string("Unrecognized character '") + ch + "'");
            return makeToken(TokenKind::Invalid, startIdx, startPos);
    }
}

} // namespace bmfe
