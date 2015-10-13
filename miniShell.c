/*
1. infinite input
2. quit on 'exit'
3. ctrl-c caught, ctrl-d also in free, prompt reset in signalHandler
4. tstamp logging working
5. parseCmds in-corporated from pipe assignment
6. loglevel override via cmd line '-l [0-4]' working
7. re-org, moved declaration above, folds made
8. single pipe added, reset noOfCmds to 0, output is interleaved to be fixed.
9. vi, clear, ls, pwd, bc all working
10. environment variable resovled - all occurrences of same env-var also handled
11. Bonus, ctrl-c killing child
12. ~ replaced with $HOME and resolveEnvVariable taking care of replacing it everywhere
13. Empty/blank space and tabs only lines ignored
14. history , history n all working

STOPPED:
!-2
!3
!vi

next todo:
* builtins

future todo:
add infile, outfile after identifying < and > redirection operators
replace '|' with ' | ', '<' with ' < ' and '>' with ' > ' that are not within quotes
replace | < >  with in quotes with ascii character before tokenization and later replace them back in cmds array

 */

#include "miniShell.h"
#define BANG_CHAR '!'

char PROMPT[MAX];
char *cwd;
pid_t shellPid;

pid_t bgProc[MAX][2];
int bgIndex = 0;
int isBg = 0;

int LOGLEVEL = 0;                                // default to INFO
char logBuf[MAXBUFFER];                          // just stores logging messages, used extensively

int logfd;
struct command cmds[MAX];                        // stores all commands delimited by pipe

struct history hist[MAXHIST + 10];
int numberOfCmds;
int CMDSIZE;
int histIndex;
int histfd;
int histNo;
char histBuf[MAXHISTLINE];
char oldPWD[MAXLINE];

// parse the given command and store it as tokens
void parseArgIntoCmds(char *argv) {

    char *token, *saveptr1, *str2, *str1, *saveptr2, *subtoken;
    int j = 0, k=0, i, c;
    int inQuotes =0, len;

    // TODO: improve parsing logic
    // 1. handle redirection identification and store in/out file
    // 2. handle |<> in quotes
    // 3. handle |<> with space around them

    char dup[MAXLINE], orig[MAXLINE];
    int changed=0;
    for(i=0 ; argv[i] != '\0'; i++) {
        //putchar(argv[i]);
        changed=0;
        c=argv[i];
        if((c == '"' || c == '\'') && inQuotes == 0) {
            changed=2;
            inQuotes=1;
        } else if((c == '"' || c == '\'') && inQuotes == 1) {
            changed=2;
            inQuotes=0;
        } else if(inQuotes==1){
            if(c==' ') {
                dup[k++]=21;
                changed=1;
            } else if(c=='|') {
                dup[k++]=22;
                changed=1;
            }
        } else {
            if(c=='<') {
                dup[k++]=32;
                dup[k++]='<';
                dup[k++]=32;
                changed=1;
            } else if(c=='>') {
                dup[k++]=32;
                dup[k++]='>';
                dup[k++]=32;
                changed=1;
            }

        }
        if(changed==0) {
            dup[k++]=c;
        }
    }
    dup[k++]=0;
//    printf("%s\n", argv); printf("%s\n", dup);
    strcpy(orig, argv); strcpy(argv, dup);

    for(str1 = argv;; str1 = NULL) {
        k = 0;
        token = strtok_r(str1, PIPE_CHAR, &saveptr1);
        if(token == NULL)
            break;
        //printf("\n");
        for(str2 = token;; str2 = NULL) {
            subtoken = strtok_r(str2, SPACE_CHAR, &saveptr2);
            if(k == 0) {
                cmds[j].commandName = subtoken;
            }

            if(subtoken != NULL) {
                if(strcmp(subtoken, "&") == 0) {
                    isBg = 1;
                } else if(strcmp(subtoken, "<") == 0) {
                    tstamp(INFO, "got inredirect", getpid());
                    cmds[j].inredirect = 1;
                    subtoken = strtok_r(str2, SPACE_CHAR, &saveptr2);
                    strcpy(cmds[j].infile, subtoken);
                } else if(strcmp(subtoken, ">") == 0) {
                    tstamp(INFO, "got outredirect", getpid());
                    cmds[j].outredirect = 1;
                    subtoken = strtok_r(str2, SPACE_CHAR, &saveptr2);
                    strcpy(cmds[j].outfile, subtoken);
                } else {
                    cmds[j].tokens[k] = subtoken;
                    k++;
                }
            }

            if(subtoken == NULL)
                break;
            //printf(" --> %s \n", subtoken);
        }
        cmds[j].tokens[k] = NULL;
        cmds[j].argsLen = k;
        j++;
    }
    numberOfCmds = j;

    for(i = 0; i < numberOfCmds; i++) {
        for(j = 0; j < cmds[i].argsLen; j++) {
            len=strlen(cmds[i].tokens[j]);
            for(k=0; k<len; k++) {
                if(cmds[i].tokens[j][k] ==  21) cmds[i].tokens[j][k]=32;
                else if(cmds[i].tokens[j][k] ==  22){ cmds[i].tokens[j][k]=124; }
            }
        }
    }


    //printf("argv %s\n", argv);
}

