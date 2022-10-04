all: compile

# sudo pacman -S libc++
# https://clang.llvm.org/docs/StandardCPlusPlusModules.html
compile:
	#clang++ -g -ggdb -Og -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. -x c++-module lib.cpp --precompile -o lib.pcm
	clang++ -Weverything -Wno-c++98-compat -Wpedantic -g -ggdb -Og -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. main.cpp -c -o main.o
	#clang++ -g -ggdb -Og -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. lib.pcm -c -o lib.o
	clang++ -g -ggdb -Og -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. main.o -lavformat -lavcodec -lavutil -lavfilter -o main # lib.o

compile-release:
	clang++ -Weverything -Wno-c++98-compat -Wpedantic -g -ggdb -O3 -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. main.cpp -c -o main.o
	clang++ -g -ggdb -O3 -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. main.o -lavformat -lavcodec -lavutil -lavfilter -o main # lib.o

run-release: compile-release
	./main

format:
	clang-format -i main.cpp # lib.cpp

compile-commands: clean
	bear -- make

clean:
	rm -f main *.pcm *.o

run: compile
	./main