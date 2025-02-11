CFLAGS := -Wall -Werror -g -Og -Isrc
LDFLAGS := -lm

wordsearch: build_dir main.c.o dungeon.c.o perlin.c.o vec_geom.c.o
	gcc build/main.c.o build/dungeon.c.o build/perlin.c.o build/vec_geom.c.o $(LDFLAGS) -o game

build_dir:
	mkdir -p build

main.c.o: src/main.c
	gcc $(CFLAGS) src/main.c -c -o build/main.c.o

dungeon.c.o: src/dungeon.c
	gcc $(CFLAGS) src/dungeon.c -c -o build/dungeon.c.o

perlin.c.o: src/util/perlin.c
	gcc $(CFLAGS) src/util/perlin.c -c -o build/perlin.c.o

vec_geom.c.o: src/util/vec_geom.c
	gcc $(CFLAGS) src/util/vec_geom.c -c -o build/vec_geom.c.o


clean:
	rm -rf build game
