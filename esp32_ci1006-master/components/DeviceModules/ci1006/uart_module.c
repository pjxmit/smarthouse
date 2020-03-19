#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "rom/crc.h"


#include "userconfig.h"
#include "DeviceCommon.h"
#include "device_controller.h"
#include "device_info.h"


#define UART_TAG "CI1006_MODULE"


#define START_0           0xA5
#define START_1           0xFA
#define END               0xFB

#define DATA_LEN          6

#define OFFSET_ID         2
#define OFFSET_CMD        3
#define OFFSET_DATA       4
#define OFFSET_CHECKSUM   6
#define OFFSET_END        7

#define VAD_TIMEOUT_MS        (5*1000)
#define VAD_SMOOTH_TIMEOUT_MS (500)
#define VAD_AGAIN_TIMEOUT_MS  (10*64)

#define MIN_LEVEL_VALUE       (60)

static int in_vad = 0;

static esp_timer_handle_t handle_start_again = 0;
static esp_timer_handle_t handle_end = 0;
static esp_timer_handle_t handle_start = 0;



static uart_port_t Uart_Num = UART_NUM_2;
static uint8_t uart_data[64];
static int run_flag = 1;
static DeviceController *device_control = NULL;

static xQueueHandle GpioEvtQueue = NULL;

static int flag = 0;

extern int get_wakeup_status(void);
extern float get_peaklevel(void);



static void update_ci1006(enum DeviceNotifyType type, int event)
{
	DeviceNotifyMsg msg = event;

	if(device_control->notify_modules) {
        device_control->notify_modules(device_control, type, &msg, sizeof(DeviceNotifyMsg));
	}
}


static void timer_start(esp_timer_handle_t *handle, esp_timer_create_args_t *arg, int timeout_ms) {
	esp_timer_create(arg, handle);
	esp_timer_start_once(*handle, timeout_ms*1000);
}

static void timer_stop(esp_timer_handle_t *handle) {
	if(*handle) {
		esp_timer_stop(*handle);
	    esp_timer_delete(*handle);
		*handle = 0;
	}
}


// 1
/*****************************************************************************************/


static void handle_start_again_cb(void *arg) {
	esp_timer_handle_t *handle = (esp_timer_handle_t *)arg;
    esp_timer_delete(*handle);
	*handle = 0;

	flag = 1;
	ESP_LOGI(UART_TAG, "handle_start_again_cb");
	gpio_num_t gpioNum = (gpio_num_t) CPIO_VAD_VALUE;
    if (GpioEvtQueue)
        xQueueSend(GpioEvtQueue, &gpioNum, 0);
}


static esp_timer_create_args_t handle_start_again_arg = {
        .callback = &handle_start_again_cb,
        .arg = (void *)&handle_start_again,
        .name = "VadStartAgainTimer"
        };

/*****************************************************************************************/


// 2
/*****************************************************************************************/


static void handle_end_cb(void *arg) {
	esp_timer_handle_t *handle = (esp_timer_handle_t *)arg;
    esp_timer_delete(*handle);
	*handle = 0;
	in_vad = 0;
	timer_stop(&handle_start);
	ESP_LOGI(UART_TAG, "handle_end_cb");
	ESP_LOGI(UART_TAG, "VAD END");
	update_ci1006(DEVICE_NOTIFY_TYPE_CI1006, DEVICE_NOTIFY_CI1006_VAD_END);
}

static esp_timer_create_args_t handle_end_arg = {
        .callback = &handle_end_cb,
        .arg = (void *)&handle_end,
        .name = "VadEndTimer"
        };

/*****************************************************************************************/



// 3
/*****************************************************************************************/


static void handle_start_cb(void *arg) {
	esp_timer_handle_t *handle = (esp_timer_handle_t *)arg;
    esp_timer_delete(*handle);
	*handle = 0;
	in_vad = 0;
	ESP_LOGI(UART_TAG, "handle_start_cb");
	ESP_LOGI(UART_TAG, "VAD END 11");
	update_ci1006(DEVICE_NOTIFY_TYPE_CI1006, DEVICE_NOTIFY_CI1006_VAD_END);

	if(get_wakeup_status()) {
		timer_start(&handle_start_again, &handle_start_again_arg, VAD_AGAIN_TIMEOUT_MS);
	}
}

static esp_timer_create_args_t handle_start_arg = {
        .callback = &handle_start_cb,
        .arg = (void *)&handle_start,
        .name = "VadStartTimer"
        };


/*****************************************************************************************/

