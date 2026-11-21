#include "highlight/ast_highlighter.hpp"
#include "lex/scanner.hpp"
#include "render/emitters.hpp"
#include "syntax/ast_nodes.hpp"
#include "syntax/rd_parser.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

enum class RunAction {
    Tokens,
    AstDump,
    AnsiHighlight,
    HtmlHighlight
};

bool loadSourceFile(const std::string& path, std::string& outText) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    outText = ss.str();
    return true;
}

bool saveOutputFile(const std::string& path, const std::string& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file << data;
    return file.good();
}

void showHelp() {
    std::cout << "B-Minor Front-End & Syntax Highlighter\n"
              << "Usage:\n"
              << "  bminor [--ansi] <source.bminor>\n"
              << "  bminor --html <source.bminor> [-o <output.html>]\n"
              << "  bminor --lex <source.bminor>\n"
              << "  bminor [--ast | --parse] <source.bminor>\n";
}

void writeDiagnostics(const std::vector<bmfe::DiagMessage>& diags) {
    for (const auto& d : diags) {
        std::cerr << d.format() << '\n';
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        showHelp();
        return 0;
    }

    RunAction action = RunAction::AnsiHighlight;
    std::string inputPath;
    std::string outputPath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            showHelp();
            return 0;
        } else if (arg == "--lex") {
            action = RunAction::Tokens;
        } else if (arg == "--ast" || arg == "--parse") {
            action = RunAction::AstDump;
        } else if (arg == "--ansi" || arg == "--highlight-ansi") {
            action = RunAction::AnsiHighlight;
        } else if (arg == "--html" || arg == "--highlight-html") {
            action = RunAction::HtmlHighlight;
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Error: unrecognized flag '" << arg << "'\n";
            return 1;
        } else {
            inputPath = arg;
        }
    }

    if (inputPath.empty()) {
        std::cerr << "Error: no input file specified.\n";
        showHelp();
        return 1;
    }

    std::string source;
    if (!loadSourceFile(inputPath, source)) {
        std::cerr << "Error: unable to open '" << inputPath << "'\n";
        return 1;
    }

    bmfe::Scanner scanner(source, inputPath);
    bmfe::ScanOutput scanOut = scanner.scanAll();

    if (action == RunAction::Tokens) {
        for (std::size_t i = 0; i < scanOut.syntacticTokens.size(); ++i) {
            if (i > 0) {
                std::cout << ' ';
            }
            std::cout << bmfe::formatLexemeToken(scanOut.syntacticTokens[i]);
        }
        std::cout << '\n';
        writeDiagnostics(scanOut.errors);
        return scanOut.errors.empty() ? 0 : 1;
    }

    bmfe::RdParser parser(scanOut.syntacticTokens, inputPath);
    bmfe::ParseOutput parseOut = parser.parseProgram();

    if (action == RunAction::AstDump) {
        if (parseOut.root) {
            std::cout << bmfe::printAst(*parseOut.root);
        }
        writeDiagnostics(scanOut.errors);
        writeDiagnostics(parseOut.errors);
        return (scanOut.errors.empty() && parseOut.ok()) ? 0 : 1;
    }

    std::vector<bmfe::DiagMessage> allDiags = scanOut.errors;
    allDiags.insert(allDiags.end(), parseOut.errors.begin(), parseOut.errors.end());

    bmfe::AstHighlighter highlighter;
    const auto styled = highlighter.classify(scanOut.stream, parseOut.root.get(), allDiags);

    if (action == RunAction::AnsiHighlight) {
        std::cout << bmfe::emitAnsiText(styled);
        if (!source.empty() && source.back() != '\n') {
            std::cout << '\n';
        }
    } else if (action == RunAction::HtmlHighlight) {
        const std::string html = bmfe::emitHtmlDocument(styled, inputPath);
        if (!outputPath.empty()) {
            if (!saveOutputFile(outputPath, html)) {
                std::cerr << "Error: failed to write '" << outputPath << "'\n";
                return 1;
            }
        } else {
            std::cout << html;
        }
    }

    writeDiagnostics(allDiags);
    return allDiags.empty() ? 0 : 1;
}
