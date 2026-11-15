#ifndef BMFE_LEX_SCANNER_HPP
#define BMFE_LEX_SCANNER_HPP

#include "core/diagnostics.hpp"
#include "lex/token_kind.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace bmfe {

/// Output produced by scanning a B-Minor source buffer.
struct ScanOutput {
    std::vector<LexemeToken> stream;
    std::vector<LexemeToken> syntacticTokens;
    std::vector<DiagMessage> errors;
};

/// Single-pass DFA lexical scanner for B-Minor with non-fatal error recovery.
class Scanner {
public:
    static constexpr std::size_t kMaxIdentLen = 256;
    static constexpr std::size_t kMaxStringLen = 256;

    Scanner(std::string input, std::string filePath = "<stdin>");

    /// Scans the entire input buffer and returns lossless and parser-ready token streams.
    ScanOutput scanAll();

private:
    std::string input_;
    std::string filePath_;
    std::size_t index_ = 0;
    std::size_t line_ = 1;
    std::size_t col_ = 1;

    bool atEnd() const;
    char peekChar(std::size_t lookahead = 0) const;
    char nextChar();
    SourcePos here() const;

    LexemeToken makeToken(TokenKind kind, std::size_t startIdx, const SourcePos& startPos) const;
    void pushError(std::vector<DiagMessage>& errors, const SourcePos& pos, const std::string& msg);

    LexemeToken readNextToken(std::vector<DiagMessage>& errors);
    LexemeToken readWhitespace();
    LexemeToken readWordOrKeyword(std::vector<DiagMessage>& errors);
    LexemeToken readNumber();
    LexemeToken readStringLiteral(std::vector<DiagMessage>& errors);
    LexemeToken readCharLiteral(std::vector<DiagMessage>& errors);
    LexemeToken readLineComment();
    LexemeToken readBlockComment(std::vector<DiagMessage>& errors);
    LexemeToken readSymbolOrInvalid(std::vector<DiagMessage>& errors);
};

} // namespace bmfe

#endif // BMFE_LEX_SCANNER_HPP
