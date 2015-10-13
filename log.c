#include "miniShell.h"

// log_level mnemonic: return name of n-th month
char *logLevelName(int n) {
    static char *name[] = { "TRACE", "DEBUG", "WARN", "INFO", "FATAL" };
    return (n < 0 || n > 4) ? name[0] : name[n];
}

// return local formatted time
void getTime(char *t) {
    time_t now;
    struct tm *lcltime;
    char formatted_lcltime[40];
    now = time(NULL);
    lcltime = localtime(&now);
    strftime(formatted_lcltime, sizeof(formatted_lcltime),
             "%d-%b %I:%M:%S %p", lcltime);
    strcpy(t, formatted_lcltime);
}

// log utility to prepend time , logLevel and message
void tstamp(int loglevel, char *buf, pid_t childPid) {
    if(loglevel >= LOGLEVEL) {
        char time[40], buf_local[MAXBUFFER] = "";
        getTime(time);
        if(childPid != shellPid)
            sprintf(buf_local, "[child %d] ", childPid);
        sprintf(buf_local + strlen(buf_local), "[bg:%d]", bgIndex);
        sprintf(buf_local + strlen(buf_local), "[%s] [%s] %s\n", time,
                logLevelName(loglevel), buf);
        write(logfd, buf_local, strlen(buf_local));
    }
}
