#ifndef BMFE_RENDER_EMITTERS_HPP
#define BMFE_RENDER_EMITTERS_HPP

#include "highlight/ast_highlighter.hpp"

#include <string>
#include <vector>

namespace bmfe {

/// Returns the ANSI SGR escape prefix for a given style class (empty for default/plain).
std::string ansiCodeForStyle(StyleClass style);

/// Returns the CSS class name for a given style class (empty for unstyled plain text).
std::string cssClassForStyle(StyleClass style);

/// Escapes HTML special characters (&, <, >, ", ').
std::string sanitizeHtml(const std::string& raw);

/// Emits ANSI-colored terminal text preserving all whitespace and source bytes.
std::string emitAnsiText(const std::vector<StyledToken>& tokens);

/// Emits a self-contained HTML5/CSS document with a <pre> code block and zero JavaScript.
std::string emitHtmlDocument(const std::vector<StyledToken>& tokens,
                             const std::string& pageTitle = "B-Minor Highlighted Source");

} // namespace bmfe

#endif // BMFE_RENDER_EMITTERS_HPP
