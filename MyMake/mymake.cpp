#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <math.h>
#include <getopt.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h> 
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <errno.h>
using namespace std;

char fflag[1] = {'f'};
char pflag[1] = {'p'};
char kflag[1] = {'k'};
char dflag[1] = {'d'};
char iflag[1] = {'i'};
char tflag[1] = {'t'};

struct option long_options[] = 
{
	{fflag, required_argument,NULL,'f'},
	{pflag, optional_argument,NULL,'p'},
	{kflag, optional_argument,NULL,'k'},
	{dflag, optional_argument,NULL,'d'},
	{iflag, optional_argument,NULL,'i'},
	{tflag, required_argument,NULL,'t'},
};

char** Paths ;
int n_path;
char* rulesdb = NULL;	
const char* Makefile = NULL;
char* makearg = NULL;
bool IsPFlagSet = false;
bool IsKFlagSet = false;
bool IsDFlagSet = false;


class RuleStat
{
public:
	int n_macros;
	int n_commandline;
	int n_targets;
	int n_commands;
	int n_inferences;
	RuleStat()
	{
		n_macros = 0;
		n_commandline = 0;
		n_commands = 0;
		n_targets = 0;
		n_inferences = 0;
	}
	~RuleStat()
	{
	}
};


RuleStat* rule_stats;
int find(const char* text , const char* pattern)
{
	int n = strlen(text);
	int m = strlen(pattern);
	for(int i = 0; i <= n - m ; i++)
	{
		int  j = 0;
		while(j < m && pattern[j] == text[i+j])j++;
		if (j == m) return i ;
	}
	return -1;
}

char* substring(const char* text , int start , int end )
{
	char subs[200];
	int i,j=0;
	for(i = start; i < end ; i++)
	{
		subs[j++] = text[i]; 
	}
	subs[j] = '\0';
	int n = strlen(subs);
	i =0;
	while(subs[i] == ' ')i++;
	j = n-1;
	while(subs[j] == ' ')j--;

	char trimmed[100];
	int k = 0;
	while(i <= j)
	{
		trimmed[k++] = subs[i++];
	}
	trimmed[k] = '\0';
	char *s = new char[strlen(trimmed)+1];
	strcpy(s,trimmed);
	return s;
}


char* trim(char *str)
{
	int n = strlen(str);
	int  i =0;
	while(str[i] == ' ')i++;
	int j = n-1;
	while(str[j] == ' ')j--;
	char trimmed[100];
	int k = 0;
	while(i <= j)
	{
		trimmed[k++] = str[i++];
	}
	trimmed[k] = '\0';
	char *s = new char[strlen(trimmed)+1];
	strcpy(s,trimmed);
	return s;
}

class CommandLine
{
public:
	char** commands;
	char* MatchedRule;
	char* s2;
	char* s1;
	int n_cmd;
	CommandLine(char* line)
	{
		commands = new char*[30];
		n_cmd = 0;
		char *p = line;
		char *ptr = strtok(p,";");
		while (ptr!=NULL)
		{
			insert_command(trim(ptr));
			ptr = strtok(NULL,";");
		}
		delete[] line;
	}

	char* apply_inference(char *start )
	{
		char cmd[100];
		int k =0, j= 0;
		while( start[j]!='\0')
		{
			if(start[j]=='$' && start[j+1] == '<' )
			{
				int l =0;
				while(MatchedRule[l]!= '\0')
				{
					cmd[k++] = MatchedRule[l++];
				}
				l= 0;
				while(s1[l] != '\0')
				{ 
					cmd[k++] = s1[l++];
				}
				j+=2;
			}
			if(start[j]=='$' && start[j+1] == '@' )
			{
				int l =0;
				while(MatchedRule[l]!= '\0')
				{
					cmd[k++] = MatchedRule[l++];
				}
				j+=2;
			}
			else
			{
				cmd[k++] = start[j++];
			}
		}
		cmd[k++] = '\0';
		return trim(cmd);
	}

	~CommandLine()
	{
		for(int i = 0 ; i < n_cmd ; i++)
		{
			delete[] commands[i];
		}
		delete[] commands;
	}

	void insert_command(char* cmd)
	{
		commands[n_cmd++] = cmd;
		rule_stats->n_commands++;
	}

