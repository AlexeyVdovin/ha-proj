#include <stdio.h>

#include "minini/minIni.h"

#define array_sz(a)  (sizeof(a)/sizeof((a)[0]))

int main(int argc, char* argv[])
{
    int n;
    char str[100] = "";
    
    if(argc > 1)
    {
        n = ini_gets(NULL, "string", "dummy", str, array_sz(str), argv[1]);
        printf("cfg: '%s'\n", str);
    }

    return 0;
}
