# queue_main: queue.c queue_main.c queue.h
# 	gcc queue.c queue_main.c -Wall -Werror -o queue_test

queue_main: queue.o queue_test.o
	gcc queue.o queue_test.o -o queue_test

queue.o: queue.c queue.h
	gcc -g queue.c -c

queue_main.o: queue_test.c queue.h
	gcc -g queue_test.c -c

clean:
	rm -rf *.o *~ queue_test