	char* absolutePath(char *cmd)
	{ 
		struct stat stat_dir;
		struct dirent* w;
		char fpath[2000];
		DIR* d;
		bool done = false;
		for(int i = 0; i< n_path && !done ; i++)
		{
			if((d = opendir(Paths[i])) == NULL)
			{
				//	    int num = errno;
				if(stat(Paths[i],&stat_dir) == -1)
				{
					//   cout<<"path : "<<Paths[i]<<"directory does not exist !"<<endl;
				}
				else
				{
					// cout<<"Error : "<<strerror(num)<<endl;
				}
				continue;
			}
			while((w=readdir(d))!= NULL)
			{
				if(strcmp(w->d_name, cmd) == 0)
				{
					sprintf(fpath,"%s/%s",Paths[i],w->d_name);
					done = true;
					break;
				}
				if((strcmp(w->d_name,".")==0) || (strcmp(w->d_name,"..")==0))
					continue;
			}
			closedir(d);
		}
		char* s = new char[strlen(fpath)+1];
		strcpy(s,fpath);
		return s;
	}

	void echoMessage(const char* cmdptr, bool IsBackground)
	{
		char message[200];
		sscanf(cmdptr,"\techo \"%[^\"]\"",message);
		int pid;
		FILE* fd;
		if((pid = fork())==0)
		{
			if((fd = fopen("/dev/tty","r+"))==0)
			{
				cout<<"Cannot open /dev/tty"<<endl;
				exit(1);
			}
			close(1);
			close(2);
			close(0);
			fprintf(fd,message);
			fclose(fd);
			exit(0);
		}
		else if(pid > 0)
		{

			if(!IsBackground)
			{
				int stat;
				int p = wait(&stat);
				int ChildExitStatus = WEXITSTATUS(stat);
				if(IsDFlagSet)
					cout<<"Child PID :"<<p<<"Exit Status : "<<ChildExitStatus<<endl;
				if(!IsKFlagSet && ChildExitStatus!=0)
				{
					exit(127);
				}
			}
		}
		else if(pid == -1)
		{
			cout<<"Erro fork "<<endl;
		}

	}


	void RunNormalCommand(const char* cmdptr,bool IsBackground)
	{
		char** argv = new char*[40];
		int argc = 0;
		char cmd[100];
		strcpy(cmd,cmdptr);
		char *p = cmd;
		char *ptr = strtok(p," \t ");
		if(ptr != NULL && find(ptr,"/")== -1)
			argv[argc++] = absolutePath(ptr);
		else if(ptr != NULL) 
			argv[argc++] = trim(ptr);
		ptr = strtok(NULL," \t ");
		while (ptr!=NULL)
		{
			argv[argc++] = trim(ptr);
			ptr = strtok(NULL," \t ");
		}
		argv[argc++] = (char*)0;
		int pid;
		if ((pid=fork()) == 0) 
		{
			if (execv(argv[0], &argv[0]) == -1)
			{ 
				exit(19);
			}               
		}
		else if(pid > 0)
		{

			if(!IsBackground)
			{
				int stat;
				int p = wait(&stat);
				int ChildExitStatus = WEXITSTATUS(stat);
				if(IsDFlagSet)
					cout<<"Child PID :"<<p<<"Exit Status : "<<ChildExitStatus<<endl;
				if(!IsKFlagSet && ChildExitStatus!=0)
				{
					exit(127);
				}
			}  
		}
		else if(pid == -1)
		{
			cout<<"fork error"<<endl;
		}
		for(int i = 0 ; i < argc ; i++) 
		{
			delete [] argv[i];
		} 
		delete[] argv;
	}

