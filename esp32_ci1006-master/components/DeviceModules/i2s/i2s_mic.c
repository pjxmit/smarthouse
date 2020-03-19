/* mic array main.c
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2s.h"
#include "esp_log.h"

#include "userconfig.h"
#include "DeviceCommon.h"
#include "device_controller.h"
#include "device_info.h"
#include "peaklevel.h"


#include "music_data.h"

#ifndef CONFIG_ETH_MODE
#define I2S_OUT         1
#endif

#define I2S_NUM         (0)
#define SAMPLE_RATE     (16000)
#define BITS_PER_SAMPLE (32) // fixed 32bits
#define SAMPLE_NUM      ((SAMPLE_RATE/1000)*64) // samples number: 16 * time(ms)
#define DATA_LEN_I2S    (SAMPLE_NUM*2*(BITS_PER_SAMPLE/8)) // 2 channel , 2 bytes


//DMA BUFFER
#define DMA_BUFFER_LEN ((SAMPLE_RATE/1000)*16) // 16ms(256 samples)
#define DMA_BUFFER_COUNT (3*(SAMPLE_NUM/DMA_BUFFER_LEN + SAMPLE_NUM%DMA_BUFFER_LEN))

static const char* TAG = "I2S";

static char buffer[DATA_LEN_I2S];

extern void buffer_push(char* data, int size, float level);


static void recvData_preProcess(char *indata, size_t size)
{
	int i;
	int *tmp = (int *)indata;
	float *tmp_f = (float *)(indata + size/2);
	short *tmp_s = (short *)indata;

	i = 0;
	while(i < SAMPLE_NUM) {
		*(tmp_s + i) = (short)((*(tmp + i*2))>>16);
		i+=1;
	}

	i = 0;
	while(i < SAMPLE_NUM) {
		*(tmp_f + i) = ((float)(*(tmp_s + i)))/32767;
		i+=1;
	}
}

static void sendData_preProcess(char *indata, size_t size)
{
	#if 1
	int i;
	int *tmp = (int *)indata + SAMPLE_NUM*2 -1;
	short *tmp_s = (short *)indata + SAMPLE_NUM -1;

	i = 0;
	while(i < SAMPLE_NUM) {
		*(tmp - i*2)	= (int)((*(tmp_s- i))<<16);
		*(tmp - i*2 -1) = (int)((*(tmp_s- i))<<16);
		i+=1;
	}
	
	#else
	int i;
	int *tmp = (int *)indata;
	float *tmp_f = (float *)indata + SAMPLE_NUM;

	i = 0;
	while(i < SAMPLE_NUM) {
		*(tmp + i*2)	 = ((int)((*(tmp_f + i))*32767))<<16;
		*(tmp + i*2 + 1) = ((int)((*(tmp_f + i))*32767))<<16;
		i+=1;
	}
	#endif
}

esp_err_t i2s_init(void)
{
	esp_err_t err;
#ifdef I2S_OUT
	i2s_config_t i2s_1_config = {
		.mode = I2S_MODE_MASTER | I2S_MODE_TX,
		.sample_rate = SAMPLE_RATE,
		.bits_per_sample = BITS_PER_SAMPLE,
		.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
		.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
		.dma_buf_count = DMA_BUFFER_COUNT,
		.dma_buf_len = DMA_BUFFER_LEN,
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1	//Interrupt level 1
	};

	i2s_pin_config_t i2s_1_pin_config = {
		.bck_io_num = 32,
        .ws_io_num = 33,
        .data_out_num = 27,
        .data_in_num = -1
	};

	err = i2s_driver_install(1, &i2s_1_config, 0, NULL);
	if (err != ESP_OK) {
		return err;
	}

	err = i2s_set_pin(1, &i2s_1_pin_config);
	if (err != ESP_OK) {
		return err;
	}
#endif
	i2s_config_t i2s_config = {
        .mode = I2S_MODE_SLAVE | I2S_MODE_RX,                                  // Only TX
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = DMA_BUFFER_COUNT,
        .dma_buf_len = DMA_BUFFER_LEN,
        .use_apll = false,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1                                //Interrupt level 1
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_0_PIN_BCK,//25,
        .ws_io_num = I2S_0_PIN_WS,//26,
        .data_out_num = -1,
        .data_in_num = I2S_0_PIN_DATA_IN//35                                               //Not used
    };
    err = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
	if (err != ESP_OK)
		return err;

	err = i2s_set_pin(I2S_NUM, &pin_config);
	if (err != ESP_OK)
		return err;
	return ESP_OK;

}

static void i2s_task(void *arg)
{
	int data_len = DATA_LEN_I2S;
	size_t size_tmp;
	ESP_LOGI(TAG, "i2s_task #####################\n");
#if 1
	void *peaklevel = NULL;
	tPeakLeavelParam peaklevel_para = {1, 0.15, 5, 0.3};
	float level;
	float *start_p = (float *)buffer + SAMPLE_NUM;

	peaklevel = peakLevel_init(SAMPLE_RATE, &peaklevel_para);
	peakLevel_set_param(peaklevel, &peaklevel_para);
	while(1) {
		i2s_read(I2S_NUM, buffer, data_len, &size_tmp, portMAX_DELAY);
	#ifndef I2S_OUT
		recvData_preProcess(buffer, data_len);
		level = peakLevel_process(peaklevel, start_p, SAMPLE_NUM)+100;
		buffer_push(buffer, SAMPLE_NUM*sizeof(short), level);
	#else
		recvData_preProcess(buffer, data_len);
		level = peakLevel_process(peaklevel, start_p, SAMPLE_NUM)+100;
        buffer_push(buffer, SAMPLE_NUM*sizeof(short), level);
		//printf("%f\n", level);
		sendData_preProcess(buffer, data_len);
		i2s_write(1, buffer, data_len, &size_tmp, portMAX_DELAY);
	#endif
	}
#else
    while (1) {
        i2s_write(1, StartupPcmData, sizeof(StartupPcmData), &size_tmp, portMAX_DELAY);
    }
#endif
	vTaskDelete(NULL);
}

static void active_module_i2s(DeviceController *device)
{
	ESP_LOGI(TAG, "active");
	i2s_init();
	
	xTaskCreatePinnedToCore(&i2s_task,
							"i2s_task",
							MODULE_I2S_TASK_STACK_SIZE,
							device,
							MODULE_I2S_TASK_PRIORITY,
							NULL,
							MODULE_I2S_TASK_CORE);
}

DeviceModule *create_module_i2s(DeviceController *handle)
{
	DeviceModule *module = (DeviceModule *) calloc(1, sizeof(DeviceModule));

	memset(module, 0x00, sizeof(DeviceModule));
	module->active = active_module_i2s;
	module->notified = NULL;

	handle->add_module(handle, module);
	return module;
}
