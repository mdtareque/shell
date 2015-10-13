#include "miniShell.h"

int isNotBlank(char *in) {
    int i = 0;
    while (isblank(in[i]))
        i++;
    if(in[i] != '\0')
        return 0;
    else
        return 1;
}

// check if path is regular file
int isFile(const char *path) {
    struct stat buf;
    stat(path, &buf);
    return S_ISREG(buf.st_mode);
}

// returns non-zero if the file is a file in the system path, and executable
int isExecutableInPath(char *name) {
    char *path = getenv("PATH");
    char *item = NULL;
    int found = 0;

    if(!path)
        return 0;
    path = strdup(path);

    char real_path[PATH_MAX];
    for(item = strtok(path, ":"); (!found) && item;
        item = strtok(NULL, ":")) {
        sprintf(real_path, "%s/%s", item, name);
        if(isFile(real_path) && !(access(real_path, F_OK) || access(real_path, X_OK)))  // check if the file exists and is executable
            found = 1;
    }
    free(path);
    return found;
}

// You must free the result if result is non-NULL.
char *strReplace(char *orig, char *rep, char *with) {
    char *result;                                // the return string
    char *ins;                                   // the next insert point
    int len_rep;                                 // length of rep
    int len_with;                                // length of with
    int len_front;                               // distance between rep and end of last rep
    int noOfOccurrences;                         // number of replacements
    char *tmp;

    //sprintf(logBuf,"strReplace orig[%s], rep[%s], with[%s]", orig, rep, with);
    //tstamp(trace, logBuf, shellPid);
    if(!orig)
        return NULL;
    if(!rep)
        rep = "";                                //return NULL;
    len_rep = strlen(rep);
    if(!with)
        with = "";                               //return NULL;
    len_with = strlen(with);

    ins = orig;
    for(noOfOccurrences = 0; (tmp = strstr(ins, rep)); ++noOfOccurrences)
        ins = tmp + len_rep;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result =
        malloc(strlen(orig) + (len_with - len_rep) * noOfOccurrences + 1);

    if(!result)
        return NULL;

    while (noOfOccurrences--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;             // goto next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

// return 'abc' from '   echo  abc'
char *getLineSkippedWord(char *in, char *output) {
    int i = 0, j = 0;
    int INSIDEQUOTES = 0;
    while (in[i] == ' ' || in[i] == '\t')
        ++i;                                     // skip any initial spaces or tabs
    while (in[i] != ' ')
        i++;                                     // skip word 'echo' or 'export'
    i++;                                         // skip initial space
    while (in[i] != '\0') {
        if((in[i] == '"' || in[i] == '\'') && INSIDEQUOTES == 0)
            INSIDEQUOTES = 1;
        else if((in[i] == '"' || in[i] == '\'') && INSIDEQUOTES == 1)
            INSIDEQUOTES = 0;
        else if(INSIDEQUOTES == 0 && in[i] == '#')
            break;
        else
            output[j++] = in[i];
        i++;
    }
    output[j] = '\0';
    return output;
}
