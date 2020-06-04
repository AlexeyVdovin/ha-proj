#include <stdio.h>

#include "minini/minIni.h"
#include "wiringPi/wiringPi.h"
#include "mcp23017.h"
#include "stm.h"

#define array_sz(a)  (sizeof(a)/sizeof((a)[0]))
char* cfg_name = "wallsw.conf";

int main(int argc, char* argv[])
{
    int n;
    char str[100] = "";
    
    if(argc > 1)
    {
        cfg_name = argv[1];
    }
    n = ini_gets("mqtt", "ip", "127.0.0.1", str, array_sz(str), cfg_name);
    printf("ip: '%s'\n", str);

    n = ini_gets("mqtt", "port", "1883", str, array_sz(str), cfg_name);
    printf("port: '%s'\n", str);

    return 0;
}
