#
# Now, only the one file will be recompiled
#
CC=gcc           #jjjjjjj
CFLAG=-g 
CFLG=-c              #hhhhhhhhh
OFLG=-o
a.out: myprog1.o myprog2.o myprog3.o myprog5
	$(CC) $(CFLAG) myprog1.o myprog2.o myprog3.o  #xxxxxxxxx
	cat < junk > anotherjunk ; cd   temp ; make   clean ; make ; make demo

myprog1.o: myprog1.c myprog.h  #1233
	$(CC) $(CFLG) myprog1.c
	echo"Inside myprog1.o"
	cat < b               #  jjsjsjsjsjsjsjsjj
myprog2.o: myprog2.c myprog.h myprog3.o
	$(CC) $(CFLG) myprog2.c 
	ps -ef | grep my | grep pro | grep Jun | grep Grads
myprog3.o: myprog3.c myprog.h myprog2.o
	$(CC) $(CFLG) myprog3.c 
.c.o: myprog.h
	$(CC) $(CFLG) $< 
	echo"Inside .c.o Double Suffix"
.c: myprog5.c
	$(CC) $(OFLG) $@ $<
	echo"Inside .c single suffix"
clean:
	rm -f a.out
	rm -f myprog1.o myprog2.o myprog3.o
	rm -f myprog1.c~ myprog2.c~ myprog3.c~ myprog.h~
demo: a.out
	./a.out
test: 
	sleep 1000
	sleep 1000	
test0:
	who
	ls
test1: 
	echo "run ./a.out"
	./a.out
	echo "pass ./a.out"
test2: 
	echo "run /bin/asdfjh"
	/bin/asdfjh
	echo "pass /bin/asdfjh"
test3: 
	echo "run ./myprog1.c"
	./myprog1.c
	echo "pass ./myprog1.c"
test4: test5
	ls&
test5: test6
	who&
test6: test4
	whoami&
test7: test8
	ls
