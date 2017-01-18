# an example with inference rules
CC=gcc
CFLAG=-g 
a.out: myprog1.o myprog2.o myprog3.o 
	$(CC) $(CFLAG) myprog1.o myprog2.o myprog3.o
.c.o:
	$(CC) -c $<
myprog1.o : myprog1.c myprog.h
	$(CC) -c myprog1.c ; echo"wow"
	echo"I am target rule"
clean :
	rm -f a.out
demo: a.out
	a.out
	

