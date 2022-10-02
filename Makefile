all: run

# https://clang.llvm.org/docs/StandardCPlusPlusModules.html
compile:
	clang++ -stdlib=libc++ -std=c++20 -Wall -Wextra -fprebuilt-module-path=. -x c++-module lib.cppm --precompile -o lib.pcm
	clang++ -stdlib=libc++ -std=c++20 -Wall -Wextra -fprebuilt-module-path=. main.cpp -c -o main.o
	clang++ -stdlib=libc++ -std=c++20 -Wall -Wextra -fprebuilt-module-path=. lib.pcm -c -o lib.o
	clang++ -stdlib=libc++ -std=c++20 -Wall -Wextra -fprebuilt-module-path=. main.o lib.o -lavformat -lavcodec -lavutil -lavfilter -o main
	

clean:
	rm -f main *.pcm *.o

run: compile
	./main