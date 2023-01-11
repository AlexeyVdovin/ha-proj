#ifndef _PCA9554_H_
#define _PCA9554_H_

#define PCA9554_SLAVE_ADDR 0x20

// registers
#define PCA9554_INPUT  0x00
#define PCA9554_OUTPUT 0x01
#define PCA9554_INV    0x02
#define PCA9554_DIR    0x03

enum
{
   PCA9554_OUT_X11 = 0, // G1/G2 heat
   PCA9554_OUT_X10,     
   PCA9554_OUT_X2,      // G1/G2 vent 
   PCA9554_OUT_X1,      
   PCA9554_OUT_X6,      // G1 circ
   PCA9554_OUT_X8,      // G2 circ
   PCA9554_DIS_1W1,
   PCA9554_DIS_1W2
};

void init_pca9554();
void close_pca9554();

void set_pca9554(int n, int val);

    
#endif