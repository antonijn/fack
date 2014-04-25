build:
	gcc -I./include src/main.c src/codegen.c src/parsing.c src/fparsing.c src/list.c src/sparsing.c src/eparsing.c src/eparsingapi.c -o fack -lm -O2 -w
debug:
	gcc -I./include src/main.c src/codegen.c src/parsing.c src/fparsing.c src/list.c src/sparsing.c src/eparsing.c src/eparsingapi.c -o fack -lm -g -rdynamic
runtests:
	make debug
	make -C tests
runtestsdbg:
	make debug
	make debug -C tests