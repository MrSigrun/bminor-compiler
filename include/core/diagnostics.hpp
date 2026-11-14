#ifndef BMFE_CORE_DIAGNOSTICS_HPP
#define BMFE_CORE_DIAGNOSTICS_HPP

#include "core/source_pos.hpp"

#include <string>

namespace bmfe {

enum class DiagPhase {
    Lexical,
    Syntactic
};

/// Compiler front-end diagnostic entry with phase, source coordinate, and explanation.
struct DiagMessage {
    DiagPhase phase = DiagPhase::Lexical;
    SourcePos pos;
    std::string text;

    std::string format() const {
        const std::string tag = (phase == DiagPhase::Lexical) ? "Lexical error" : "Syntax error";
        return tag + " at " + pos.format() + ": " + text;
    }
};

} // namespace bmfe

#endif // BMFE_CORE_DIAGNOSTICS_HPP
