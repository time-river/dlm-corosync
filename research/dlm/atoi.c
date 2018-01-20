#include <stdio.h>
#include <stdlib.h>

int main(void){
    printf("'  1234' --> %d\n'12abc' --> %d\n'abcd' --> %d\n'12 3' --> %d\n'123\\n12' --> %d\n",
            atoi("  1234"), atoi("12abc"), atoi("abcd"), atoi("12 3"), atoi("123\n12"));

    return 0;
}
