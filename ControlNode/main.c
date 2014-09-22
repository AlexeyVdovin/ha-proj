#ifndef __AVR__

int main()
{

    return 0;
}

#else /* __AVR__ */

#include "timer.h"
#include "led_display.h"
#include "sio.h"
#include "packet.h"
#include "rs485.h"

int main()
{
    timer_init();
    disp_init();
    sio_init();
    rs485_init();
    
    disp[0] = 0x10;
    disp_digit(1, 4);
    return 0;
}

#endif /* __AVR__ */
