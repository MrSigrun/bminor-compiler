# B-Minor Compiler Front-End & AST Syntax Highlighter

A zero-dependency **C++17** compiler front-end for **B-Minor** (Amirkabir University of Technology — Compiler Design Course, Phase 1), providing lexical scanning, recursive-descent parsing, AST construction, panic-mode error recovery, and AST-level syntax highlighting in both **ANSI Terminal** and **standalone HTML/CSS** formats.

---

## Pipeline Architecture

```text
Source (.bminor)
   │
   ▼  bmfe::Scanner (DFA, O(n), 1-char lookahead)
ScanOutput { stream, syntacticTokens, lexical errors }
   │
   ▼  bmfe::RdParser (LL(1)/LL(2) Recursive Descent + Panic-Mode Recovery)
ParseOutput { ProgramUnit AST, syntax errors }
   │
   ▼  bmfe::AstHighlighter (AstVisitor + (line, col) role table)
StyledToken Stream
   ├──► emitAnsiText(...)     ──► ANSI Terminal Output
   └──► emitHtmlDocument(...) ──► Self-Contained HTML5 + CSS (No JS)
```

---

## Project Layout

```text
bminor-compiler/
├── Makefile
├── README.md
├── docs/
│   ├── 01-lexical-rules.md       # Token regexes, 15 keywords, priority & recovery rules
│   ├── 02-scanner-dfa.md         # DFA scanner design & dual token streams
│   ├── 03-ebnf-grammar.md        # Formal left-recursion-free EBNF grammar
│   ├── 04-llk-analysis.md        # LL(1)/LL(2) FIRST sets & recovery sync points
│   └── 05-system-architecture.md # End-to-end architecture & extension notes
├── include/
│   ├── core/                     # SourcePos & DiagMessage
│   ├── lex/                      # TokenKind, LexemeToken & Scanner
│   ├── syntax/                   # AstNode hierarchy, AstVisitor & RdParser
│   ├── highlight/                # AstHighlighter & StyleClass
│   └── render/                   # ANSI & HTML/CSS emitters
├── src/
│   ├── main.cpp                  # CLI entry point
│   ├── lex/                      # DFA scanner & token formatting
│   ├── syntax/                   # AST printer & recursive-descent parser
│   ├── highlight/                # AST-based context classifier
│   └── render/                   # ANSI SGR & HTML5/CSS output emitters
└── tests/
    ├── test_scanner.cpp          # Scanner unit tests
    ├── test_parser.cpp           # Parser & AST unit tests
    ├── test_highlighter.cpp      # Highlighter & emitter unit tests
    ├── test_pipeline.cpp         # End-to-end integration tests
    └── samples/
        ├── valid/                # fibonacci.bminor, matrix_ops.bminor
        └── invalid/              # lex_issues.bminor, syntax_issues.bminor, full_recovery.bminor
```

---

## Building & Testing

Requires a **C++17** compiler (`g++` or `clang++`) and **GNU Make** (`mingw32-make` on Windows, `make` on Linux/macOS). No external libraries are needed.

**Windows (PowerShell / MSYS2):**
```powershell
mingw32-make clean
mingw32-make all
mingw32-make test
```

**Linux / macOS:**
```bash
make clean
make all
make test
```

Running `mingw32-make test` executes all four suites:
```text
Scanner unit tests passed.
Parser and AST unit tests passed.
Highlighter and emitter unit tests passed.
End-to-end pipeline tests passed.
```

---

## CLI Usage & Examples

```text
Usage:
  bminor [--ansi] <source.bminor>
  bminor --html <source.bminor> [-o <output.html>]
  bminor --lex <source.bminor>
  bminor [--ast | --parse] <source.bminor>
```

