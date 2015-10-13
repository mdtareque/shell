#include "miniShell.h"
// signal handler for sigint
void sigIntHandler(int signo) {
    //write(STDERR_FILENO, "CtrlC", 5);
    strcpy(logBuf, "Caught CtrlC\0");
    tstamp(WARN, logBuf, shellPid);
    printf("\n%s", PROMPT);
    fflush(stdout);
    signal(SIGINT, sigIntHandler);               // reset handler
}

void execute_cd(char *in) {
    tstamp(INFO, "builtin cd", shellPid);
    char out[MAXLINE];
    char cdTo[MAXLINE];
    char *resolved = realpath(getLineSkippedWord(in, out), NULL);

    strcpy(cdTo, cmds[0].tokens[1]);
    if(strcmp(cdTo, "-") == 0) {
        sprintf(logBuf, "going to old is %s", oldPWD);
        tstamp(INFO, logBuf, getpid());
        cwd = getcwd(NULL, 0);                   // get current working directory for display
        chdir(oldPWD);
        strcpy(oldPWD, cwd);
        memset(PROMPT, 0, sizeof(char) * MAX);
        strcpy(PROMPT, "mini-shell@");
        // need to be fully fixed
        cwd = getcwd(NULL, 0);                   // get current working directory for display
        strcat(PROMPT, cwd);
        strcat(PROMPT, "> ");
        sprintf(logBuf, "PROMPT updated to [%s]", PROMPT);
        tstamp(INFO, logBuf, shellPid);
    } else if(resolved != NULL) {
        cwd = getcwd(NULL, 0);                   // get current working directory for display
        if(strcmp(cwd, oldPWD) != 0)
            strcpy(oldPWD, cwd);
        chdir(resolved);
        memset(PROMPT, 0, sizeof(char) * MAX);
        strcpy(PROMPT, "mini-shell@");
        cwd = getcwd(NULL, 0);                   // get current working directory for display
        strcat(PROMPT, cwd);
        strcat(PROMPT, "> ");
        sprintf(logBuf, "PROMPT updated to [%s]", PROMPT);
        tstamp(INFO, logBuf, shellPid);
        free(resolved);
    } else {
        sprintf(logBuf, "Not able to get realpath of [%s]",
                cmds[0].tokens[1]);
        tstamp(INFO, logBuf, shellPid);
    }
}
void execute_pwd() {
    tstamp(INFO, "builtin pwd", shellPid);
    cwd = getcwd(NULL, 0);                       // get current working directory for display
    printf("%s\n", cwd);
}

void execute_echo(char *in) {
    // resolve variables
    // display from initial input string
    char output[MAXLINE];
    tstamp(INFO, "builtin echo", shellPid);
    printf("%s\n", getLineSkippedWord(in, output));
}

void execute_export(char *in, char *ke, char *valu, int export) {
    //tstamp(INFO, "builtin export", shellPid);
    char output[MAXLINE];
    char *exportArg = in;
    if(export == 1) {
        exportArg = getLineSkippedWord(in, output);
    }
    //sprintf(logBuf, "got exportArg[%s] export[%d]", exportArg, export);
    //tstamp(TRACE, logBuf, shellPid);
    int len = strlen(exportArg);
    int indexOfEqualSign = strstr(exportArg, "=") - exportArg;
    //sprintf(logBuf, "got indexOfEqualSign[%d]", indexOfEqualSign);
    //tstamp(TRACE, logBuf,shellPid);
    char *key = (char *) calloc(1, indexOfEqualSign - 0 + 1);   // end -start + 1 (for null character)
    memcpy(key, exportArg, indexOfEqualSign);
    char *value = (char *) calloc(1, len - indexOfEqualSign);
    memcpy(value, exportArg + indexOfEqualSign + 1,
           len - indexOfEqualSign);
    //sprintf(logBuf, "key=[%s], value=[%s]", key, value);
    //tstamp(INFO, logBuf, shellPid);
    if(export == 1)
        setenv(key, value, 1);                   // overwrite!
    if(ke != NULL && valu != NULL) {
        strcpy(ke, key);
        strcpy(valu, value);
        //sprintf(logBuf, "setting outbound ke[%s] and value[%s]", ke, valu);
        //tstamp(INFO, logBuf, shellPid);
    }
    free(key);
    free(value);
}

void execute_env() {
    tstamp(INFO, "builtin env", shellPid);
    char **p;
    for(p = environ; *p; ++p)
        printf("%s\n", *p);
}

void execute_assign(char *in) {                  // treat simple assign as export, to ease echo
    tstamp(INFO, "builtin assign", shellPid);
    char modifiedIn[MAXLINE];
    strcpy(modifiedIn, "export ");
    strcat(modifiedIn, in);
    execute_export(modifiedIn, NULL, NULL, 1);
}

char *resolveEnvVariables(char *in) {
    tstamp(TRACE, "in resolve env variable", shellPid);
    char **p;
    char key[MAXLINE], value[MAXLINE], tmp2[MAXLINE] = "$", *tmp;
    int i = 0;
    for(p = environ; *p; ++p) {
        i++;
        //sprintf(tmp2, "running for %d", i); tstamp(INFO, tmp2, shellPid);
        strcpy(key, "");
        strcpy(value, "");
        execute_export(*p, key, value, 0);
        //printf("after execute export %s\n", in);
        //tstamp(TRACE, "trying to replace each env var", shellPid);
        if(key != NULL && value != NULL) {
            //sprintf(logBuf,"substituiting %s-%s:%s\n", *p, key, value);
            //tstamp(INFO, logBuf ,shellPid);
            tmp2[0] = '$';
            tmp2[1] = '\0';
            tmp = strReplace(in, strcat(tmp2, key), value);
            //printf("in is .%s.\n ", in);
            if(strcmp(in, tmp) != 0) {
                strcpy(in, tmp);
                sprintf(logBuf, "after replace in is %s", in);
                tstamp(TRACE, logBuf, shellPid);
            }
            free(tmp);
        }
    }
    sprintf(logBuf, "EnvVar resolved %s", in);
    tstamp(INFO, logBuf, shellPid);
    return in;
}
