# queue_main: queue.c queue_main.c queue.h
# 	gcc queue.c queue_main.c -Wall -Werror -o queue_test

queue_main: queue.o queue_test.o
	gcc queue.o queue_test.o -o queue_test

queue.o: queue.c queue.h
	gcc -g queue.c -c

queue_main.o: queue_test.c queue.h
	gcc -g queue_test.c -c

fio: fio.c
	gcc fio.c -o fio

strings: strings.c
	gcc strings.c -o strings

chars: char.c
	gcc char.c -Wall -Werror -o char

varray: varray.c
	gcc varray.c -Wall -Werror -DVARRAY_MAIN -o varray

macros: macros.c
	gcc macros.c -Wall -Werror -o macros

vaargs: vaargs.c
	gcc vaargs.c -Wall -Werror -o vaargs

return_local: return_local.c
	gcc return_local.c -Wall -o return_local

curses: curses.c
	gcc curses.c -Wall -lncurses -o curses

mid_ex: mid_ex.c
	gcc mid_ex.c -Wall -o mid_ex

clean:
	rm -rf *.o *~ queue_test strings char varray macros
