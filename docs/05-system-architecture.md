# System Architecture & Implementation Guide

This document describes the module organization, data structures, and extension points of the **B-Minor** Phase 1 Compiler Front-End (`bmfe`).

---

## 1. Pipeline Overview

Orchestrated by [src/main.cpp](../src/main.cpp), source files pass through four stages:

```text
Source Buffer (.bminor)
   │
   ▼  bmfe::Scanner (DFA Scanner)
ScanOutput { stream, syntacticTokens, errors(Lexical) }
   │
   ▼  bmfe::RdParser (LL(1)/LL(2) Recursive Descent + Panic-Mode Recovery)
ParseOutput { ProgramUnit AST, errors(Syntactic) }
   │
   ▼  bmfe::AstHighlighter (AstVisitor + (line, col) Role Map)
std::vector<StyledToken>
   ├──► bmfe::emitAnsiText(...)     ──► ANSI Terminal Output
   └──► bmfe::emitHtmlDocument(...) ──► Standalone HTML5 + Embedded CSS (No JS)
```

---

## 2. Core Modules & Design Choices

### 2.1. Source Coordinates & Diagnostics (`include/core/`)
- **[source_pos.hpp](../include/core/source_pos.hpp):** `SourcePos` tracks `file`, 1-based `line`, and 1-based `col`, formatting positions as `"line:col"` or `"file:line:col"`.
- **[diagnostics.hpp](../include/core/diagnostics.hpp):** `DiagMessage` records `DiagPhase` (`Lexical` or `Syntactic`), `SourcePos`, and an actionable diagnostic message.

### 2.2. DFA Scanner (`include/lex/`, `src/lex/`)
- References: [01-lexical-rules.md](01-lexical-rules.md), [02-scanner-dfa.md](02-scanner-dfa.md).
- Uses a single-pass DFA scanner (`Scanner`) with a compile-time sorted keyword table (`std::lower_bound`), Maximal Munch for multi-character operators, 256-character identifier/string bounds, and non-fatal recovery.
- Emits both `stream` (lossless, including trivia and `Invalid` tokens) and `syntacticTokens` (filtered for `RdParser`).

### 2.3. Abstract Syntax Tree (`include/syntax/ast_nodes.hpp`, `src/syntax/ast_nodes.cpp`)
- Rooted at `AstNode`, every AST node stores `pos` (`SourcePos` of its first token) and `resolvedType` (`std::optional<std::string>`, reserved for Phase 2 semantic type checking).
- Uses `std::unique_ptr` ownership across `Decl`, `Stmt`, `Expr`, `TypeSpec`, `InitNode`, `ParamDecl`, and `ProgramUnit`, plus `MalformedExpr` / `MalformedStmt` recovery placeholders.
- `AstVisitor` decouples AST structure from operations such as `printAst()` and `AstHighlighter`.

### 2.4. Recursive-Descent Parser (`include/syntax/rd_parser.hpp`, `src/syntax/rd_parser.cpp`)
- References: [03-ebnf-grammar.md](03-ebnf-grammar.md), [04-llk-analysis.md](04-llk-analysis.md).
- Parses left-recursion-free EBNF via `LL(1)` recursive descent and one `LL(2)` check (`Ident` followed by `Colon`) in `parseStatement()`.
- Template helper `parseLeftAssoc(...)` factors left-associative binary precedence tiers cleanly, while `parseAssignment()` and `parsePower()` handle right-associative `=` and `^`.
- Panic-mode synchronization (`syncStatement`, `syncDeclaration`, `requireRightParenBeforeBody`) isolates syntax errors without discarding subsequent healthy statements or functions.

### 2.5. AST Highlighter & Emitters (`include/highlight/`, `include/render/`)
- **Context-Sensitive Classification:** `RoleCollector` (`AstVisitor`) populates a `std::map<std::pair<std::size_t, std::size_t>, StyleClass>` mapping `(line, col)` coordinates so `FunctionDecl` and `FuncCall` targets receive `StyleClass::Function` while `VariableDecl`, `ParamDecl`, and `VarRef` receive `StyleClass::Variable`.
- **Category Styling:**

| `StyleClass` | Description | ANSI Sequence | HTML Class |
| :--- | :--- | :--- | :--- |
| `Keyword` | Structural & control keywords | `\x1b[1;34m` | `.kw` |
| `TypeName` | `integer`, `boolean`, `char`, `string`, `void` | `\x1b[36m` | `.type` |
| `Variable` | Variable & parameter identifiers | Default | `.var` |
| `Function` | Function declarations & calls | `\x1b[93m` | `.fn` |
| `IntLiteral` / `BoolLiteral` | Numeric & boolean constants | `\x1b[38;5;208m` | `.lit` |
| `TextLiteral` | String & character literals | `\x1b[32m` | `.str` |
| `Operator` | Unary, binary, postfix, assignment ops | `\x1b[37m` | `.op` |
| `Comment` | `//` and `/* ... */` comments | `\x1b[2;3;90m` | `.cmt` |
| `Error` | `Invalid` tokens & syntax error positions | `\x1b[4;91m` | `.err` |

---

## 3. Extending for Later Phases

- **Semantic Analysis (Phase 2):** Implement a symbol-table and type-checker `AstVisitor` that resolves declarations and populates `AstNode::resolvedType`.
- **Control-Flow / Call Graph Analysis (Phase 3):** Traverse `ProgramUnit` and `CompoundStmt` nodes directly to construct CFGs and call graphs.
