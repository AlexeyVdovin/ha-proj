#ifndef _PCA9554_H_
#define _PCA9554_H_

#define PCA9554_MAX_DEV 4

// registers
#define PCA9554_INPUT  0x00
#define PCA9554_OUTPUT 0x01
#define PCA9554_INV    0x02
#define PCA9554_DIR    0x03

void init_pca9554(int id);
void close_pca9554(int id);

void set_pca9554(int id, int n, int val);

    
#endif