#include "highlight/ast_highlighter.hpp"
#include "lex/scanner.hpp"
#include "render/emitters.hpp"
#include "syntax/ast_nodes.hpp"
#include "syntax/rd_parser.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace bmfe;

namespace {

std::string readSampleFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    assert(in.good());
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string removeAnsi(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\x1b' && i + 1 < input.size() && input[i + 1] == '[') {
            i += 2;
            while (i < input.size() && input[i] != 'm') {
                ++i;
            }
            continue;
        }
        out.push_back(input[i]);
    }
    return out;
}

struct PipelineResult {
    ScanOutput scan;
    ParseOutput parse;
    std::vector<DiagMessage> allErrors;
    std::vector<StyledToken> styled;
    std::string ansi;
    std::string html;
};

PipelineResult executePipeline(const std::string& source, const std::string& filename) {
    PipelineResult res;
    Scanner scanner(source, filename);
    res.scan = scanner.scanAll();

    RdParser parser(res.scan.syntacticTokens, filename);
    res.parse = parser.parseProgram();

    res.allErrors = res.scan.errors;
    res.allErrors.insert(res.allErrors.end(), res.parse.errors.begin(), res.parse.errors.end());

    AstHighlighter hl;
    res.styled = hl.classify(res.scan.stream, res.parse.root.get(), res.allErrors);
    res.ansi = emitAnsiText(res.styled);
    res.html = emitHtmlDocument(res.styled, filename);
    return res;
}

void checkValidFixture(const std::string& path) {
    const std::string src = readSampleFile(path);
    const PipelineResult res = executePipeline(src, path);
    assert(res.allErrors.empty());
    assert(res.parse.root != nullptr && !res.parse.root->declarations.empty());
    assert(removeAnsi(res.ansi) == src);
    assert(res.html.find("<!DOCTYPE html>") != std::string::npos);
    assert(res.html.find("<pre class=\"bm-source\">") != std::string::npos);
    assert(res.html.find("<script") == std::string::npos);
}

void checkInvalidFixture(const std::string& path, bool expectLexErr, bool expectSynErr) {
    const std::string src = readSampleFile(path);
    const PipelineResult res = executePipeline(src, path);
    assert(res.parse.root != nullptr);
    if (expectLexErr) {
        assert(!res.scan.errors.empty());
    }
    if (expectSynErr) {
        assert(!res.parse.errors.empty());
    }
    assert(removeAnsi(res.ansi) == src);
    assert(res.html.find("<span class=\"err\">") != std::string::npos);
    assert(res.html.find("<script") == std::string::npos);
}

void checkDynamicGeneratedProgram() {
    std::ostringstream ss;
    for (int i = 0; i < 5; ++i) {
        ss << "helper_" << i << ": function integer ( p_" << i << ": integer ) = {\n"
           << "    v_" << i << ": integer = p_" << i << " + " << (i + 1) << ";\n"
           << "    return v_" << i << ";\n"
           << "}\n";
    }
    ss << "driver: function integer () = {\n"
       << "    sum: integer = 0;\n";
    for (int i = 0; i < 5; ++i) {
        ss << "    sum = sum + helper_" << i << "(" << i << ");\n";
    }
    ss << "    return sum;\n}\n";

    const PipelineResult res = executePipeline(ss.str(), "dynamic.bminor");
    assert(res.allErrors.empty());
    assert(res.parse.root->declarations.size() == 6);
    assert(removeAnsi(res.ansi) == ss.str());
    assert(res.html.find("<span class=\"fn\">helper_4</span>") != std::string::npos);
    assert(res.html.find("<span class=\"var\">sum</span>") != std::string::npos);
}

} // namespace

int main() {
    checkValidFixture("tests/samples/valid/fibonacci.bminor");
    checkValidFixture("tests/samples/valid/matrix_ops.bminor");

    checkInvalidFixture("tests/samples/invalid/lex_issues.bminor", true, true);
    checkInvalidFixture("tests/samples/invalid/syntax_issues.bminor", false, true);
    checkInvalidFixture("tests/samples/invalid/full_recovery.bminor", true, true);

    const PipelineResult mixed = executePipeline(
        readSampleFile("tests/samples/invalid/full_recovery.bminor"),
        "tests/samples/invalid/full_recovery.bminor");
    bool sawHealthyRoutine = false;
    for (const auto& decl : mixed.parse.root->declarations) {
        if (const auto* fn = dynamic_cast<const FunctionDecl*>(decl.get())) {
            if (fn->name == "healthy_routine") {
                sawHealthyRoutine = true;
            }
        }
    }
    assert(sawHealthyRoutine);

    checkDynamicGeneratedProgram();

    std::cout << "End-to-end pipeline tests passed.\n";
    return 0;
}
