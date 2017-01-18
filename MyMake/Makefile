CC = g++
CFLAGS= -O0 -Wall -std=c++98 -pedantic

all:  mymake
clean:
	rm -f *.o
	rm -f mymake
	rm -f *~
	rm -f core

mymake.o: mymake.cpp
	$(CC) $(CFLAGS) -g -c mymake.cpp

mymake:  mymake.o 
	$(CC) $(CFLAGS) -g  -o mymake mymake.o 
  
demo:	 mymake
	./mymake 
	
