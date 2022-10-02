all: run

compile:
	g++ -std=c++20 -lavformat -lavcodec -lavutil -lavfilter main.cpp -o main

run: compile
	./main