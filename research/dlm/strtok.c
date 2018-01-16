#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    char *line = NULL;
    size_t n = 0;
    ssize_t count;
    char *str, *subtoken, *saveptr;

    while(true){
        count = getline(&line, &n, stdin);
        printf("input: %s count: %zd size: %zu\n", line, count, n);
        for(str=line; ; str=NULL){
            subtoken = strtok_r(str, " \n", &saveptr);
            if(subtoken == NULL)
                break;
            printf(" --> %s str: %s saveptr: %s\n", subtoken, str, saveptr);
        } 
    }

    free(line);
    return 0;
}
