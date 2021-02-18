OBJS = .\src\main.c .\src\scanner.c .\src\parser.c .\src\symbol_table.c

CC = gcc

LIBRARY_PATHS = -LC:\MinGW\lib

COMPILER_FLAGS = -Wall -Wextra

OBJ_NAME = pcomp

all: $(OBJS)
	$(CC) $(OBJS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) -o $(OBJ_NAME)

clean:
	rm -r $(OBJ_NAME)
