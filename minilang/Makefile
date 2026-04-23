CC     = gcc
CC_WIN = x86_64-w64-mingw32-gcc
CFLAGS = -Wall -g

all: linux windows

linux: parser.tab.c lex.yy.c ir.c backend_python.c backend_c.c
	mkdir -p linux
	$(CC) $(CFLAGS) -o linux/minilang.out \
	    parser.tab.c lex.yy.c ir.c backend_python.c backend_c.c

windows: parser.tab.c lex.yy.c ir.c backend_python.c backend_c.c
	mkdir -p windows
	$(CC_WIN) $(CFLAGS) -o windows/minilang.exe \
	    parser.tab.c lex.yy.c ir.c backend_python.c backend_c.c

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

clean:
	rm -f parser.tab.c parser.tab.h lex.yy.c output.py output.c
	rm -rf linux windows
