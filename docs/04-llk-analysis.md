# LL(k) Parser Compatibility & Recovery Analysis

This document verifies that the grammar in [03-ebnf-grammar.md](03-ebnf-grammar.md) contains no left recursion and is deterministically parsed by a top-down **Recursive-Descent Parser** (`RdParser` in [rd_parser.hpp](../include/syntax/rd_parser.hpp)).

---

## 1. Parsing Strategy

- **Approach:** Hand-written **Recursive Descent** with one parsing method per non-terminal.
- **Lookahead Bound:** **`LL(1)`** across all rules except one **`LL(2)`** decision in `parseStatement()`: when the current token is `IDENT`, inspecting `peek(1) == Colon` immediately distinguishes `local_var_decl` from `expr_stmt` without backtracking.

---

## 2. Left-Recursion Removal

| Grammar Group | Technique Used |
| :--- | :--- |
| `program`, `block_stmt` | `while` loops over `decl*` and `stmt*` bounded by `EndOfFile` / `}` |
| `param_list`, `expr_list`, `initializer` | Iterative comma-separated loops `( ',' item )*` |
| `logic_or`, `logic_and`, `relation`, `sum`, `product` | Left-associative iteration `( op rhs )*` via `parseLeftAssoc(...)` |
| `assignment`, `power` | Right-associative tail recursion (`a = b = 1`, `a ^ b ^ c`) |
| `unary` | Prefix operators (`-`, `!`) consumed before recursive call |
| `postfix` | Factored as `atom postfix_suffix*` so `arr[i](x)++` iterates left-to-right |

---

## 3. `FIRST` Sets & Deterministic Branching

### Key `FIRST` Sets
- `FIRST(decl)` = `{ IDENT }`
- `FIRST(func_decl_tail)` = `{ 'function' }`
- `FIRST(var_decl_tail)` = `FIRST(type_spec)` = `{ 'integer', 'boolean', 'char', 'string', 'array' }`
- `FIRST(return_type)` = `FIRST(type_spec) ∪ { 'void' }`
- `FIRST(stmt)` = `{ 'if', 'for', 'return', 'print', '{', IDENT, '-', '!', '(', INT_LIT, BOOL_LIT, CHAR_LIT, STR_LIT }`
- `FIRST(postfix_suffix)` = `{ '[', '(', '++', '--' }`

### Conflict Resolution
1. **Top-level declarations (`decl`):** Factored on common prefix `IDENT ':'`; `FIRST(func_decl_tail) ∩ FIRST(var_decl_tail) = ∅` → `LL(1)`.
2. **Statements starting with `IDENT`:**
   - `peek(0) == Ident && peek(1) == Colon` → `parseLocalDeclStmt()` (`LL(2)`).
   - Otherwise → `parseExprStmt()`.
3. **Dangling `else`:** `parseIfStmt()` greedily matches `'else'` immediately after the `then` branch, binding `else` to the nearest enclosing `if`.
4. **Optional clauses (`for`, `return`, `print`, `array_type`):** Every optional expression's `FIRST` set is disjoint from its following delimiter (`;`, `)`, `]`).

---

## 4. Panic-Mode Error Recovery Sync Points

To report multiple errors cleanly in one pass without crashing:
- **Statement sync (`syncStatement`):** Skips tokens until `;` (consumed), `}` (preserved), `{` (preserved), statement starters (`if`, `for`, `return`, `print`), `IDENT ':'`, or `EndOfFile`.
- **Missing `)` before `{`:** When `if (...)` or `for (...)` omits `)` right before `{`, `RdParser` emits `"missing ')' before '{'"` and leaves `{` intact so the block body parses normally.
- **Declaration sync (`syncDeclaration`):** Tracks `{` / `}` brace nesting so broken top-level declarations skip cleanly to `;` or matching `}` at depth `0`, or the next `IDENT ':'`.
- **Progress invariants:** Outer `parse()` and `parseBlockStmt()` loops verify cursor advancement to guarantee $O(n)$ termination on arbitrary invalid token sequences.
