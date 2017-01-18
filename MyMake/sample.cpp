#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
using namespace std;
class myfork
{

public:
myfork(){}
~myfork(){}
void junk(int argc, char* argv[])
{
for(int i =0 ; i<5 ; i++){
  int pid;
  int stat;

  if (argc < 2) {
    printf("Usage: a.out command\n");
    exit(0);
  }

  if (fork() == 0) {
    if (execv(argv[1], &argv[1]) == -1) exit(19);
    
    
  } else {
    pid = wait(&stat);
    if (stat >> 8 == 0xff) {
      printf("command failed\n");
    } else 
      printf("child pid = %d, status = %d\n", pid, stat>>8);
  }
}}
};

int main(int argc, char* argv[])
{

char** args = new char*[argc +1];
for(int i = 0; i<argc ; i++)args[i]=argv[i];
myfork* f = new myfork();
f->junk(argc,args);
}
