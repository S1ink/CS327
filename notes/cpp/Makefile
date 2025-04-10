hello: hello.cpp
	g++ hello.cpp -o hello

swap: swap.cpp
	g++ swap.cpp -o swap

string2 : string.cpp
	g++ string.cpp -o string2 -DSTRING2_MAIN=1

bst : bst.cpp
	g++ bst.cpp -o bst


c_func : c_func.c.o c_func.cpp.o
	g++ c_func.c.o c_func.cpp.o -o c_func

c_func.c.o : c_func.c
	gcc c_func.c -c -o c_func.c.o

c_func.cpp.o : c_func_main.cpp
	g++ c_func_main.cpp -c -o c_func.cpp.o


cpp_func : cpp_func.cpp.o cpp_func.c.o
	g++ cpp_func.cpp.o cpp_func.c.o -o cpp_func

cpp_func.c.o : cpp_func_main.c
	gcc cpp_func_main.c -c -o cpp_func.c.o

cpp_func.cpp.o : cpp_func.cpp
	g++ cpp_func.cpp -c -o cpp_func.cpp.o


inheritance : inheritance.cpp
	g++ inheritance.cpp -o inheritance
