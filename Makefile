all: run

# https://clang.llvm.org/docs/StandardCPlusPlusModules.html
compile:
	clang++ -std=c++20 -Wall -Wextra -x c++-module lib.cpp --precompile -o lib.pcm
	clang++ -std=c++20 -fprebuilt-module-path=. -Wall -Wextra -lavformat -lavcodec -lavutil -lavfilter lib.pcm main.cpp -o main

clean:
	rm -f main *.pcm

run: compile
	./main