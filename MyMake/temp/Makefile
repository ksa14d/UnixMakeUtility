CC = g++
CFLAGS= -O0 -Wall -std=c++98 -pedantic

all:  mymake
clean:
	rm -f *.o
	rm -f name
	rm -f *~
	rm -f core

mymake.o: mymake.cpp
	$(CC) $(CFLAGS) -g -c name.cpp

mymake:  mymake.o 
	$(CC) $(CFLAGS) -g  -o name name.o 
  
demo:	 name
	./name
	
