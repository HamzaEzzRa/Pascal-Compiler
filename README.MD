# About
The front-end part of a PASCAL mini-compiler. Written from scratch in C without the use of lex/yacc and using [Free Pascal](https://www.freepascal.org/docs-html/current/ref/ref.html) as a reference.

# Progress

### Scanner

- [x] Scanning most data types referenced in Free Pascal.
- [ ] Add support for some other data types (set, variant, record, pointer, ...)

### Parser

- [x] Syntax check for program block.
- [x] Syntax check for external libraries.
- [x] Syntax check for variables block.
- [x] Syntax check for procedures, functions.
- [x] Syntax check for main block.

### Semantics

- [x] Implemented symbol tables for semantic checks.
- [x] Semantic checks for assignments.
- [x] Semantic checks for function / procedure calls.
- [x] Semantic checks for function overloading (C++ style name mangling)
- [ ] Handling variables with no initial values.
- [ ] Visualizing the symbol tables in XML format.

### Intermediate Code

- [x] Three address code implementation for some operations (binary, comparison, function/procedure call, ...)
- [ ] Code generation.

# References

- [Free Pascal](https://www.freepascal.org/docs-html/current/ref/ref.html).
- [https://github.com/mikestaub/Pascal-Compiler/](https://github.com/mikestaub/Pascal-Compiler/).
- [https://github.com/DrkSephy/pascal-compiler](https://github.com/DrkSephy/pascal-compiler).