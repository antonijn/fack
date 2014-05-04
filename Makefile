build:
	gcc -I./include src/*.c src/cpus/*.c -o fack -lm -O2 -w
debug:
	gcc -I./include src/*.c src/cpus/*.c -o fack -lm -g -rdynamic
runtests:
	make debug
	make -C tests
runtestsdbg:
	make debug
	make debug -C tests