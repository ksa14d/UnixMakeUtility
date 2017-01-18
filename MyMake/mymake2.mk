# an example with inference rules
CC=gcc
CFLAG=-g 
 a.out:  myprog5
.c:  myprog5.c
	$(CC) -o  $@  $< 

clean :
	rm -f myprog5
demo: a.out
	myprog5
	

