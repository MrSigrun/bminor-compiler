#ifndef BMFE_CORE_SOURCE_POS_HPP
#define BMFE_CORE_SOURCE_POS_HPP

#include <cstddef>
#include <string>

namespace bmfe {

/// 1-based line and column coordinate inside a B-Minor source file.
struct SourcePos {
    std::string file;
    std::size_t line = 1;
    std::size_t col = 1;

    /// Returns "<line>:<col>".
    std::string lineCol() const {
        return std::to_string(line) + ":" + std::to_string(col);
    }

    /// Returns "<file>:<line>:<col>" (or "<line>:<col>" when file is empty).
    std::string format() const {
        if (file.empty()) {
            return lineCol();
        }
        return file + ":" + lineCol();
    }
};

} // namespace bmfe

#endif // BMFE_CORE_SOURCE_POS_HPP
