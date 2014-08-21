#ifndef _QUEUE_H_
#define _QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

void* queue_get();
void queue_put(void* item);
int queue_len();

#ifdef __cplusplus
}
#endif


#endif