// display parsed data
void displayCmds() {
    int i, j;
    strcpy(logBuf, "Parsed tokens:");
    for(i = 0; i < numberOfCmds; i++) {
        sprintf(logBuf + strlen(logBuf), "\n%s: ", cmds[i].commandName);
        if(cmds[i].inredirect == 1)
            sprintf(logBuf, " Input is redirected from %s\n",
                    cmds[i].infile);
        if(cmds[i].outredirect == 1)
            sprintf(logBuf, " Output is redirected from %s\n",
                    cmds[i].outfile);
        for(j = 0; j < cmds[i].argsLen; j++) {
            sprintf(logBuf + strlen(logBuf), " <%s>", cmds[i].tokens[j]);
        }
    }
    tstamp(TRACE, logBuf, shellPid);
}

// free up allocated memory
void freeCmds() {
    int i;
    for(i = 0; i < CMDSIZE; i++) {
        free(&cmds[i]);
    }
}

// BI : BuiltIn, BANG : history related
enum CommandType { BI_ECHO, BI_EXPORT, BI_ASSIGN, BI_CD, BI_PWD, BANG_POSN, BANG_NEGN, BANG_STR, BANG_PREV, MULTI_PIPE, HISTN, NOP, FG };       //BI_ENV};

// original input required to identify some cases
int typeOfCommand(char *s[MAX], char *in) {
    if(s[0] != NULL && strlen(s[0]) > 0) {
        // check for simple assignment
        //printf("In typeOfCommand\n\n");
        if(numberOfCmds == 1 && strstr(in, "export") == NULL
           && strstr(in, "=") != NULL)
            return BI_ASSIGN;
        if(cmds[0].inredirect == 0 && cmds[0].outredirect == 0
           && numberOfCmds == 1) {
            if(strcmp(s[0], FG_CMD) == 0)
                return FG;
            if(strcmp(s[0], CD) == 0)
                return BI_CD;
            if(strcmp(s[0], PWD) == 0)
                return BI_PWD;
            if(strcmp(s[0], ECHO) == 0)
                return BI_ECHO;
            if(strcmp(s[0], EXPORT) == 0)
                return BI_EXPORT;
            if(strcmp(s[0], HISTORY) == 0)
                return HISTN;
        }

        if(strcmp(s[0], HISTORY) == 0) {         //&& numberOfCmds > 1) {
            // if no number is given
            if(cmds[0].tokens[1] == NULL) {
                sprintf(logBuf, "In history | grep");
                //printf("%s\n", logBuf);
                tstamp(INFO, logBuf, getpid());
                strcpy(cmds[0].commandName, "cat");
                strcpy(cmds[0].tokens[0], "cat");
                sprintf(logBuf, "%s", HISTFILE);
                cmds[0].tokens[1] = malloc(strlen(logBuf) + 1);
                strcpy(cmds[0].tokens[1], logBuf);
                cmds[0].tokens[2] = NULL;
                cmds[0].argsLen = 2;
                return MULTI_PIPE;
            } else {                             // parse the number if given
                int num;
                sprintf(logBuf, "In history 4 > out   ....");
                //printf("%s\n", logBuf);
                tstamp(INFO, logBuf, getpid());
                sscanf(cmds[0].tokens[1], "%d", &num);
                if(num < 0) {
                    sprintf(logBuf,
                            "minShell: history: %d: invalid option\n",
                            num);
                    //printf("%s", logBuf);
                    tstamp(INFO, logBuf, getpid());
                    return NOP;
                } else if(num >= 0) {
                    tstamp(INFO, "in history 4 > out, replacing cmds[0]",
                           getpid());
                    strcpy(cmds[0].commandName, "tail");
                    strcpy(cmds[0].tokens[0], "tail");
                    sprintf(logBuf, "-n%d", num);
                    cmds[0].tokens[1] = malloc(strlen(logBuf) + 1);
                    strcpy(cmds[0].tokens[1], logBuf);

                    sprintf(logBuf, "%s", HISTFILE);
                    cmds[0].tokens[2] = malloc(strlen(logBuf) + 1);
                    strcpy(cmds[0].tokens[2], logBuf);
                    cmds[0].tokens[3] = NULL;
                    cmds[0].argsLen = 3;
                    return MULTI_PIPE;

                }

            }
        }
        // todo
        char currCmd[MAXLINE];
        strcpy(currCmd, s[0]);
        if(currCmd[0] == BANG_CHAR) {
            if(currCmd[1] == BANG_CHAR) {
                return BANG_PREV;
            }
            if(currCmd[1] == '-' && isdigit(currCmd[2])) return BANG_NEGN;
            if(isdigit(currCmd[1])) return BANG_POSN;
        }
        // check for BANG +ve -ve and str
        return MULTI_PIPE;
    } else
        return 0;

}

