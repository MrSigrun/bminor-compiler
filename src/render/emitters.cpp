#include "render/emitters.hpp"

#include <sstream>

namespace bmfe {

std::string ansiCodeForStyle(StyleClass style) {
    switch (style) {
        case StyleClass::Keyword:     return "\x1b[1;34m";
        case StyleClass::TypeName:    return "\x1b[36m";
        case StyleClass::Variable:    return "";
        case StyleClass::Function:    return "\x1b[93m";
        case StyleClass::IntLiteral:  return "\x1b[38;5;208m";
        case StyleClass::BoolLiteral: return "\x1b[38;5;208m";
        case StyleClass::TextLiteral: return "\x1b[32m";
        case StyleClass::Operator:    return "\x1b[37m";
        case StyleClass::Comment:     return "\x1b[2;3;90m";
        case StyleClass::Error:       return "\x1b[4;91m";
        case StyleClass::Plain:       return "";
    }
    return "";
}

std::string cssClassForStyle(StyleClass style) {
    switch (style) {
        case StyleClass::Keyword:     return "kw";
        case StyleClass::TypeName:    return "type";
        case StyleClass::Variable:    return "var";
        case StyleClass::Function:    return "fn";
        case StyleClass::IntLiteral:  return "lit";
        case StyleClass::BoolLiteral: return "lit";
        case StyleClass::TextLiteral: return "str";
        case StyleClass::Operator:    return "op";
        case StyleClass::Comment:     return "cmt";
        case StyleClass::Error:       return "err";
        case StyleClass::Plain:       return "";
    }
    return "";
}

std::string sanitizeHtml(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (const char ch : raw) {
        switch (ch) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += ch;       break;
        }
    }
    return out;
}

std::string emitAnsiText(const std::vector<StyledToken>& tokens) {
    std::ostringstream oss;
    for (const auto& tok : tokens) {
        const std::string code = ansiCodeForStyle(tok.style);
        if (code.empty()) {
            oss << tok.text;
        } else {
            oss << code << tok.text << "\x1b[0m";
        }
    }
    return oss.str();
}

std::string emitHtmlDocument(const std::vector<StyledToken>& tokens,
                             const std::string& pageTitle) {
    const std::string safeTitle = sanitizeHtml(pageTitle);
    std::ostringstream oss;
    oss << "<!DOCTYPE html>\n"
        << "<html lang=\"en\">\n"
        << "<head>\n"
        << "  <meta charset=\"UTF-8\">\n"
        << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        << "  <title>" << safeTitle << "</title>\n"
        << "  <style>\n"
        << "    body { margin: 0; padding: 28px; background: #161821; color: #e2e8f0; font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", sans-serif; }\n"
        << "    .frame { max-width: 980px; margin: 0 auto; background: #1e222d; border: 1px solid #2e3446; border-radius: 10px; overflow: hidden; box-shadow: 0 8px 24px rgba(0,0,0,0.35); }\n"
        << "    .titlebar { padding: 10px 18px; background: #141720; border-bottom: 1px solid #2e3446; font-size: 13px; color: #94a3b8; }\n"
        << "    pre.bm-source { margin: 0; padding: 18px; overflow-x: auto; font-family: \"JetBrains Mono\", Consolas, \"Courier New\", monospace; font-size: 14px; line-height: 1.6; }\n"
        << "    .kw { color: #569cd6; font-weight: 700; }\n"
        << "    .type { color: #4ec9b0; }\n"
        << "    .var { color: #e2e8f0; }\n"
        << "    .fn { color: #dcdcaa; }\n"
        << "    .lit { color: #ce9178; }\n"
        << "    .str { color: #89d185; }\n"
        << "    .op { color: #cbd5e1; }\n"
        << "    .cmt { color: #6a737d; font-style: italic; }\n"
        << "    .err { color: #f87171; text-decoration: underline wavy #ef4444; }\n"
        << "  </style>\n"
        << "</head>\n"
        << "<body>\n"
        << "  <div class=\"frame\">\n"
        << "    <div class=\"titlebar\">" << safeTitle << "</div>\n"
        << "    <pre class=\"bm-source\"><code>";

    for (const auto& tok : tokens) {
        const std::string escaped = sanitizeHtml(tok.text);
        const std::string cls = cssClassForStyle(tok.style);
        if (cls.empty()) {
            oss << escaped;
        } else {
            oss << "<span class=\"" << cls << "\">" << escaped << "</span>";
        }
    }

    oss << "</code></pre>\n"
        << "  </div>\n"
        << "</body>\n"
        << "</html>\n";
    return oss.str();
}

} // namespace bmfe