	void RunPipedCommand(char* cmdptr,bool IsBackground)
	{
		char pipedcmd[100];
		char *cmds[5];
		int n_pcmd = 0;
		strcpy(pipedcmd,cmdptr);
		char *p = pipedcmd;
		char *ptr = strtok(p,"|");
		while (ptr!=NULL)
		{
			cmds[n_pcmd++] = trim(ptr);
			ptr = strtok(NULL,"|");
		}
		char** argv = new char*[15];
		int argc = 0;
		int** fds = new int*[n_pcmd-1];
		for(int i = 0 ; i< n_pcmd -1; i++)
		{
			fds[i] = new int[2];
			pipe(fds[i]);
		}
		int* pid = new int[n_pcmd];
		int* child = new int[n_pcmd];
		int* fd;
		int* pfd;
		for(int i = 0 ; i < n_pcmd ; i++) 
		{
			argc = 0;
			p = cmds[i];
			ptr = strtok(p," \t ");
			if(ptr != NULL && find(ptr,"/")== -1)
				argv[argc++] = absolutePath(ptr);
			else if(ptr != NULL) 
				argv[argc++] = trim(ptr);
			ptr = strtok(NULL," \t ");
			while (ptr!=NULL)
			{
				argv[argc++] = trim(ptr);
				ptr = strtok(NULL," \t ");
			}
			argv[argc++] = (char*)0;
			if(i < n_pcmd -1)fd = fds[i];
			if(i > 0)pfd = fds[i-1];
			if((pid[i]=fork()) == 0 )
			{
				if(i > 0)
				{
					close(0);dup(pfd[0]);
				}
				if(i < n_pcmd -1)	
				{
					close(1);dup(fd[1]);
				}
				for(int j = 0 ; j < n_pcmd; j++)
				{
					for(int k =0 ; k< 2; k++)
						close(fds[j][k]);
				}
				if (execv(argv[0], &argv[0]) == -1)
				{ 
					exit(19);
				}               
			}
			else if(pid[i] == -1)
			{
				cout<<"err could not fork"<<endl;
			}
			else if(pid[i] > 0)
			{
				if(i > 0)
				{
					close(pfd[0]); close(pfd[1]);
				}
			}
		}

		if(!IsBackground)
		{
			for(int i =0; i<n_pcmd ; i++) 
			{
				int pid =  wait(&child[i]);
				int ChildExitStatus = WEXITSTATUS(child[i]);
				if(IsDFlagSet)
					cout<<"Child PID :"<<pid<<"Exit Status : "<<ChildExitStatus<<endl;
				if(!IsKFlagSet && ChildExitStatus!=0)
				{
					exit(127);
				}
			}
		}
		for(int i = 0 ; i< n_pcmd -1; i++)
		{
			delete[] fds[i];
		}
		delete[] fds;
		delete[] pid;
		delete[] argv;
		delete[] child;	 
	}


	void RunRedirectedCommand(char* cmdptr,bool IsBackground)
	{
		char rcmd[100];
		char *cmds[4] = {'\0','\0','\0','\0'} ;// command file 1 file 2
		int n_rcmd = 0;
		strcpy(rcmd,cmdptr);
		char* p = rcmd;
		char delim[2] = {'>','<'};
		char* ptr = strtok(p,delim);
		while (ptr!=NULL)
		{
			cmds[n_rcmd++] = trim(ptr);
			ptr = strtok(NULL,delim);
		}
		int mode[2];
		bool IsFirst = true;
		strcpy(rcmd,cmdptr);
		int len = strlen(rcmd);
		for(int i = 0 ; i< len ; i++)
		{
			switch(rcmd[i])
			{
			case '<':if(IsFirst){mode[0]=0;IsFirst = false;}
					 else mode[1]=0;
					 break;

			case '>':if(IsFirst){mode[0]=1;IsFirst = false;}
					 else mode[1]=1;
					 break;
			}
		}
		strcpy(rcmd,cmds[0]); 

		char** argv = new char*[15];
		int argc = 0;
		p = rcmd;
		ptr = strtok(p," \t ");
		if(ptr != NULL && find(ptr,"/")== -1)
			argv[argc++] = absolutePath(ptr);
		else if(ptr != NULL) 
			argv[argc++] = trim(ptr);
		ptr = strtok(NULL," \t ");
		while (ptr!=NULL)
		{
			argv[argc++] = trim(ptr);
			ptr = strtok(NULL," \t ");
		}
		argv[argc++] = (char*)0;
		int pid;
		if((pid = fork())==0)
		{
			for(int i = 1 ;i < n_rcmd ; i++)
			{
				close(mode[i-1]);
				open(cmds[i],O_RDWR | O_CREAT, 0777);
			}
			if(execv(argv[0],&argv[0]) == -1)
			{
				exit(19);
			}
		}
		else if(pid > 0)
		{

			if(!IsBackground)
			{
				int stat;
				int p = wait(&stat);
				int ChildExitStatus = WEXITSTATUS(stat);
				if(IsDFlagSet)
					cout<<"Child PID :"<<p<<"Exit Status : "<<ChildExitStatus<<endl;
				if(!IsKFlagSet && ChildExitStatus!=0)
				{
					exit(127);
				}
			}
		}
		else if(pid == -1)
		{
			cout<<"error fork"<<endl;
		}
	}