void executeCmd(char *in, int addToHistory) {
    int i, n;
    char origIn[MAXLINE], tmp[MAXLINE];
    //printf("Prev in %s\n",in);
    in = strReplace(in, "~", "$HOME");
    strcpy(tmp, in);
    //    printf("after in %s\n",in);

    in = resolveEnvVariables(tmp);

    strcpy(origIn, in);
    parseArgIntoCmds(in);
    //printf("after parsing %s\n",origIn);
    displayCmds();
    int cmdType = typeOfCommand(cmds[0].tokens, in);
    displayCmds();
    sprintf(logBuf, "cmdType is %d numOfcmds %d", cmdType, numberOfCmds);
    tstamp(INFO, logBuf, shellPid);
    if(cmdType == NOP) {
        return;
    }
    if(cmdType == FG) {
        if(cmds[0].tokens[1] == NULL) {
            for(i = 0; i < bgIndex; i++)
                if(bgProc[i][1] == 1) {          // means we had entered it as bg process
                    // check if it's alive
                    if(kill(bgProc[i][0], 0) == 0) {    // [0] holds pid
                        /* process is running or a zombie */
                        // bring the first bg process up
                        waitpid(bgProc[i][0], NULL, 0);
                        bgProc[i][1] = 0;        // don't search next time
                    } else if(errno == ESRCH) {
                        // nothing to do in full search
                    }
                }
        } else {
            int pidToBringInFg, status;
            sscanf(cmds[0].tokens[1] + 1, "%d", &pidToBringInFg);  // fg #2352342
            //printf("fg number read %d\n", pidToBringInFg);
            if(kill(pidToBringInFg, 0) == 0) {   // [0] holds pid
                //printf("Waiting for child %d\n", pidToBringInFg);
                waitpid(pidToBringInFg, NULL, 0);

                signal(SIGINT, SIG_DFL);
                while (waitpid
                       (pidToBringInFg, &status,
                        WUNTRACED | WCONTINUED) != -1);
                signal(SIGINT, sigIntHandler);

            } else if(errno == ESRCH)
                printf("miniShell: fg: current: no such job\n");
        }
        return;
    }
    if(numberOfCmds == 1) {                      // find what type of command
        switch (cmdType) {
        case BI_ECHO:
            execute_echo(origIn);
            break;
        case BI_EXPORT:
            execute_export(origIn, NULL, NULL, 1);
            break;
        case BI_ASSIGN:
            execute_assign(origIn);
            break;
        case BI_CD:
            execute_cd(origIn);
            break;
        case BI_PWD:
            execute_pwd();
            break;
        case BANG_POSN:
            //printf("run bang posn\n");
            sscanf(origIn+1, "%d", &n);
            runNthHistCmd(n);
            break;
        case BANG_NEGN:
            //printf("run bang negn\n");
            sscanf(origIn+2, "%d", &n);
            runNthHistCmdFromBack(n);
            break;
        case BANG_STR:
            break;
        case BANG_PREV:
            runPrevious();
            break;
        case HISTN:
            displayHist(cmds[0].tokens[1]);
            break;
            //case BI_ENV: execute_env();break;
        }
    }
    if(addToHistory==1) {
        if(cmdType != BANG_PREV) {
            if(cmdType == BANG_POSN || cmdType == BANG_NEGN || cmdType == BANG_STR)
                addToHist(in);
            else
                addToHist(origIn);
        }
    }
    // resolve all idiosyncrasies before this
    // like history | grep   # history should be replaced by cat historyFile
    // like echo $abc | wc   # variables should be substituited
    if(cmdType == MULTI_PIPE && isBg == 0) {
        pid_t mainChild = fork();
        tstamp(INFO, "forked", getpid());
        if(mainChild == 0) {
            tstamp(INFO, "main child running ", getpid());
            int pifd[CMDSIZE * 2];               // [0] read end, [1] write end
            int i;

            for(i = 0; i < numberOfCmds; i++) {

                pid_t cpid;
                if(pipe(pifd + 2 * i) < 0) {
                    fprintf(stderr, "Pipe creation error\n");
                    exit(-1);
                }
                cpid = fork();

                if(cpid == 0) {                  // child
                    if(i != 0) {
                        close(STDIN_FILENO);
                        if(dup2(pifd[2 * (i - 1)], STDIN_FILENO) < 0) {
                            perror("Not able to dup stdin");
                            exit(-1);
                        }
                    }
                    if(i < numberOfCmds - 1) {
                        close(STDOUT_FILENO);
                        close(pifd[2 * i]);
                        if(dup2(pifd[2 * (i) + 1], STDOUT_FILENO) < 0) {
                            perror("Not able to dup stdout");
                            exit(-1);
                        }
                    }
                    if(cmds[i].inredirect == 1) {
                        //printf("duping infile\n");
                        int infd = open(cmds[i].infile, O_RDONLY);
                        if(infd < 0) {
                            perror("Error creating infile");
                        }
                        dup2(infd, STDIN_FILENO);
                    }
                    if(cmds[i].outredirect == 1) {
                        //printf("duping outfile\n");
                        int outfd =
                            open(cmds[i].outfile,
                                 O_WRONLY | O_CREAT | O_TRUNC,
                                 S_IRUSR | S_IWUSR);
                        if(outfd < 0) {
                            perror("Error creating outfile");
                        }

                        dup2(outfd, STDOUT_FILENO);
                    }
                        if(isExecutableInPath(cmds[i].commandName)) {
                            tstamp(INFO, "in running cmd", getpid());
                        } else {
                            sprintf(logBuf, "%s: command not found", cmds[i].commandName);
                            printf("%s\n",logBuf);
                            tstamp(INFO, logBuf, getpid());
                            exit(-1);
                        }
//                    printf("Running <%s> in child-child\n", cmds[i].commandName);
                    if(execvp(cmds[i].commandName, cmds[i].tokens) < 0) {
                        fprintf(stderr, "execvp error in parent\n");
                        exit(-1);
                    }

                } else if(cpid > 0) {
                    if(i != 0)
                        close(pifd[2 * (i - 1)]);
                    close(pifd[2 * i + 1]);
                }
            }
            for(i = 0; i < numberOfCmds; i++)
                wait(NULL);
            exit(0);
        }                                        // main child ending here
    }
    if(isBg == 1) {
        pid_t mainChild = fork();
        tstamp(INFO, "forked for bg", getpid());
        if(mainChild == 0) {
            if(setsid() == -1) {
                sprintf(logBuf,
                        "Child failed to become a session leader. [Not able to create background process]");
                tstamp(FATAL, logBuf, getpid());
                perror(logBuf);
            }
            if(execvp(cmds[0].commandName, cmds[0].tokens) < 0) {
                fprintf(stderr, "execvp error in parent\n");
                exit(-1);
            }
        }
        printf("[%d] %d\n", bgIndex + 1, mainChild);
        bgProc[bgIndex][0] = mainChild;
        bgProc[bgIndex++][1] = 1;                // 0 means running
        sprintf(logBuf, "Child running in backgroung %d", mainChild);
        tstamp(INFO, logBuf, getpid());
    }

    if(isBg == 0)
        for(i = 0; i < numberOfCmds; i++)
            wait(NULL);
    isBg = 0;
    for(i = 0; i < numberOfCmds; i++) {
        cmds[i].inredirect = 0;
        cmds[i].outredirect = 0;
    }
    numberOfCmds = 0;
}

int main(int argc, char *argv[]) {
    char line[MAXLINE];
    init(argc, argv);
    sprintf(logBuf, "Started minishell 201505521 pid[%d] cwd=[%s]", shellPid, cwd);     // log launch message
    tstamp(INFO, logBuf, shellPid);

    while (1) {
        printf("%s", PROMPT);
        line[0] = 0;
        fgets(line, MAXLINE, stdin);
        line[strlen(line) - 1] = '\0';

        if(!isNotBlank(line))
            tstamp(INFO, line, shellPid);
        else
            tstamp(INFO, "Empty input", shellPid);
//        printf("received %s\n", line);

        if(strcmp(line, "q") == 0 || strcmp(line, "exit") == 0) {
            printf("Bye...\n");
            tstamp(INFO, "Bye...", shellPid);
            break;
        }
        //printf("strlen(line) %d .%s.",strlen(line), line);
        if(!isNotBlank(line)) {
            executeCmd(line, 1);
        }

    }
    free(cwd);
    return 0;
}
