
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<errno.h>
#include<time.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<limits.h>
#include<ctype.h>

#define LOGFILE ".201505521_minishell.log"
#define MAXBUFFER 4096
#define TRACE 0
#define DEBUG 1
#define WARN 2
#define INFO 3
#define FATAL 5

#define PIPE_CHAR "|"
#define SPACE_CHAR " "
#define MAX 256
#define MAXLINE 4096

#define CD "cd"
#define PWD "pwd"
#define EXPORT "export"
#define ECHO "echo"
#define HISTORY "history"
#define FG_CMD "fg"

#define MAXHISTLINE 128
#define MAXHIST 10
#define HISTFILE ".201505521_sh_history"

extern char **environ;
extern int errno;

extern int logfd;
extern pid_t shellPid;
extern int LOGLEVEL; // default to INFO

extern char PROMPT[MAX];
extern char *cwd;

extern pid_t bgProc[MAX][2];
extern int bgIndex;
extern char logBuf[MAXBUFFER]; // just stores logging messages, used extensively
extern int isBg;


// structure to hold command and it's options and arguments
struct command{
    char *commandName;
    char *tokens[MAX];
    int inredirect;
    int outredirect;
    char infile[MAX];
    char outfile[MAX];
    int argsLen;
};
extern struct command cmds[MAX]; // stores all commands delimited by pipe

struct history {
    int no;
    char cmd[MAXHISTLINE];
};
extern struct history hist[MAXHIST+10];


extern int numberOfCmds;
extern int CMDSIZE;
extern int histIndex;
extern int histfd;
extern int histNo;
extern char histBuf[MAXHISTLINE];
extern char oldPWD[MAXLINE];

void parseArgIntoCmds(char *); // parse the given command
void displayCmds(); // utility to display parsed data
void init(int , char **); // initialize the commands array
void freeCmds(); // free up memory
void sigIntHandler(int signo);
char *logLevelName(int passedLogLevel);
void tstamp(int loglevel, char *buf, pid_t childPid);
void executeCmd(char *, int addToHist);
void getTime(char *t);
void readFromHistFile();
void writeHistFileAgain();
void addToHistWithNo(int histNo, char *cmdline);
void addToHist(char *cmdline);
void displayHist(char *lastn);
void sigIntHandler(int signo);
int isFile(const char* path);
int isExecutableInPath(char *name);
char *strReplace(char *orig, char *rep, char *with);
int typeOfCommand(char *s[MAX], char *in);
char *getLineSkippedWord(char *in, char *output);
void execute_cd(char *in);
void execute_pwd();
void execute_echo(char *in);
void execute_export(char *in, char *ke, char *valu, int export);
void execute_env();
void execute_assign(char *in) ; // treat simple assign as export, to ease echo
char *resolveEnvVariables(char *in);
int isNotBlank(char *in);
void runPrevious();
void runNthHistCmd(int n);
void runNthHistCmdFromBack(int n);
