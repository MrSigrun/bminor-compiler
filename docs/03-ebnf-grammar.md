# B-Minor Formal Grammar (EBNF)

This document defines the complete context-free grammar for **B-Minor** using **Extended Backus-Naur Form (EBNF)**, factored to eliminate left recursion and support deterministic top-down recursive-descent parsing.

---

## 1. EBNF Notation

- `rule ::= ...` — Non-terminal production (`snake_case` names)
- `A B` — Sequence
- `A | B` — Alternation (choice)
- `A?` — Optional (0 or 1)
- `A*` — Repetition (0 or more)
- `( ... )` — Grouping
- `'...'` — Concrete keyword, operator, or delimiter terminal
- `IDENT`, `INT_LIT`, `BOOL_LIT`, `CHAR_LIT`, `STR_LIT`, `EOF` — Scanner token terminals

---

## 2. Production Rules

```ebnf
(* ============================================================ *)
(* Program & Top-Level Declarations                             *)
(* ============================================================ *)

program          ::= decl* EOF

decl             ::= IDENT ':' ( func_decl_tail | var_decl_tail )

func_decl_tail   ::= 'function' return_type '(' param_list? ')' ( '=' block_stmt | ';' )

var_decl_tail    ::= type_spec ( '=' initializer )? ';'

initializer      ::= expr
                   | '{' initializer ( ',' initializer )* '}'

(* ============================================================ *)
(* Types & Formal Parameters                                    *)
(* ============================================================ *)

return_type      ::= type_spec
                   | 'void'

type_spec        ::= scalar_type
                   | array_type

scalar_type      ::= 'integer'
                   | 'boolean'
                   | 'char'
                   | 'string'

array_type       ::= 'array' '[' expr? ']' type_spec

param_list       ::= param ( ',' param )*

param            ::= IDENT ':' type_spec

(* ============================================================ *)
(* Blocks & Statements                                          *)
(* ============================================================ *)

block_stmt       ::= '{' stmt* '}'

stmt             ::= if_stmt
                   | for_stmt
                   | return_stmt
                   | print_stmt
                   | block_stmt
                   | local_var_decl
                   | expr_stmt

local_var_decl   ::= IDENT ':' var_decl_tail

if_stmt          ::= 'if' '(' expr ')' stmt ( 'else' stmt )?

for_stmt         ::= 'for' '(' expr? ';' expr? ';' expr? ')' stmt

return_stmt      ::= 'return' expr? ';'

print_stmt       ::= 'print' expr_list? ';'

expr_stmt        ::= expr ';'

(* ============================================================ *)
(* Expressions (Ascending Precedence)                           *)
(* ============================================================ *)

expr_list        ::= expr ( ',' expr )*

expr             ::= assignment

assignment       ::= logic_or ( '=' assignment )?

logic_or         ::= logic_and ( '||' logic_and )*

logic_and        ::= relation ( '&&' relation )*

relation         ::= sum ( ( '<' | '<=' | '>' | '>=' | '==' | '!=' ) sum )*

sum              ::= product ( ( '+' | '-' ) product )*

product          ::= power ( ( '*' | '/' | '%' ) power )*

power            ::= unary ( '^' power )?

unary            ::= ( '-' | '!' ) unary
                   | postfix

postfix          ::= atom postfix_suffix*

postfix_suffix   ::= '[' expr ']'
                   | '(' expr_list? ')'
                   | '++'
                   | '--'

atom             ::= IDENT
                   | INT_LIT
                   | BOOL_LIT
                   | CHAR_LIT
                   | STR_LIT
                   | '(' expr ')'
```

---

## 3. Syntactic Design Notes

1. **Top-Level vs. Local Declarations:** Functions may only be declared at top level (`decl`). Inside a `block_stmt`, `local_var_decl` permits local scalar or array variables only.
2. **Prototypes & Definitions:** A function header ending in `;` defines a prototype, whereas `= { ... }` defines its implementation.
3. **Arrays:** `array_type` allows an optional size expression so both sized variable arrays (`buf: array [16] char;`) and unsized function parameters (`buf: array [] char`) share one grammar rule.
4. **Control Flow:** B-Minor provides `if`, `if-else`, and C-style `for` loops (`for (init; cond; step)` where each clause is optional).
5. **Separated Lists:** `entity ( ',' entity )*` prevents trailing commas or empty list items syntactically.

---

## 4. Syntax vs. Semantics Boundary

Checks requiring symbol tables or type inference are intentionally left for Phase 2 (Semantic Analysis):
- Requiring an entry-point `main` function returning `integer`.
- Restricting global initializers to compile-time constants.
- Verifying lvalue eligibility for assignment targets (`=`) and `++`/`--` operands.
- Enforcing sized array bounds in variable declarations vs. unsized parameters.
- Scope resolution, duplicate declarations, and static type checking.
