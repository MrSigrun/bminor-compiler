#include "lex/scanner.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string renderTokenStream(const std::vector<bmfe::LexemeToken>& tokens) {
    std::ostringstream ss;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) {
            ss << ' ';
        }
        ss << bmfe::formatLexemeToken(tokens[i]);
    }
    return ss.str();
}

std::string joinRawStream(const std::vector<bmfe::LexemeToken>& stream) {
    std::string raw;
    for (const auto& tok : stream) {
        raw += tok.text;
    }
    return raw;
}

void verifyFibonacciTokens() {
    const std::string code =
        "fib: function integer ( n: integer ) = {\n"
        "    if (n <= 1) return n;\n"
        "    return fib(n - 1) + fib(n - 2);\n"
        "}\n";

    bmfe::Scanner scanner(code, "fib.bminor");
    bmfe::ScanOutput out = scanner.scanAll();

    assert(out.errors.empty());
    assert(joinRawStream(out.stream) == code);
    const std::string expected =
        "IDENT(fib) COLON KW(function) KW(integer) LPAREN IDENT(n) COLON "
        "KW(integer) RPAREN ASSIGN LBRACE KW(if) LPAREN IDENT(n) OP(<=) INT(1) "
        "RPAREN KW(return) IDENT(n) SEMI KW(return) IDENT(fib) LPAREN IDENT(n) "
        "OP(-) INT(1) RPAREN OP(+) IDENT(fib) LPAREN IDENT(n) OP(-) INT(2) "
        "RPAREN SEMI RBRACE EOF";
    assert(renderTokenStream(out.syntacticTokens) == expected);

    bool foundCallPos = false;
    for (const auto& tok : out.syntacticTokens) {
        if (tok.text == "fib" && tok.pos.line == 3 && tok.pos.col == 12) {
            foundCallPos = true;
        }
    }
    assert(foundCallPos);
}

void verifyKeywordsVsIdentifiers() {
    const std::string code =
        "array boolean char else false for function if integer "
        "print return string true void while "
        "if_flag _while array2 true_val false1";

    bmfe::Scanner scanner(code);
    bmfe::ScanOutput out = scanner.scanAll();
    assert(out.errors.empty());

    const std::vector<bmfe::TokenKind> expected = {
        bmfe::TokenKind::KwArray,
        bmfe::TokenKind::KwBoolean,
        bmfe::TokenKind::KwChar,
        bmfe::TokenKind::KwElse,
        bmfe::TokenKind::BoolLit,
        bmfe::TokenKind::KwFor,
        bmfe::TokenKind::KwFunction,
        bmfe::TokenKind::KwIf,
        bmfe::TokenKind::KwInteger,
        bmfe::TokenKind::KwPrint,
        bmfe::TokenKind::KwReturn,
        bmfe::TokenKind::KwString,
        bmfe::TokenKind::BoolLit,
        bmfe::TokenKind::KwVoid,
        bmfe::TokenKind::KwWhile,
        bmfe::TokenKind::Ident,
        bmfe::TokenKind::Ident,
        bmfe::TokenKind::Ident,
        bmfe::TokenKind::Ident,
        bmfe::TokenKind::Ident,
        bmfe::TokenKind::EndOfFile
    };
    assert(out.syntacticTokens.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        assert(out.syntacticTokens[i].kind == expected[i]);
    }
}

void verifyMaximalMunchAndSymbols() {
    const std::string code = "a<=b>=c==d!=e&&f||g++h--i+j-k*l/m%n^!o=p(){}[]:;,";
    bmfe::Scanner scanner(code);
    bmfe::ScanOutput out = scanner.scanAll();
    assert(out.errors.empty());

    const std::string expected =
        "IDENT(a) OP(<=) IDENT(b) OP(>=) IDENT(c) OP(==) IDENT(d) OP(!=) IDENT(e) "
        "OP(&&) IDENT(f) OP(||) IDENT(g) OP(++) IDENT(h) OP(--) IDENT(i) OP(+) "
        "IDENT(j) OP(-) IDENT(k) OP(*) IDENT(l) OP(/) IDENT(m) OP(%) IDENT(n) "
        "OP(^) OP(!) IDENT(o) ASSIGN IDENT(p) "
        "LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET COLON SEMI COMMA EOF";
    assert(renderTokenStream(out.syntacticTokens) == expected);

    bmfe::Scanner munch("x+++y");
    const auto munchOut = munch.scanAll();
    assert(munchOut.errors.empty());
    assert(renderTokenStream(munchOut.syntacticTokens) == "IDENT(x) OP(++) OP(+) IDENT(y) EOF");
}

void verifyLengthBoundsAndEscapes() {
    const std::string validId(256, 'v');
    const std::string longId(257, 'w');
    const std::string validStr = "\"" + std::string(256, 's') + "\"";
    const std::string longStr = "\"" + std::string(257, 't') + "\"";

    bmfe::Scanner scanner(validId + " " + longId + " " + validStr + " " + longStr + " 'a' '\\n' '\\''");
    const auto out = scanner.scanAll();

    assert(out.errors.size() == 2);
    assert(out.syntacticTokens.size() == 8); // 4 above + 3 chars + EOF
    assert(out.syntacticTokens[0].kind == bmfe::TokenKind::Ident);
    assert(out.syntacticTokens[1].kind == bmfe::TokenKind::Invalid);
    assert(out.syntacticTokens[2].kind == bmfe::TokenKind::StrLit);
    assert(out.syntacticTokens[3].kind == bmfe::TokenKind::Invalid);
    assert(out.syntacticTokens[4].kind == bmfe::TokenKind::CharLit);
    assert(out.syntacticTokens[5].kind == bmfe::TokenKind::CharLit);
    assert(out.syntacticTokens[6].kind == bmfe::TokenKind::CharLit);
}

void verifyLexicalRecovery() {
    const std::string code =
        "a: integer = 5$;\n"
        "s: string = \"unclosed\n"
        "c1: char = '';\n"
        "c2: char = 'xy';\n"
        "b: boolean = x & y | z;\n"
        "/* unclosed block";

    bmfe::Scanner scanner(code, "bad.bminor");
    const auto out = scanner.scanAll();

    assert(out.errors.size() == 7);
    assert(joinRawStream(out.stream) == code);
    assert(out.syntacticTokens.back().kind == bmfe::TokenKind::EndOfFile);

    bool sawInvalidDollar = false;
    bool sawZ = false;
    for (const auto& tok : out.syntacticTokens) {
        if (tok.kind == bmfe::TokenKind::Invalid && tok.text == "$" &&
            tok.pos.line == 1 && tok.pos.col == 15) {
            sawInvalidDollar = true;
        }
        if (tok.kind == bmfe::TokenKind::Ident && tok.text == "z" && tok.pos.line == 5) {
            sawZ = true;
        }
    }
    assert(sawInvalidDollar && sawZ);
}

} // namespace

int main() {
    verifyFibonacciTokens();
    verifyKeywordsVsIdentifiers();
    verifyMaximalMunchAndSymbols();
    verifyLengthBoundsAndEscapes();
    verifyLexicalRecovery();

    std::cout << "Scanner unit tests passed.\n";
    return 0;
}
