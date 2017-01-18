The Package consists of makefiles and test data and mymake .cpp


1)The mymake has three test make files for use mymake1.mk
mymake2.mk mymake3.mk

2)The mymake.cpp command allow flags f p k d i t for requirement
as mention in the project report

3)use the make command to generate a exe mymake and execute as 
follows
       ./mymake  or ./mymake -f mymake2.mk  or
./mymake -f mymake3.mk -p or ./mymake -f mymake3.mk -k

or ./mymake -f mymake3.mk -I   or ./mymake -f mymake3.mk -t 3

4)The mymake utilty behaves similar to the unix make utility

5) The mymake utility check for circular dependency and also 
generates the targets only if there is a update in the 
relavent files

6)It allows Pipe ,I/o redirection , echo , background process
Interupt and alarm 
7) gracefully terminates by sending SIGKILL to its process group