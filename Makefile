PROJ_NAME=simp

# .c files

C_SOURCE=$(wildcard ./src/*.c)

# .h files

H_SOURCE=$(wildcard ./src/*.h)

# Object files

OBJ=$(subst .c,.o,$(subst source,objects,$(C_SOURCE)))

CC=clang

LIBS=-lz       \
	 -lpng16   \
	 -ljpeg    \

RM = rm -rf

all: objFolder $(PROJ_NAME)

$(PROJ_NAME): $(OBJ)
	$(CC) $(LIBS) -o $@ $^

./build/%.o: ./src/%.c ./src/%.h
	$(CC) $(LIBS) -o $@ $<

./build/main.o: ./src/main.c $(H_SOURCE)
	$(CC) $(LIBS) -o $@ $<

objFolder:
	mkdir -p build

clean:
	@ $(RM) ./src/*.o $(PROJ_NAME) *~

.PHONY: all clean
