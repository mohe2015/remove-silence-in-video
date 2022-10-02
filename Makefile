all: run

# https://clang.llvm.org/docs/StandardCPlusPlusModules.html
compile:
	clang++ -std=c++20 -Wall -Wextra -x c++-module lib.cppm --precompile -o lib.pcm
	clang++ -std=c++20 -fprebuilt-module-path=. -Wall -Wextra main.cpp -c -o main.o
	clang++ -std=c++20 lib.pcm -c -o lib.o
	clang++ main.o lib.o -lavformat -lavcodec -lavutil -lavfilter -o main
	

clean:
	rm -f main *.pcm *.o

run: compile
	./main