	void RunCommandLine()
	{
		char cwd[500];
		char *oldpath = NULL;
		int suc;
		for(int i = 0; i< n_cmd ; i++)
		{
			char* cmd = commands[i];
			if(find(cmd,"$@")!=-1||find(cmd,"$<")!=-1)
			{
				cmd= apply_inference(cmd);	 
			}
			cout<<cmd<<endl;
			bool IsBackground = false;
			int pos =-1;
			if((pos = find(cmd,"&"))!=-1)
			{
				cmd[pos] = ' ';
				IsBackground = true;
			}
			if(find(cmd,"cd")!= -1)
			{ 
				char cdcmd[200];
				sscanf(cmd,"%*s %s",cdcmd); // check this one
				if(oldpath == NULL)
				{
					oldpath = getcwd(cwd,499);
				}   
				if((suc = chdir(cdcmd))!=0 && !IsKFlagSet)
				{
					cout<<"could not change directory"<<endl;
					exit(127);
				}
			}
			else if(find(cmd,"|") != -1)
			{
				RunPipedCommand(cmd,IsBackground);
			}
			else if(find(cmd,"<") != -1 || find(cmd,">") != -1)
			{
				RunRedirectedCommand(cmd,IsBackground);	 
			}
			else if(find(commands[i],"echo")!= -1)
			{
				echoMessage(cmd,IsBackground);   
			}
			else
			{
				RunNormalCommand(cmd,IsBackground);
			}

		}
		if(oldpath!=NULL)
			if((suc = chdir(oldpath))!=0 && !IsKFlagSet)
			{
				cout<<"could not restore directory path"<<endl;
				exit(127);
			}
	}
};

class Target
{
public:
	char* name;
	char** dependents;
	bool IsInference ;
	bool IsSingleSuffix ;
	char* MatchedRule;
	int n_deps;
	char* s1;
	char* s2;
	CommandLine** commandLines;
	int n_cmdln;
	Target(char* target,char* dependencyList)
	{
		name = target;
		IsInference = false;
		char a[10];
		char b[10];
		int x = 0;
		if((x =sscanf(name,".%[^.].%[^.]",a,b)) == 2)
		{
			IsInference = true; 
			IsSingleSuffix = false;
			s1 = new char[strlen(a)+1];
			sprintf(s1,".%s",a);
			s2 = new char[strlen(b)+1];
			sprintf(s2,".%s",b);
		}
		else if((x =sscanf(name,".%[^.]",a)) == 1)
		{
			IsInference = true;
			IsSingleSuffix = true;
			s1 = new char[strlen(a)+1];
			sprintf(s1,".%s",a);
		}

		n_deps = 0;
		dependents = new char*[20];
		n_cmdln = 0;
		commandLines = new CommandLine*[40];
		char *p = dependencyList;
		char *ptr = strtok(p," \t ");
		while (ptr!=NULL)
		{
			insert_dependents(trim(ptr));
			ptr = strtok(NULL," \t ");
		} 
		delete[] dependencyList;    
	}
	void insert_commandLine(CommandLine* cmdln)
	{
		commandLines[n_cmdln++] = cmdln;
		rule_stats->n_commandline++;
	}

	void insert_dependents(char* deps)
	{
		dependents[n_deps++] = deps;
	}
	~Target()
	{
		delete[] name;
		for(int i = 0 ; i<n_cmdln ; i++)
		{
			delete commandLines[i];
		}
		delete[] commandLines;
		for(int i = 0 ; i< n_deps ; i++)
		{
			delete[] dependents[i];
		}
		delete[] dependents;
	}

	void RunCommandLines()
	{       
		for(int i = 0 ; i< n_cmdln ;i++)
		{
			if(IsInference)
			{
				commandLines[i]->MatchedRule = MatchedRule;
				commandLines[i]->s1 = s1;
				commandLines[i]->s2 = s2;
			}
			commandLines[i]->RunCommandLine();
		}
	}
};

class Macro
{
public :
	char* LHS ;
	char* RHS ;
	Macro(char* l,char* r)
	{
		LHS = new char[strlen(l)+4]; 
		RHS = r;
		LHS = l; 
	}

