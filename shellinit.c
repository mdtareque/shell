#include "miniShell.h"
void init(int argc, char *argv[]) {
    bgIndex = 0;
    numberOfCmds = 0;

    char *home = getenv("HOME");
    setenv("~", home, 1);

    // catch ctrl-c signal
    signal(SIGINT, sigIntHandler);
    // open logfile
    logfd =
        open(LOGFILE, O_CREAT | O_WRONLY | O_APPEND,
             S_IRWXU | S_IRGRP | S_IROTH);

    histIndex = -1;
    histNo = 1;
    // read from hist file and populate data structure
    readFromHistFile();
    histfd =
        open(HISTFILE, O_CREAT | O_WRONLY | O_APPEND,
             S_IRWXU | S_IRGRP | S_IROTH);

    // get current main process pid
    shellPid = getpid();
    CMDSIZE = 512;

    // override logleve '-l [0-4]' via commandline
    if(argc > 1) {
        char *tmp = argv[1];
        if(strcmp(tmp, "-l") == 0 && argc > 2) {
            int l = atoi(argv[2]);
            if(l >= 0 && l < 5) {
                LOGLEVEL = l;
                sprintf(logBuf, "loglevel set to %d %s", LOGLEVEL,
                        logLevelName(LOGLEVEL));
                tstamp(WARN, logBuf, shellPid);
            } else {
                sprintf(logBuf,
                        "loglevel passed [%d] is not in range [0-4]. Default is still %s",
                        l, logLevelName(LOGLEVEL));
                tstamp(WARN, logBuf, shellPid);
            }
        }
    }
    // set starting prompt
    cwd = getcwd(NULL, 0);                       // get current working directory for display
    strcpy(PROMPT, "mini-shell@");
    strcat(PROMPT, cwd);
    strcat(PROMPT, "> ");
}
