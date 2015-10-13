#include "miniShell.h"
void writeHistFileAgain() {
    close(histfd);
    histfd =
        open(HISTFILE, O_CREAT | O_WRONLY | O_TRUNC,
             S_IRWXU | S_IRGRP | S_IROTH);
    int toWriteFrom = histIndex - MAXHIST + 1;
    int i;
    if(toWriteFrom < 0)
        toWriteFrom = 0;
    //printf("writing hist records from %d to %d\n", toWriteFrom, histIndex);
    for(i = toWriteFrom; i < histIndex + 1; i++) {
        sprintf(histBuf, "%d\t%s\n", hist[i].no, hist[i].cmd);
        write(histfd, histBuf, strlen(histBuf));
    }
    histIndex = -1;
    //printf("Reinitializing internal hist datastrucutre\n");
    readFromHistFile();                          // again to populate the internal ds
}

void addToHistWithNo(int histNo, char *cmdline) {
    ++histIndex;
    hist[histIndex].no = histNo;
    strcpy(hist[histIndex].cmd, cmdline);
}
void addToHist(char *cmdline) {
    if(histIndex == -1) {
        histNo = 1;                              // histNo will be set if histfile has any data, it would be last histno
    }
    ++histIndex;
    hist[histIndex].no = histNo;
    strcpy(hist[histIndex].cmd, cmdline);
    sprintf(histBuf, "%d\t%s\n", histNo, cmdline);
    write(histfd, histBuf, strlen(histBuf));
    histNo++;
    if(histIndex >= MAXHIST - 2) {
        // delete first 1000,better copy rest to 1..1000 and set the histIndex
        //printf("history limited exceeded, writing last 10 record to histfile\n");
        writeHistFileAgain();
    }
    //printf("HistIndex %d, histNo %d\n", histIndex, histNo);
}

void readFromHistFile() {
    FILE *fp = fopen(HISTFILE, "r");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char digit[10];
    int j, hno, lineNo = 0, i;
    if(fp == NULL) {
        histNo = 1;
    } else {
        while ((read = getline(&line, &len, fp)) != -1) {
            j = 0;
            i = 0;
            line[strlen(line) - 1] = '\0';
            //printf("%s\n", line);
            while (isdigit(line[i]))
                digit[j++] = line[i++];
            digit[j] = '\0';
            sscanf(digit, "%d", &hno);
            while (isblank(line[i]))
                i++;
            //printf("after skipping histno[%d] cmd[%s]\n", hno, line);
            addToHistWithNo(hno, line + i);
            lineNo++;
            if(lineNo > MAXHIST)
                break;

        }
        histNo = hno + 1;
        fclose(fp);
    }
}
void displayHist(char *lastn) {
    int i = 0, n;
    if(lastn == NULL)
        n = histIndex;
    else
        sscanf(lastn, "%d", &n);
    if(n < 0) {
        sprintf(logBuf, "minShell: history: %d: invalid option\n", n);
        printf("%s", logBuf);
        tstamp(INFO, logBuf, getpid());
        return;
    }
    if(n > MAXHIST)
        n = MAXHIST;
    for(i = histIndex - n + 1; i <= histIndex; i++)
        printf("  %d\t%s\n", hist[i].no, hist[i].cmd);
}

void runPrevious(){
    char cmd[MAXLINE];
    strcpy(cmd,hist[histIndex].cmd);
    executeCmd(cmd, 0);
}

void runNthHistCmd(int n) {
    int found=0, i;
    char cmd[MAXLINE];
    for(i=0;i<histIndex; i++) {
        if(hist[i].no == n) {
            found=1;
            strcpy(cmd,hist[i].cmd);
            executeCmd(cmd, 0);
            break;
        }
    }
    if(found==0)
        printf("miniShell: !%d: event not found\n", n);
}
void runNthHistCmdFromBack(int n){
    int found=0, i;
    char cmd[MAXLINE];
    if(n<=histIndex) {
        for(n=histIndex; n>=0; n--) { }
        found=1;
        strcpy(cmd,hist[n].cmd);
        executeCmd(cmd, 0);
    }
    if(found==0)
        printf("miniShell: !-%d: event not found\n", n);

}
