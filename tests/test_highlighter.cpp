#include "highlight/ast_highlighter.hpp"
#include "lex/scanner.hpp"
#include "render/emitters.hpp"
#include "syntax/rd_parser.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace bmfe;

namespace {

std::vector<StyledToken> runHighlight(const std::string& code) {
    Scanner scanner(code, "hl.bminor");
    ScanOutput scanOut = scanner.scanAll();
    RdParser parser(scanOut.syntacticTokens, "hl.bminor");
    ParseOutput parseOut = parser.parseProgram();

    std::vector<DiagMessage> diags = scanOut.errors;
    diags.insert(diags.end(), parseOut.errors.begin(), parseOut.errors.end());

    AstHighlighter hl;
    return hl.classify(scanOut.stream, parseOut.root.get(), diags);
}

std::string stripAnsiEscapes(const std::string& text) {
    std::string plain;
    plain.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\x1b' && i + 1 < text.size() && text[i + 1] == '[') {
            i += 2;
            while (i < text.size() && text[i] != 'm') {
                ++i;
            }
            continue;
        }
        plain.push_back(text[i]);
    }
    return plain;
}

const StyledToken* nthOccurrence(const std::vector<StyledToken>& list,
                                 const std::string& text,
                                 std::size_t idx = 0) {
    std::size_t count = 0;
    for (const auto& item : list) {
        if (item.text == text) {
            if (count == idx) {
                return &item;
            }
            ++count;
        }
    }
    return nullptr;
}

void verifyContextSensitiveRoles() {
    const std::string code =
        "compute: function integer ( x: integer ) = {\n"
        "    return x * 2;\n"
        "}\n"
        "main: function integer () = {\n"
        "    out: integer = compute(4);\n"
        "    {\n"
        "        compute: integer = 7;\n"
        "        print compute, out;\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    const auto styled = runHighlight(code);
    assert(nthOccurrence(styled, "compute", 0)->style == StyleClass::Function);
    assert(nthOccurrence(styled, "compute", 1)->style == StyleClass::Function);
    assert(nthOccurrence(styled, "compute", 2)->style == StyleClass::Variable);
    assert(nthOccurrence(styled, "compute", 3)->style == StyleClass::Variable);
    assert(nthOccurrence(styled, "x", 0)->style == StyleClass::Variable);
}

void verifyAllCategoriesAndAnsiRoundTrip() {
    const std::string code =
        "// single line comment\n"
        "/* multi\nline comment */\n"
        "arr: array [2] integer = { 10, 20 };\n"
        "emit: function void ( msg: string ) = {\n"
        "    flag: boolean = true;\n"
        "    ch: char = 'z';\n"
        "    if (flag && arr[0] <= 10) {\n"
        "        print \"ok\", ch;\n"
        "    }\n"
        "}\n";

    const auto styled = runHighlight(code);
    assert(nthOccurrence(styled, "// single line comment")->style == StyleClass::Comment);
    assert(nthOccurrence(styled, "/* multi\nline comment */")->style == StyleClass::Comment);
    assert(nthOccurrence(styled, "array")->style == StyleClass::Keyword);
    assert(nthOccurrence(styled, "function")->style == StyleClass::Keyword);
    assert(nthOccurrence(styled, "if")->style == StyleClass::Keyword);
    assert(nthOccurrence(styled, "integer")->style == StyleClass::TypeName);
    assert(nthOccurrence(styled, "void")->style == StyleClass::TypeName);
    assert(nthOccurrence(styled, "boolean")->style == StyleClass::TypeName);
    assert(nthOccurrence(styled, "char")->style == StyleClass::TypeName);
    assert(nthOccurrence(styled, "string")->style == StyleClass::TypeName);
    assert(nthOccurrence(styled, "emit")->style == StyleClass::Function);
    assert(nthOccurrence(styled, "arr")->style == StyleClass::Variable);
    assert(nthOccurrence(styled, "10", 0)->style == StyleClass::IntLiteral);
    assert(nthOccurrence(styled, "true")->style == StyleClass::BoolLiteral);
    assert(nthOccurrence(styled, "'z'")->style == StyleClass::TextLiteral);
    assert(nthOccurrence(styled, "\"ok\"")->style == StyleClass::TextLiteral);
    assert(nthOccurrence(styled, "<=")->style == StyleClass::Operator);

    const std::string ansi = emitAnsiText(styled);
    assert(ansi.find("\x1b[1;34mfunction\x1b[0m") != std::string::npos);
    assert(ansi.find("\x1b[36minteger\x1b[0m") != std::string::npos);
    assert(ansi.find("\x1b[93memit\x1b[0m") != std::string::npos);
    assert(ansi.find("\x1b[38;5;208mtrue\x1b[0m") != std::string::npos);
    assert(stripAnsiEscapes(ansi) == code);
}

void verifyHtmlDocumentOutput() {
    const std::string code =
        "cmp: function boolean ( a: integer, b: integer ) = {\n"
        "    if (a < b && b > 0) {\n"
        "        print \"<a & 'b'>\";\n"
        "        return true;\n"
        "    }\n"
        "    return false;\n"
        "}\n";

    const auto styled = runHighlight(code);
    const std::string html = emitHtmlDocument(styled, "cmp.bminor");

    assert(html.find("<!DOCTYPE html>") != std::string::npos);
    assert(html.find("<style>") != std::string::npos);
    assert(html.find("<pre class=\"bm-source\">") != std::string::npos);
    assert(html.find("<span class=\"kw\">if</span>") != std::string::npos);
    assert(html.find("<span class=\"lit\">true</span>") != std::string::npos);
    assert(html.find("<span class=\"fn\">cmp</span>") != std::string::npos);
    assert(html.find("<span class=\"op\">&lt;</span>") != std::string::npos);
    assert(html.find("<span class=\"op\">&amp;&amp;</span>") != std::string::npos);
    assert(html.find("&quot;&lt;a &amp; &#39;b&#39;&gt;&quot;") != std::string::npos);
    assert(html.find("<script") == std::string::npos);
}

void verifyErrorHighlighting() {
    const std::string code =
        "x: integer = 4$;\n"
        "main: function integer () = {\n"
        "    y: integer = ;\n"
        "    return 0;\n"
        "}\n";

    const auto styled = runHighlight(code);
    assert(nthOccurrence(styled, "$")->style == StyleClass::Error);
    assert(nthOccurrence(styled, ";", 1)->style == StyleClass::Error);

    const std::string ansi = emitAnsiText(styled);
    assert(ansi.find("\x1b[4;91m$\x1b[0m") != std::string::npos);
    assert(stripAnsiEscapes(ansi) == code);

    const std::string html = emitHtmlDocument(styled, "errors.bminor");
    assert(html.find("<span class=\"err\">$</span>") != std::string::npos);
    assert(html.find("<span class=\"err\">;</span>") != std::string::npos);
}

} // namespace

int main() {
    verifyContextSensitiveRoles();
    verifyAllCategoriesAndAnsiRoundTrip();
    verifyHtmlDocumentOutput();
    verifyErrorHighlighting();

    std::cout << "Highlighter and emitter unit tests passed.\n";
    return 0;
}
