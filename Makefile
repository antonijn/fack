build:
	gcc -I./include src/main.c src/codegen.c src/parsing.c src/fparsing.c src/list.c src/sparsing.c src/eparsing.c -o fack -O2
debug:
	gcc -I./include src/main.c src/codegen.c src/parsing.c src/fparsing.c src/list.c src/sparsing.c src/eparsing.c -o fack -g -rdynamic
runtests:
	make debug
	make -C tests