	char* apply(char* text)
	{      
		char newstr[200];
		char* pat[2];
		pat[0] = new char[strlen(LHS)+4];
		pat[1] = new char[strlen(LHS)+2];
		sprintf(pat[0],"$(%s)",LHS);
		sprintf(pat[1],"$%s",LHS);
		for(int x = 0 ; x < 2 ; x++ )
		{
			int startpos ,i = 0, k = 0 ;
			while ((startpos = find(text,pat[x]))!= -1)
			{
				int l_len = strlen(pat[x]);
				int r_len = strlen(RHS);
				while(i < startpos)newstr[i++] = text[k++];
				int l = k+ l_len;
				while(k < l )text[k++]=' ';
				int j = 0;
				while(j < r_len) newstr[i++] = RHS[j++];  
				while((newstr[i++]=text[k++])!= '\0');
			}
			newstr[i] = '\0';
			if(i>0)
			{
				delete[] text;
				text = new char[strlen(newstr)+1];
				sprintf(text,"%s",newstr);
			}
		}
		return text;
	}
	~Macro()
	{

		delete[] LHS;
		delete[] RHS;
	}
};


int n_chain=0;
char** dependency_chain = new char*[50];

bool check_circular_deps(char* elem)
{
	for(int i =0 ; i< n_chain ; i++)
	{
		if(strcmp(dependency_chain[i],elem)==0)
		{
			return true;
		}
	}
	dependency_chain[n_chain++]=elem;
	return false;
}


void remove_from_chain(char* elem)
{
	dependency_chain[n_chain--]=NULL;
}
class Rules
{
public :
	Target* prev;
	int n_macros;
	int n_targets;
	Macro** macros;
	Target** targets;
	int n_chain;
	Rules()
	{
		prev = NULL;
		n_macros = 0;
		n_targets = 0;
		macros = new Macro*[20];
		targets = new Target*[30];
	}
	~Rules()
	{
		for(int i =0; i< n_chain; i++)
		{
			delete[] dependency_chain[i];
		}
		delete[] dependency_chain;
		for(int i = 0;i<n_macros;i++)
		{
			delete macros[i];
		}
		delete []macros;
		for(int i = 0; i< n_targets; i++)
		{
			delete targets[i];
		}
		delete[] targets; 
	}

	void insert_macro(Macro* m)
	{
		macros[n_macros++] = m ;
		rule_stats->n_macros++;
	}

	void insert_target(Target* t)
	{
		targets[n_targets++] = t;
		if(t->IsInference)
		{
			rule_stats->n_inferences++;
		}
		else
		{
			rule_stats->n_targets++;
		}
	}


	Target* GetTargetByName(char* targetName)
	{
		Target *t = NULL ;
		int i;
		for(i= 0; i<n_targets;  i++)
		{
			if(strcmp(targets[i]->name,targetName)==0 && !targets[i]->IsInference)
			{
				t = targets[i];
				break;
			}
		}
		return t;
	}


	Target* GetTargetByInference(char* targetName)
	{
		Target *t = NULL ;
		int i;
		for(i= 0; i<n_targets;  i++)
		{
			if(targets[i]->IsInference)
			{
				if(targets[i]->IsSingleSuffix)
				{
					if(find(targetName,".")==-1)
					{
						t = targets[i];
						break;
					}
				}
				else 
				{
					int m = strlen(targetName) ;
					int n = strlen(targets[i]->s2) ;
					if(find(targetName,targets[i]->s2)==m-n)
					{
						t = targets[i];
						break;
					}
				}
			}
		}
		return t;
	}


	void identify_rule(char* line)
	{
		int pos;
		if((pos = find(line,"#"))== 0)
		{
			cout << "comment : "<<line << endl;
		}
		else if((pos = find(line,"="))!= -1)
		{
			Macro* m = new Macro(substring(line,0,pos),substring(line,pos+1,strlen(line)));
			insert_macro(m);
		}
		else if((pos = find(line,":"))!= -1)
		{
			Target* t = new Target(substring(line,0,pos),substring(line,pos+1,strlen(line)));
			prev = t;
			insert_target(prev);
		} 
		else if((pos = find(line,"\t"))==0)
		{
			char* s = trim(line);
			for(int i = 0; i < n_macros ; i++)
			{	     
				s =  macros[i]->apply(s); 
			}
			CommandLine *cl = new CommandLine(s);
			prev->insert_commandLine(cl);
		}
	}

