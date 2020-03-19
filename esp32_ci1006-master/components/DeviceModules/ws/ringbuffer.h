
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "freertos/timers.h"
//#include "freertos/semphr.h"




typedef struct 	RingBuffer
{
   char *data;
   int   size;
   int   read_ptr;
   int   write_ptr;
   int   available;
   xSemaphoreHandle mutex;
   //xSemaphoreHandle buffer_mutex
}tRingBuffer;

int ring_buffer_init(tRingBuffer* st, int size);

void ring_buffer_free(tRingBuffer *st);

//不覆盖写接口
int ring_buffer_write(tRingBuffer *st, char*data, int len);
//循环覆盖写接口
int ring_buffer_overwrite(tRingBuffer *st, char*data, int len);


int ring_buffer_writezeros(tRingBuffer *st, int len);

int ring_buffer_read(tRingBuffer *st, char *data, int len);

//清空buffer数据
int ring_buffer_reset(tRingBuffer *st);

//int ring_buffer_get_available(tRingBuffer *st);



#ifdef __cplusplus
}
#endif

#endif




