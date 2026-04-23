# MiniLang: Custom IR-Based Code Compiler

MiniLang is a custom compiler built from scratch for a university compilers course. It takes a specialized functional/imperative programming language and compiles it down to a custom 3-address Intermediate Representation (IR). From this single unified IR, the compiler can generate fully executable code across multiple backend targets: **Python** and **C**.

### Research Inspiration
This project demonstrates the core conceptual architecture discussed in recent academic research, explicitly acting as a small-scale model for the idea that **Intermediate Representations act as a universal bridge for robust cross-language code translation**:
1. *"IRCoder: Intermediate Representations Make Language Models Robust Multilingual Code Generators"* — Paul et al. (ACL 2024)
2. *"Code Translation with Compiler Representations"* — Szafraniec et al. (ICLR 2023)
3. *"Can Large Language Models Understand Intermediate Representations in Compilers?"* (arXiv 2025)

---

## Architecture Pipeline

The compiler's translation pipeline is as follows:
`MiniLang Source` -> `Flex (Lexer)` -> `Bison (Parser)` -> `AST (Abstract Syntax Tree)` -> `3-Address IR` -> `Python Backend` & `C Backend`

Both `output.py` and `output.c` produce identical computational results when executed. This empirically verifies that the 3-address IR serves as a correct, complete representation of the original source's semantics regardless of the output language.

---

## Features Supported

The MiniLang compiler currently supports:
*   **Variable declaration:** `let x = 10`
*   **Arithmetic operations:** `let z = x + y * 2`
*   **Comparisons:** `>`, `<`, `>=`, `<=`, `==`, `!=`
*   **Control Flow (If/Else):** `if z > 30 { ... } else { ... }`
*   **Loops:** `loop i from 1 to 5 { ... }`
*   **Console Output:** `print x`
*   **Comments:** `# this is a comment`

### Example MiniLang Program
```swift
# test1.ml
let x = 10
let y = 20
let z = x + y * 2
if z > 30 {
    let result = 1
} else {
    let result = 0
}

print result
```

---

## Setup & Installation

This project relies on standard Unix compilation tools (`make`, `flex`, `bison`, `gcc`). On Windows, **Windows Subsystem for Linux (WSL)** is the recommended development environment.

### Prerequisites (Ubuntu / WSL)
Install the required packages along with the MinGW cross-compiler to generate Windows `.exe` files:
```bash
sudo apt update
sudo apt install build-essential flex bison python3 mingw-w64
```

---

## Generating the Compiler

1. Open your terminal (or WSL on Windows).
2. Navigate to the `minilang/` directory.
3. Run the following make command to build the project components (Lexical analyzer, Parser, and IR builder) completely automatically:
```bash
make
```

The Makefile is explicitly configured to compile binaries for multiple operating systems, yielding two output executables in separate folders:
*   **Linux version:** Found at `linux/minilang.out`
*   **Windows version:** Found at `windows/minilang.exe` (cross-compiled via MinGW)

To clear out the automatically generated source files and previously compiled compilers, run `make clean`.

---

## Running and Using the Compiler

To use the compiler, pass any valid `.ml` source file as an argument. The compiler will ingest the source code, display the 3-address Intermediate Representation (IR), and automatically generate the two target backend outputs (`output.py` and `output.c`) right in your current directory.

### On Windows (PowerShell)
```powershell
# 1. Provide a MiniLang source file to your compiled executable
.\windows\minilang.exe tests\test1.ml

# 2. To verify the generation, execute the produced Python output
python output.py
```

### On Linux / WSL (Bash)
```bash
# 1. Provide a MiniLang source file to your compiled executable
./linux/minilang.out tests/test1.ml

# 2. Execute the produced Python backend output
python3 output.py

# 3. Compile and execute the produced C backend output
gcc output.c -o result
./result
```

Both backend outputs will yield the exact same printed results (validating the IR logic) without throwing any syntax or scoping errors.

---

## Project Structure

```text
minilang/
├── Makefile             # Multi-target build configuration
├── lexer.l              # Flex lexical analyzer definitions
├── parser.y             # Bison grammar parsing & AST construction
├── ir.h                 # AST node types and IR instruction definitions
├── ir.c                 # IR generator (The core logic mapping AST to IR)
├── backend_python.c     # Emits 'output.py' from semantic IR
├── backend_c.c          # Emits 'output.c' from semantic IR
└── tests/               # MiniLang test suites
    ├── test1.ml         # Focus: Basic arithmetic and if/else
    ├── test2.ml         # Focus: Simple loops and accumulation
    ├── test3.ml         # Focus: Nested branching
    ├── test4.ml         # Focus: Operator precedence
    └── test5.ml         # Focus: Loops combined with internal branching
```
