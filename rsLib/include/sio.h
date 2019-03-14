#ifndef _SIO_H_
#define _SIO_H_

#include "defines.h"

void sio_init();
void sio_stop();

char sio_putchar(char c);
int sio_getchar();
uchar sio_rxcount();

#endif /* _SIO_H_ */
