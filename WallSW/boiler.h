#ifndef _BOILER_H_
#define _BOILER_H_

#define OW_MAX_DEVCES 8  // Max 1W devices per channel

enum {
  OW_BOILER_TEMP1   =  0,
  OW_BOILER_TEMP2, //  1
  OW_BOILER_IN,    //  2
  OW_BOILER_OUT,   //  3
  OW_BOILER_RET,   //  4
  OW_FLOOR_IN,     //  5
  OW_FLOOR_OUT,    //  6
  OW_FLOOR_RET,    //  7
  OW_PIPE_OUT,     //  8
  OW_PIPE_RET,     //  9
  OW_HEAT_IN,      // 10 Gas heater in/out
  OW_HEAT_OUT,     // 11
  OW_HWATER_IN,    // 12 Hot water heater in/out
  OW_HWATER_OUT,   // 13
  OW_AMBIENT,      // 14
  OW_RESERVED      // 15
};


void init_boiler();
void close_boiler();

void setup_boiler_poll();
void handle_boiler();
void msg_boiler(int param, const char* message, size_t message_len);


#endif