	bool ExecuteMake(char* makeTarget, struct stat *statbuf)
	{
		if(IsDFlagSet)
		{
			cout<<"making "<<makeTarget<<endl;
		}	 
		Target *t = GetTargetByName(makeTarget);
		if(t == NULL)
		{
			t = GetTargetByInference(makeTarget);
			if(t != NULL)
			{
				char substitute[50];
				int i;
				for(i = 0 ; makeTarget[i]!= '\0' && makeTarget[i]!= '.'; i++)
				{
					substitute[i] = makeTarget[i];
				}
				substitute[i] = '\0';
				t->MatchedRule = trim(substitute);
			}
		}
		bool IsTargetAccessible = false;
		if(stat(makeTarget,statbuf) != -1)
		{
			IsTargetAccessible = true;
		} 
		if(t != NULL)
		{   
			if(IsDFlagSet)
			{
				cout<<"\tTarget rule for "<<t->name<<endl;
				cout<<"\tPrerequisits:";
				for(int x =0; x<t->n_deps; x++)
				{
					cout<<t->dependents[x]<<" ";
				}
				cout<<endl;
			}
			bool IsFilesAccessible = true,IsSourceModified = false;
			for(int i =0; i< t->n_deps ; i++)    
			{
				struct stat stats;
				bool IsCircleDep = check_circular_deps(t->dependents[i]);
				if(!IsCircleDep)
				{
					if(!ExecuteMake(t->dependents[i],&stats))IsFilesAccessible = false;
					remove_from_chain(t->dependents[i]);
				}
				else
				{
					cout<<"Target : "<<t->name<<" <-- "<<t->dependents[i]<<" dropped"<<endl;
				}
				// check lmt availability compare from stat buf only if both are accesible if target is not accessible make it
				if(IsFilesAccessible && IsTargetAccessible)
				{
					double s = stats.st_mtime - statbuf->st_mtime;
					if(s > 0)
					{
						IsSourceModified = true;
					}
				}
			}
			if(t->IsInference)
			{
				char temp[100];
				sprintf(temp,"%s%s",t->MatchedRule,t->s1);
				struct stat stats;
				if(!ExecuteMake(temp,&stats))IsFilesAccessible = false;

				if(IsFilesAccessible && IsTargetAccessible)
				{
					double s = stats.st_mtime - statbuf->st_mtime;
					if(s > 0)
					{
						IsSourceModified = true;
					}
				} 
			}
			if(IsFilesAccessible && (IsSourceModified || !IsTargetAccessible)) 
			{
				if(IsDFlagSet)
				{
					cout<<"\tActions:"<<endl;
				}
				t->RunCommandLines();
				stat(makeTarget,statbuf);// != -1)
				// {
				IsTargetAccessible = true;
				// } 
			}
		}
		if(IsDFlagSet)
		{
			cout<<"Done making "<<makeTarget<<endl;
		}
		return IsTargetAccessible;
	} 
};


Rules *rule ;
void print_database()
{
	cout<<"Macros : "<<rule_stats->n_macros<<endl;
	cout<<"Targets : "<<rule_stats->n_targets<<endl;
	cout<<"Inferences : "<<rule_stats->n_inferences<<endl;
	cout<<"Command Lines : "<<rule_stats->n_commandline<<endl;
	cout<<"commands : "<<rule_stats->n_commands<<endl;
	Macro** m = rule->macros;
	for(int i=0; i< rule->n_macros; i++)
	{
		cout<<m[i]->LHS<<"="<<m[i]->RHS<<endl;
	}
	Target** t = rule->targets;
	for(int i=0; i< rule->n_targets; i++)
	{
		cout<<t[i]->name<<":";
		char** p = t[i]->dependents;
		for(int j =0 ; j < t[i]->n_deps; j++)
		{
			cout<<" "<<p[j];
		}
		for(int j= 0; j < t[i]->n_cmdln; j++)
		{
			CommandLine** cl = t[i]->commandLines;
			char** c = cl[j]->commands;
			cout<<endl;
			for(int k = 0; k< cl[j]->n_cmd; k++)
			{
				if(k < cl[j]->n_cmd -1)
					cout<<c[k]<<"; ";
				else cout<<c[k];
			}
		}
		cout<<endl;
	}
	exit(0);
}

bool checkWhiteSpace(char* s)
{
	int i = 0;
	bool IsWhiteSpace= false;
	while(s[i]!='\0')
	{
		if(!isspace(s[i]))
		{
			IsWhiteSpace = true;
			break;
		}
		else i++;
	}
	return IsWhiteSpace;
}