static void IRAM_ATTR vad_gpio_intr_handler(void* arg)
{
    gpio_num_t gpioNum = (gpio_num_t) CPIO_VAD_VALUE;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (GpioEvtQueue)
        xQueueSendFromISR(GpioEvtQueue, &gpioNum, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}


static void gpio_vad_init(void)
{
	gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.pin_bit_mask     = GPIO_VAD_SEL;
    io_conf.mode             = GPIO_MODE_INPUT;
    io_conf.pull_up_en       = GPIO_PULLUP_ENABLE;
    io_conf.intr_type        = GPIO_PIN_INTR_ANYEDGE ;
    gpio_config(&io_conf);
	gpio_isr_handler_add(CPIO_VAD_VALUE, vad_gpio_intr_handler, NULL);
}


static int uart_init(void)
{
	 uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(Uart_Num, &uart_config);
    uart_set_pin(Uart_Num, UART_2_TX, UART_2_RX, -1, -1);
    // We won't use a buffer for sending data.
    uart_driver_install(Uart_Num, 1024 * 2, 512 * 2, 0, NULL, 0);
	return 0;
}

static void debug_dump()
{
#if 0
	printf("/***********************************/\n");
	printf("ID:    0x%02X\n", uart_data[OFFSET_ID]);
	printf("CMD:   0x%02X\n", uart_data[OFFSET_CMD]);
	printf("DATA0: 0x%02X\n", uart_data[OFFSET_DATA]);
	printf("DATA1: 0x%02X\n", uart_data[OFFSET_DATA+1]);
	printf("CHSUM: 0x%02X\n", uart_data[OFFSET_CHECKSUM]);
	printf("/***********************************/\n");
#else
	ESP_LOGI(UART_TAG, "CMD:  0x%02X", uart_data[OFFSET_CMD]);
	ESP_LOGI(UART_TAG, "DATA: 0x%02X", uart_data[OFFSET_DATA]);
#endif
}

static void uart_data_process(void)
{
	int ret = -1;
	//short datalen;
	//unsigned char crc;
	//unsigned char *response_data = NULL;
	//int   response_len  = 0;

	ret = uart_read_bytes(Uart_Num, uart_data+OFFSET_ID, DATA_LEN, 500/portTICK_RATE_MS);
	if(ret >= DATA_LEN && uart_data[OFFSET_END] == END) {
		debug_dump();
		if(uart_data[OFFSET_CMD] == 0x81) {
			if(uart_data[OFFSET_DATA] == 0x01 || uart_data[OFFSET_DATA] == 0x02) {
				update_ci1006(DEVICE_NOTIFY_TYPE_CI1006, DEVICE_NOTIFY_CI1006_WAKEUP);
				flag =1;
			}else {
				update_ci1006(DEVICE_NOTIFY_TYPE_CI1006_INSTRUCTIONS, (int)uart_data[OFFSET_DATA]);
			}
		} else if(uart_data[OFFSET_CMD] == 0x82 && uart_data[OFFSET_DATA] == 0x00) {
			update_ci1006(DEVICE_NOTIFY_TYPE_CI1006, DEVICE_NOTIFY_CI1006_WAKEUP_END);
		}
	}
}

static void vad_task(void *pv)
{
	int value;
	gpio_num_t gpioNum;

	while(1) {
		if(xQueueReceive(GpioEvtQueue, &gpioNum, portMAX_DELAY) == pdPASS) {
			if(gpioNum == CPIO_VAD_VALUE) {
				value = gpio_get_level(gpioNum);
				//ESP_LOGI(UART_TAG, "CPIO_VAD_VALUE = %d", value);
                if(flag) {
                    in_vad = 0;
                    flag = 0;
                    //ESP_LOGI(UART_TAG, "CPIO_VAD_VALUE = %d, vad_value_pre = %d", value, vad_value_pre);
                }
				timer_stop(&handle_start_again);
				timer_stop(&handle_end);
				if(!in_vad)
					timer_stop(&handle_start);
				if(value == 0 && !in_vad){
					in_vad = 1;
					ESP_LOGI(UART_TAG, "VAD START");
					//if(get_peaklevel() > MIN_LEVEL_VALUE)
						update_ci1006(DEVICE_NOTIFY_TYPE_CI1006, DEVICE_NOTIFY_CI1006_VAD);
					timer_start(&handle_start, &handle_start_arg, VAD_TIMEOUT_MS);
				} else if(value == 1){
					timer_start(&handle_end, &handle_end_arg, VAD_SMOOTH_TIMEOUT_MS);
				}
			}
		}
	}
	vTaskDelete(NULL);
}


static void uart_task(void *pv)
{
	int len;
	while (run_flag) {
		memset(uart_data, 0x00, sizeof(uart_data));
        len = uart_read_bytes(Uart_Num, uart_data, 1, portMAX_DELAY);
        if (len > 0 && uart_data[0] == START_0) {
			len = uart_read_bytes(Uart_Num, uart_data + 1, 1, portMAX_DELAY);
	        if (len > 0 && uart_data[1] == START_1) {
				uart_data_process();
	        }
        }
    }
	vTaskDelete(NULL);
}

static void active_module_uart(DeviceController *device)
{
	ESP_LOGI(UART_TAG, "active");
	GpioEvtQueue = xQueueCreate(3, sizeof(int));
    configASSERT(GpioEvtQueue);
	uart_init();
	device_control = device;
	
	xTaskCreatePinnedToCore(&uart_task,
							"uart_task",
							MODULE_UART_TASK_STACK_SIZE,
							device,
							MODULE_UART_TASK_PRIORITY,
							NULL,
							MODULE_UART_TASK_CORE);


	gpio_vad_init();
	xTaskCreatePinnedToCore(&vad_task, 
							"vad_task", 
							MODULE_VAD_TASK_STACK_SIZE, 
							device, 
							MODULE_VAD_TASK_PRIORITY, 
							NULL, 
							MODULE_VAD_TASK_CORE);
}

DeviceModule *create_module_ci1006(DeviceController *handle)
{
	DeviceModule *module = (DeviceModule *) calloc(1, sizeof(DeviceModule));

	memset(module, 0x00, sizeof(DeviceModule));
	module->active = active_module_uart;
	module->notified = NULL;

	handle->add_module(handle, module);
	return module;
}
