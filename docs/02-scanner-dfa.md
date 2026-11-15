# Scanner Design — Deterministic Finite Automaton (DFA)

This document outlines the internal design of `bmfe::Scanner` ([scanner.hpp](../include/lex/scanner.hpp), [scanner.cpp](../src/lex/scanner.cpp)).

---

## 1. DFA Architecture

Each regular expression from [01-lexical-rules.md](01-lexical-rules.md) corresponds to an NFA (via Thompson's construction), whose union determinizes and minimizes (via subset construction and Hopcroft's algorithm) into a single DFA. `Scanner` implements this minimized DFA directly in C++17:

- **Single-Pass $O(n)$ Scan:** Maintains a forward-only byte index `index_`, 1-based `line_`, and 1-based `col_` with at most 1-character lookahead (`peekChar(1)`).
- **Sorted Keyword Table:** Identifiers matching `[a-zA-Z_][a-zA-Z0-9_]*` are resolved against a compile-time sorted `std::array<KeywordEntry, 15>` via `std::lower_bound` to enforce keyword priority over identifiers.
- **Maximal Munch:** Multi-character operators (`++`, `--`, `<=`, `>=`, `==`, `!=`, `&&`, `||`) and comment openers (`//`, `/*`) check `peekChar()` before falling back to single-character tokens.

---

## 2. Dual-Stream `ScanOutput`

`Scanner::scanAll()` returns a `ScanOutput` structure:

1. `stream`: Complete, lossless vector of every `LexemeToken` (`Whitespace`, `LineComment`, `BlockComment`, `Invalid`, syntactic tokens, and trailing `EndOfFile`). Concatenating `tok.text` across `stream` reconstructs the input file verbatim.
2. `syntacticTokens`: Filtered token stream omitting whitespace and comments (retaining `Invalid` and `EndOfFile`), ready for the parser.
3. `errors`: Non-fatal `DiagMessage` list (`DiagPhase::Lexical`) pinpointing any invalid characters, oversized identifiers/strings, or unclosed literals/comments.
