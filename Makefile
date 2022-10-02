all: run

compile:
	g++ -lavformat -lavcodec -lavutil main.cpp -o main

run: compile
	./main