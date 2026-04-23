CC     = gcc
CFLAGS = -Wall -g

all: minilang

minilang: parser.tab.c lex.yy.c ir.c backend_python.c backend_c.c
	$(CC) $(CFLAGS) -o minilang \
	    parser.tab.c lex.yy.c ir.c backend_python.c backend_c.c \
	    -lfl

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

clean:
	rm -f minilang parser.tab.c parser.tab.h lex.yy.c output.py output.c
