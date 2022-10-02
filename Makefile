all: run

# https://clang.llvm.org/docs/StandardCPlusPlusModules.html
compile:
	clang++ -std=c++20 -fmodules -fbuiltin-module-map -Wall -Wextra lib.cppm --precompile -o lib.pcm
	clang++ -std=c++20 -fmodules -fbuiltin-module-map -fprebuilt-module-path=. -Wall -Wextra -lavformat -lavcodec -lavutil -lavfilter lib.pcm main.cpp -o main

run: compile
	./main