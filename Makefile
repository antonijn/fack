build:
	gcc -I./include src/main.c src/codegen.c src/parsing.c src/fparsing.c -o tfacc