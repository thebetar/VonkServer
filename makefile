# Makefile

SRC = ./src/*.c
OUT = ./main

# Default target
all:
		gcc $(SRC) -o $(OUT)

# Clean target
clean:
		rm -f $(OUT)
