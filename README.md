# MiniLang Compiler

MiniLang is a small, educational compiler implemented in C using Flex and Bison. It parses a simple, integer-only imperative language, lowers it to a 3-address code (3AC) Intermediate Representation (IR), runs a classic middle-end optimization pipeline, and then emits equivalent programs in two backends:

- `output.py` (Python interpreter over IR)
- `output.c` (C code generated from IR)

The compiler also prints both the unoptimized IR (`=== IR ===`) and optimized IR (`=== Optimized IR ===`). Code generation is performed from the optimized IR.

## Pipeline

`MiniLang (.ml)` → `lexer (Flex)` → `parser (Bison)` → `AST` → `3AC IR` → `IR optimizer` → `Python backend + C backend`

## Language Summary

MiniLang programs consist of statements with block-structured control flow.

### Supported Constructs

- Variable declaration: `let x = 10`
- Reassignment: `x = x + 1` (assignment to an undeclared variable is a compile-time error)
- Integer arithmetic: `+ - * /`
- Comparisons: `> < >= <= == !=` (results are `0` or `1`)
- If/else blocks:
  - `if cond { ... } else { ... }`
- Counted loops:
  - `loop i from 1 to 5 { ... }` (inclusive)
- Output: `print expr`
- Comments: lines beginning with `#`

### Scoping Rules (Enforced at Compile-Time)

- Blocks introduce a new scope.
- Using a variable outside of its scope (or before declaration) is a compile-time error.
- Redeclaration within the same scope is ignored (no duplicate symbol entry).

### Example Program

```text
# Example
let x = 10
let y = 20
let z = x + y * 2
let result = 0

if z > 30 {
    result = 1
} else {
    result = 0
}

print result
```

## IR and Optimizations

The IR is a simple 3-address instruction list (e.g., `ASSIGN`, `ADD`, `LTE`, `JUMP`, `JUMPF`, `LABEL`, `PRINT`).

The optimizer in `minilang/ir.c` performs conservative, basic-block-local optimizations:

- Constant propagation and constant folding
- Copy propagation (with correct invalidation on reassignment)
- Algebraic simplifications (e.g., `x + 0`, `x * 1`, `0 * x`)
- Dead temporary elimination (removes unused `tN` temps)
- Simple jump cleanup (removes `JUMP` to the immediately-next `LABEL`, removes unreachable instructions after unconditional jumps until next label)

Note: The project is intended for coursework and demonstration. The Python backend uses `//` for integer division; for negative operands this may differ from C’s truncation behavior.

## Build (Recommended: WSL on Windows)

This project uses common Unix build tools (`make`, `flex`, `bison`, `gcc`). On Windows, building via WSL is the simplest path.

### Prerequisites (Ubuntu / WSL)

```bash
sudo apt update
sudo apt install build-essential flex bison python3 mingw-w64
```

### Build Steps

From the repository root:

```bash
cd minilang
make clean
make
```

Artifacts:

- Linux/WSL compiler: `minilang/linux/minilang.out`
- Windows compiler (cross-compiled): `minilang/windows/minilang.exe`

## Run

The compiler takes a `.ml` program path and generates `output.py` and `output.c` in the current working directory.

### Linux / WSL

```bash
cd minilang
./linux/minilang.out tests/test1.ml

python3 output.py

gcc output.c -o result
./result
```

### Windows (PowerShell)

Build the project in WSL first (so `windows/minilang.exe` exists), then run:

```powershell
cd minilang
.\windows\minilang.exe tests\test1.ml

python output.py

cl output.c   # optional if you have MSVC installed
```

## Tests

Test programs live in `minilang/tests/`.

- `test1.ml`–`test5.ml` cover arithmetic, branching, and loops.
- `test6.ml` is a regression test for copy-propagation correctness.

## Repository Layout

```text
minilang/
├── Makefile
├── lexer.l
├── parser.y
├── ir.h
├── ir.c
├── backend_python.c
├── backend_c.c
└── tests/
    ├── test1.ml
    ├── test2.ml
    ├── test3.ml
    ├── test4.ml
    ├── test5.ml
    └── test6.ml
```  
