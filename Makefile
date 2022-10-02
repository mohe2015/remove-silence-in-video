all: run

compile:
	gcc main.cpp -o main

run: compile
	./main