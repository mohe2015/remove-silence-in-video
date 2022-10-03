all: compile

# sudo pacman -S libc++
# https://clang.llvm.org/docs/StandardCPlusPlusModules.html
compile:
	#clang++ -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. -x c++-module lib.cpp --precompile -o lib.pcm
	clang++ -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. main.cpp -c -o main.o
	#clang++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. lib.pcm -c -o lib.o
	clang++ -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. main.o -lavformat -lavcodec -lavutil -lavfilter -o main # lib.o

format:
	clang-format -i lib.cpp main.cpp

compile-commands: clean
	bear -- make

clean:
	rm -f main *.pcm *.o

run: compile
	./main