all: run

compile:
	g++ -lavformat -lavcodec -lavutil -lavfilter main.cpp -o main

run: compile
	./main