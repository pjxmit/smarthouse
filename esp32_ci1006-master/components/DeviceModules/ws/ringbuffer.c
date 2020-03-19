#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "ringbuffer.h"



//#define RDEBUG
#if 1
int ring_buffer_init(tRingBuffer* st, int size)
{
	if (!st)
	{
		fprintf(stderr, "ring buffer init error\n\r");
		return -1;
	}
	if ((st->mutex = xSemaphoreCreateMutex()) == NULL)
	{	
		fprintf(stderr, "can't create mutex\n");
		return -1;		
	}	
	st->data = malloc(size);
	st->size = size;
	st->read_ptr = 0;
	st->write_ptr = 0;
	st->available = 0;
	return 0;
}

//不覆盖写接口
int ring_buffer_write(tRingBuffer *st, char*data, int len)
{
   int end;
   int end1;
   
	xSemaphoreTake(st->mutex, portMAX_DELAY);
	//printf("buf wirte\n");
	if (st->available + len > st->size) //there can take other policy
	{
		xSemaphoreGive(st->mutex);
		fprintf(stderr, "ring buffer full\n\r");
		return 0;
	}
	end = st->write_ptr + len;
	end1 = end;
	if (end1 > st->size)
		end1 = st->size;
	memcpy(st->data + st->write_ptr, data, end1 - st->write_ptr);
	if (end > st->size)
	{
		end -= st->size;
		memcpy(st->data, data + end1-st->write_ptr, end);
	}
	st->available += len;

	st->write_ptr += len;
	if (st->write_ptr >= st->size)
		st->write_ptr -= st->size;
	xSemaphoreGive(st->mutex);
	return len;
}


int ring_buffer_overwrite(tRingBuffer *st, char*data, int len)
{

	int end;
	int end1;

	xSemaphoreTake(st->mutex, portMAX_DELAY);
	#ifdef RDEBUG
	printf("@w@w:%d, r:%d, size:%d\n", st->write_ptr,st->read_ptr,st->available);
	#endif
	if (st->available + len > st->size) //there can take other policy
	{
		#ifdef RDEBUG
		fprintf(stderr, "ring buffer overwrite:%d < %d + %d\n\r", st->size, st->available, len);
		#endif
	}
	end = st->write_ptr + len;
	end1 = end;
	if (end1 > st->size)
		end1 = st->size;
	memcpy(st->data + st->write_ptr, data, end1 - st->write_ptr);
	if (end > st->size)
	{
		end -= st->size;
		memcpy(st->data, data + end1 - st->write_ptr, end);
	}

	st->write_ptr += len;
	if (st->write_ptr >= st->size)
		st->write_ptr -= st->size;
	st->available += len;
	if (st->available >= st->size)
	{
		#ifdef RDEBUG
		printf("@ww@w:%d, r:%d, size:%d\n", st->write_ptr,st->read_ptr,st->available);
		#endif
		st->available = st->size;
		st->read_ptr = st->write_ptr;
		#ifdef RDEBUG
		printf("@ww@w:%d, r:%d, size:%d\n", st->write_ptr,st->read_ptr,st->available);
		#endif
	}
	

	xSemaphoreGive(st->mutex);
	return len;

}



int ring_buffer_writezeros(tRingBuffer *st, int len)
{
  
	int end;
	int end1;
	xSemaphoreTake(st->mutex, portMAX_DELAY);

	if (len > st->size)
	{
		len = st->size;
	}
	
	end = st->write_ptr + len;
	end1 = end;
	if (end1 > st->size)
	end1 = st->size;
	memset(st->data + st->write_ptr, 0, end1 - st->write_ptr);
	if (end > st->size)
	{
		end -= st->size;
		memset(st->data, 0, end);
	}
	st->available += len;
	if (st->available >= st->size)
	{
		st->available = st->size;
		st->read_ptr = st->write_ptr;
	}
	st->write_ptr += len;
	if (st->write_ptr >= st->size)
		st->write_ptr -= st->size;
	xSemaphoreGive(st->mutex);
	return len;
}

int ring_buffer_read(tRingBuffer *st, char *data, int len)
{
	int end, end1;

	if (st->available == 0)
		return 0;
	xSemaphoreTake(st->mutex, portMAX_DELAY);

	if (len > st->available)
	{
		len = st->available;
	}
	end = st->read_ptr + len;
	end1 = end;
	if (end1 > st->size)
		end1 = st->size;
	memcpy(data, st->data + st->read_ptr, end1 - st->read_ptr);

	if (end > st->size)
	{
		end -= st->size;
		memcpy(data + end1-st->read_ptr, st->data, end);
	}
	#ifdef RDEBUG
	printf("@r@r:%d, r:%d, size:%d\n", st->write_ptr,st->read_ptr,st->available);
	#endif
	st->available -= len;
	st->read_ptr += len;
	if (st->read_ptr >= st->size)
		st->read_ptr -= st->size;
	#ifdef RDEBUG 
	printf("@r@r:%d, r:%d, size:%d\n", st->write_ptr,st->read_ptr,st->available);
	#endif
	xSemaphoreGive(st->mutex);
	return len;
}

//清空buffer数据
int ring_buffer_reset(tRingBuffer *st)
{
	xSemaphoreTake(st->mutex, portMAX_DELAY);
	st->available = 0;
	st->read_ptr = 0;
	st->write_ptr = 0;
	xSemaphoreGive(st->mutex);
	return 0;
}

void ring_buffer_free(tRingBuffer *st)
{
	if (st)
	{
		ring_buffer_reset(st);
		xSemaphoreGive(st->mutex);
		st->mutex = NULL;
		free(st->data);
		st->size = 0;
	}
	
}
#endif