### 1. Token Stream (`--lex`)
```powershell
.\build\bminor.exe --lex tests\samples\valid\fibonacci.bminor
```
Output:
```text
IDENT(fib) COLON KW(function) KW(integer) LPAREN IDENT(n) COLON KW(integer) RPAREN ASSIGN LBRACE KW(if) LPAREN IDENT(n) OP(<=) INT(1) RPAREN KW(return) IDENT(n) SEMI KW(return) IDENT(fib) LPAREN IDENT(n) OP(-) INT(1) RPAREN OP(+) IDENT(fib) LPAREN IDENT(n) OP(-) INT(2) RPAREN SEMI RBRACE IDENT(main) COLON KW(function) KW(integer) LPAREN RPAREN ASSIGN LBRACE IDENT(res) COLON KW(integer) ASSIGN IDENT(fib) LPAREN INT(7) RPAREN SEMI KW(print) STRING("fib(7) = ") COMMA IDENT(res) COMMA STRING("\n") SEMI KW(return) INT(0) SEMI RBRACE EOF
```

### 2. Abstract Syntax Tree (`--ast`)
```powershell
.\build\bminor.exe --ast tests\samples\valid\fibonacci.bminor
```
Output:
```text
ProgramUnit [pos=2:1]
  decl[0]: FunctionDecl(name='fib', kind=definition, pos=2:1)
    returnType: ScalarType(integer, pos=2:15)
    param[0]: ParamDecl(name='n', pos=2:25)
      type: ScalarType(integer, pos=2:28)
    body: CompoundStmt(pos=2:40)
      stmt[0]: IfElseStmt(pos=3:5)
        cond: BinaryOp(op='<=', pos=3:9)
          left: VarRef(name='n', pos=3:9)
          right: IntegerLit(val=1, pos=3:14)
        then: ReturnStmt(pos=3:17)
          value: VarRef(name='n', pos=3:24)
      stmt[1]: ReturnStmt(pos=4:5)
        value: BinaryOp(op='+', pos=4:12)
          left: FuncCall(callee='fib', pos=4:12)
            arg[0]: BinaryOp(op='-', pos=4:16)
              left: VarRef(name='n', pos=4:16)
              right: IntegerLit(val=1, pos=4:20)
          right: FuncCall(callee='fib', pos=4:25)
            arg[0]: BinaryOp(op='-', pos=4:29)
              left: VarRef(name='n', pos=4:29)
              right: IntegerLit(val=2, pos=4:33)
  ...
```

### 3. ANSI & Standalone HTML Output (`--ansi`, `--html`)
```powershell
.\build\bminor.exe --ansi tests\samples\valid\fibonacci.bminor
.\build\bminor.exe --html tests\samples\valid\fibonacci.bminor -o build\fibonacci.html
```

### 4. Error Recovery (`full_recovery.bminor`)
```powershell
.\build\bminor.exe --ansi tests\samples\invalid\full_recovery.bminor
```
Produces lexical and syntactic diagnostics on `stderr` while rendering the full recovered source to `stdout` and exiting with status `1`.

---

## Color Mapping Reference

| Construct Category | Visual Style | ANSI Sequence | HTML Class |
| :--- | :--- | :--- | :--- |
| Keywords (`if`, `return`, `function`, `array`, ...) | Bold blue | `\x1b[1;34m` | `.kw` |
| Type names (`integer`, `boolean`, `char`, `string`, `void`) | Teal / cyan | `\x1b[36m` | `.type` |
| Variable & parameter identifiers | Default / light | Default color | `.var` |
| Function names (declarations & calls) | Yellow / gold | `\x1b[93m` | `.fn` |
| Integer & boolean literals | Orange | `\x1b[38;5;208m` | `.lit` |
| String & character literals | Warm green | `\x1b[32m` | `.str` |
| Operators | Light gray | `\x1b[37m` | `.op` |
| Comments (`//`, `/* ... */`) | Dim gray, italic | `\x1b[2;3;90m` | `.cmt` |
| Errors & `Invalid` tokens | Red underline | `\x1b[4;91m` | `.err` |
