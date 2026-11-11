# B-Minor Lexical Rules & Token Specification

This specification documents the regular expressions, token taxonomy, disambiguation policies, and lexical error-recovery strategy implemented by the **B-Minor** front-end scanner (`include/lex/`, `src/lex/`).

---

## 1. Token Data Model

Every token emitted by the scanner (`LexemeToken` in [token_kind.hpp](../include/lex/token_kind.hpp)) records:

| Field | Type | Purpose |
| :--- | :--- | :--- |
| `kind` | `TokenKind` | Token classification (`Kw*`, `Ident`, literals, operators, trivia, `Invalid`, `EndOfFile`) |
| `text` | `std::string` | Exact substring matched in the source buffer |
| `pos` | `SourcePos` | 1-based `(file, line, col)` of the first byte of the token |
| `offset` | `std::size_t` | Zero-based byte offset in the input string |
| `byteLength` | `std::size_t` | Length of `text` in bytes |

---

## 2. Formal Token Categories & Regular Expressions

### 2.1. Reserved Keywords (`Kw*`)
Fifteen words are reserved and cannot serve as user identifiers:
- Control & declarations: `array`, `else`, `for`, `function`, `if`, `print`, `return`, `void`, `while`
- Built-in types: `boolean`, `char`, `integer`, `string`
- Boolean constants: `false`, `true` (mapped to `TokenKind::BoolLit`)

```regex
array|boolean|char|else|false|for|function|if|integer|print|return|string|true|void|while
```

*(Note: `while` is reserved lexically even though B-Minor relies on `for` for iteration.)*

### 2.2. Identifiers (`Ident`)
```regex
[a-zA-Z_][a-zA-Z0-9_]*
```
- Must start with an ASCII letter or `_`.
- Maximum allowed length: **256 characters**. Longer identifiers produce a lexical error and emit `TokenKind::Invalid` without halting the scan.

### 2.3. Integer Literals (`IntLit`)
```regex
[0-9]+
```
- Unsigned decimal integer sequences (e.g., `0`, `42`, `1024`).
- Unary `-` is scanned separately as `OpMinus` so expressions like `n-1` tokenize as `IDENT(n) OP(-) INT(1)`.

### 2.4. Boolean Literals (`BoolLit`)
```regex
true|false
```

### 2.5. String Literals (`StrLit`)
```regex
"([^"\\\r\n]|\\[^\r\n])*"
```
- Supports escape sequences such as `\n`, `\t`, `\r`, `\0`, `\"`, `\\`.
- Maximum decoded character length: **256 characters**.
- Unescaped newlines (`\r`, `\n`) or `EOF` before `"` trigger an unterminated string error.

### 2.6. Character Literals (`CharLit`)
```regex
'([^'\\\r\n]|\\[^\r\n])'
```
- Represents exactly one ASCII byte or escape sequence inside single quotes (`'a'`, `'\n'`, `'\''`).
- Empty (`''`), multi-character (`'xy'`), or unclosed `'...` literals produce a diagnostic and `TokenKind::Invalid`.

### 2.7. Operators
```regex
\+\+|--|<=|>=|==|!=|&&|\|\||[+\-*/%^!<>=]
```
- Unary/postfix: `!`, `-`, `++`, `--`
- Arithmetic & exponentiation: `+`, `-`, `*`, `/`, `%`, `^`
- Relational & equality: `<`, `<=`, `>`, `>=`, `==`, `!=`
- Logical: `&&`, `||`
- Assignment: `=`

### 2.8. Punctuators / Delimiters
```regex
[(){}\[\]:;,]
```
`LParen` `(`, `RParen` `)`, `LBrace` `{`, `RBrace` `}`, `LBracket` `[`, `RBracket` `]`, `Colon` `:`, `Semicolon` `;`, `Comma` `,`.

### 2.9. Single-Line & Block Comments (`LineComment`, `BlockComment`)
- Line comment: `//[^\r\n]*`
- Block comment (non-nesting): `/\*([^*]|\*+[^*/])*\*+/`

### 2.10. Whitespace (`Whitespace`)
- Pattern: `[ \t\r\n]+`
- Consumed while updating `line` (incremented on `\n`, resetting `col = 1`) and `col`.

---

## 3. Precedence & Disambiguation Policies

1. **Maximal Munch (Longest Match):**
   - `<=` produces `OpLessEq` rather than `OpLess` + `OpAssign`.
   - `x+++y` scans as `IDENT(x) OP(++) OP(+) IDENT(y)`.
   - `//` and `/*` take priority over `OpSlash` (`/`).
   - `return1` scans as `IDENT(return1)` rather than `KW(return)` + `INT(1)`.
2. **Keyword-Before-Identifier Priority:**
   - Once an identifier lexeme `[a-zA-Z_][a-zA-Z0-9_]*` finishes scanning, it is checked against the sorted keyword lookup table (`std::lower_bound`) before falling back to `TokenKind::Ident`.

---

## 4. Non-Fatal Lexical Error Recovery

When encountering invalid input, the scanner logs a `DiagMessage` (`DiagPhase::Lexical`) with exact `SourcePos`, emits `TokenKind::Invalid`, and continues:
- **Unknown characters** (e.g., `@`, `$`, or isolated `&` / `|`): emit `Invalid` for that character and advance to the next byte.
- **Unterminated string literal**: stop at `\r`, `\n`, or `EOF`, report the error at the opening quote's position, and resume cleanly.
- **Malformed character literal**: recover to the closing `'` on the same line (or stop before newline/`EOF`) and report the error.
- **Unterminated block comment**: consume through `EOF`, report the error at the opening `/*`, and emit `Invalid` followed by `EndOfFile`.
