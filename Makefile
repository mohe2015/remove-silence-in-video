all: run

compile:
	g++ -lavformat -lavcodec main.cpp -o main

run: compile
	./main