void read_make_file(const char* make)
{
	char line[100],s[100],comment[100] ;
	rule_stats = new RuleStat();
	rule = new Rules();
	FILE* fp = fopen(make,"r");    
	if(fp)
	{
		fgets(line, 100, fp);
		line[strlen(line) - 1] = '\0';
		while (!feof(fp))
		{ 
			if(strlen(line)!=0)
			{
				sscanf(line,"%[^#]#%s",s,comment);
				if(strlen(s)!=0 && checkWhiteSpace(s))
					rule->identify_rule(s);
				line[0] = '\0';
				s[0] = '\0';
			}
			fgets(line, 100, fp);
			line[strlen(line) - 1] = '\0';
		}
		fclose(fp);
		if(IsPFlagSet)
		{
			print_database();
		}
		struct stat stats;
		if(makearg==NULL)
		{
			if(rule->n_targets > 0)
			{
				makearg = rule->targets[0]->name;
			}
		}
		check_circular_deps(makearg);
		rule->ExecuteMake(makearg,&stats);
		remove_from_chain(makearg);
	}
	else
	{
		printf("Cannot read the configfile!");
		fflush(stdout);
		exit(1); 
	}
	delete rule;
	delete rule_stats;
}



void check_default_makefile()
{
	int i = 0;
	const char *default_files[] = { "mymake1.mk","mymake2.mk","mymake3.mk", NULL};
	struct stat statbuf;
	bool got_file = false;
	if(Makefile != NULL)got_file = true;
	while(!got_file && default_files[i] != NULL)
	{
		if(stat(default_files[i],&statbuf) != -1)
		{
			Makefile = default_files[i];
			got_file = true;
		} 
		else i++;
	}
	if(!got_file)
	{
		printf("No make file found in the current directory\n");
	}
	else
	{
		read_make_file(Makefile);
	}
}

class MyAlarm
{
public :
	struct sigaction act;
	MyAlarm(int time)
	{
		struct sigaction act;
		act.sa_handler = alarm_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0;
		sigaction(SIGALRM, &act, 0);
		alarm(time); 
	}
	static void alarm_handler(int signo)
	{
		kill(0,SIGKILL);
	}
	~MyAlarm(){}
};

class InteruptHandler
{
public:
	struct sigaction act;
	InteruptHandler(bool IsBlock)
	{
		if(!IsBlock)
			act.sa_handler = interupt_handler; 
		else act.sa_handler = SIG_IGN;
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0;
		sigaction(SIGINT,&act,NULL);
	}

	static void interupt_handler(int signo)
	{
		kill(0,SIGKILL);
	}
	~InteruptHandler(){}
};

void GetMyPath()
{
	Paths = new char*[50];
	n_path = 0;
	char* myval = getenv("MYPATH");
	if(myval == NULL)
	{
		cout<<"ENVIRONMENT VARIABLE : MYPATH"<<endl;
		exit(19);
	}
	char*tok = strtok(myval,":");
	while(tok != NULL)
	{
		Paths[n_path] = new char[strlen(tok)+1];
		strcpy(Paths[n_path],tok);
		tok = strtok(NULL,":");
		n_path++;
	}

}

int main(int argc, char** argv)
{
	int c,option_index=0;    
	InteruptHandler *IntBlk = NULL;
	MyAlarm* alrm = NULL;
	//    const char *value = getenv("PATH");
	//  const char *name = "MYPATH";
	// setenv(name,value,1);
	while ((c = getopt_long_only (argc, argv, "f:pkdit:",long_options, &option_index)) != EOF)
	{
		switch (c)
		{

		case 'f':	Makefile = (char*)optarg; 			
			break;

		case 'p':      	IsPFlagSet = true;		
			break;	

		case 'k':       IsKFlagSet = true;
			break;	

		case 'd':       IsDFlagSet = true;
			break;

		case 'i':       IntBlk = new InteruptHandler(true) ; 
			break;

		case 't':       alrm = new MyAlarm(atoi(optarg));
			break;				

		case ':':	break;

		case '?':	break;									
		}
	}
	while(optind < argc)
	{
		makearg = argv[optind];   /* demo clean */
		optind++;
	}     
	if(IntBlk == NULL)
		IntBlk = new InteruptHandler(false);
	GetMyPath();    
	check_default_makefile();

	delete IntBlk;
	delete alrm;
	return 0;
}

