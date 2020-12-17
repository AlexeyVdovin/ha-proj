#ifndef _PCA9454_H_
#define _PCA9454_H_

#define PCA9454_MAX_DEV LIGHTS_MAX_DEV+POWER_MAX_DEV

// registers
#define PCA9454_INPUT  0x00
#define PCA9454_OUTPUT 0x01
#define PCA9454_INV    0x02
#define PCA9454_DIR    0x03

void init_pca9454(int id);
void close_pca9454(int id);

void set_pca9454(int id, int n, int val);
void msg_pca9454(int param, const char* message, size_t message_len);

    
#endif
