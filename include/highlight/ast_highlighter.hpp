#ifndef BMFE_HIGHLIGHT_AST_HIGHLIGHTER_HPP
#define BMFE_HIGHLIGHT_AST_HIGHLIGHTER_HPP

#include "core/diagnostics.hpp"
#include "core/source_pos.hpp"
#include "lex/token_kind.hpp"
#include "syntax/ast_nodes.hpp"

#include <string>
#include <vector>

namespace bmfe {

/// Highlighting categories aligned with the B-Minor Phase 1 color specification.
enum class StyleClass {
    Plain,
    Keyword,
    TypeName,
    Variable,
    Function,
    IntLiteral,
    BoolLiteral,
    TextLiteral,
    Operator,
    Comment,
    Error
};

/// Single source slice tagged with its AST-resolved semantic highlighting role.
struct StyledToken {
    std::string text;
    StyleClass style = StyleClass::Plain;
    TokenKind kind = TokenKind::Invalid;
    SourcePos pos;
};

/// AST-driven context-sensitive syntax highlighter.
/// Distinguishes function names (declarations and calls) from variables while
/// preserving every source byte (whitespace, comments, and invalid tokens).
class AstHighlighter {
public:
    std::vector<StyledToken> classify(const std::vector<LexemeToken>& fullStream,
                                      const ProgramUnit* program,
                                      const std::vector<DiagMessage>& errors = {}) const;
};

} // namespace bmfe

#endif // BMFE_HIGHLIGHT_AST_HIGHLIGHTER_HPP
