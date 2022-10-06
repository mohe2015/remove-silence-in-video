all: compile

# sudo pacman -S libc++
# https://clang.llvm.org/docs/StandardCPlusPlusModules.html
compile:
	#clang++ -g -ggdb -Og -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. -x c++-module lib.cpp --precompile -o lib.pcm
	clang++ -fno-omit-frame-pointer -fsanitize=address -Wpedantic -g -ggdb -O1 -fno-optimize-sibling-calls -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. main.cpp -c -o main.o
	#clang++ -g -ggdb -Og -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. lib.pcm -c -o lib.o
	clang++ -fno-omit-frame-pointer -fsanitize=address -g -ggdb -O1 -fno-optimize-sibling-calls -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. main.o -lavformat -lavcodec -lavutil -lavfilter -o main # lib.o

compile-release:
	clang++ -Wpedantic -g -ggdb -O3 -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. main.cpp -c -o main.o
	clang++ -g -ggdb -O3 -stdlib=libc++ -std=c++20 -Wall -Wextra -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=. main.o -lavformat -lavcodec -lavutil -lavfilter -o main # lib.o

time: compile-release
	time ./main > /dev/null 2>&1

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