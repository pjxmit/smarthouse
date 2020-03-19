#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include "esp_log.h"
#include "driver/touch_pad.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "touchpad.h"
#include "userconfig.h"
#include "device_controller.h"
#include "DeviceCommon.h"

#define TOUCH_TAG "TOUCH_MANAGER"

#define TOUCHPAD_THRESHOLD                      400
#define TOUCHPAD_FILTER_VALUE                   150

static QueueHandle_t xQueueTouch;
static DeviceController *device_control = NULL;

static void ProcessTouchEvent(void *evt)
{
    touchpad_msg_t *recv_value = (touchpad_msg_t *) evt;
    int touch_num = recv_value->num;
    DeviceNotifyMsg msg = DEVICE_NOTIFY_KEY_UNDEFINED;

	ESP_LOGI(TOUCH_TAG, "touch_num=%d", touch_num);

    switch (touch_num) {

		case TOUCH_1:
			if (recv_value->event == TOUCHPAD_EVENT_TAP) {
	            //ESP_LOGI(TOUCH_TAG, "TOUCH_1 S");
				msg = DEVICE_NOTIFY_KEY_TP_1_S;
	        } else if (recv_value->event == TOUCHPAD_EVENT_LONG_PRESS) {
				ESP_LOGI(TOUCH_TAG, "TOUCH_1 L");
				msg = DEVICE_NOTIFY_KEY_TP_1_L;
				if(device_control->smartConfig) {
					device_control->smartConfig();
				}
	        } else if (recv_value->event == TOUCHPAD_EVENT_RELEASE) {
				ESP_LOGI(TOUCH_TAG, "TOUCH_1 R");
				msg = DEVICE_NOTIFY_KEY_TP_1_R;
	        } else if (recv_value->event == TOUCHPAD_EVENT_PUSH) {
				ESP_LOGI(TOUCH_TAG, "TOUCH_1 P");
				msg = DEVICE_NOTIFY_KEY_TP_1_R;
	        }
			break;

		case TOUCH_2:
			if (recv_value->event == TOUCHPAD_EVENT_TAP) {
	            ESP_LOGI(TOUCH_TAG, "TOUCH_2 S");
				msg = DEVICE_NOTIFY_KEY_TP_2_S;
	        } else if (recv_value->event == TOUCHPAD_EVENT_LONG_PRESS) {
				ESP_LOGI(TOUCH_TAG, "TOUCH_2 L");
				msg = DEVICE_NOTIFY_KEY_TP_2_L;
	        } else if (recv_value->event == TOUCHPAD_EVENT_RELEASE) {
				ESP_LOGI(TOUCH_TAG, "TOUCH_2 R");
				msg = DEVICE_NOTIFY_KEY_TP_2_R;
	        }
			break;

	    default:
	        ESP_LOGI(TOUCH_TAG, "Not supported line=%d", __LINE__);
	        break;

    }

    //if (msg != DEVICE_NOTIFY_KEY_UNDEFINED) {
		//if(device_control->notify_modules)
			//device_control->notify_modules(device_control, DEVICE_NOTIFY_TYPE_KEY, &msg, sizeof(DeviceNotifyMsg));
    //}
}

static void TouchManagerEvtTask(void *pvParameters)
{
    touchpad_msg_t recv_value = {0};
    while (1) {
        if (xQueueReceive(xQueueTouch, &recv_value, portMAX_DELAY)) {
            ProcessTouchEvent(&recv_value);        }
    }
    vTaskDelete(NULL);
}

static void active_module_touch_key(DeviceController *device)
{
	xQueueTouch = xQueueCreate(4, sizeof(touchpad_msg_t));
    configASSERT(xQueueTouch);
	ESP_LOGI(TOUCH_TAG, "active");

	 //touchpad
	touchpad_create(TOUCH_1, TOUCHPAD_THRESHOLD, TOUCHPAD_FILTER_VALUE, TOUCHPAD_SINGLE_TRIGGER, 2, &xQueueTouch, 10);
    //touchpad_create(TOUCH_2, TOUCHPAD_THRESHOLD, TOUCHPAD_FILTER_VALUE, TOUCHPAD_SINGLE_TRIGGER, 2, &xQueueTouch, 10);
    touchpad_init();

	xTaskCreatePinnedToCore(&TouchManagerEvtTask,
						"touch",
						MODULE_KEY_TASK_STACK_SIZE,
						device,
						MODULE_KEY_TASK_PRIORITY,
						NULL,
						MODULE_KEY_TASK_CORE);
}


DeviceModule *create_module_touch_key(DeviceController *handle)
{
	DeviceModule *module = (DeviceModule *) calloc(1, sizeof(DeviceModule));

	device_control = handle;
	memset(module, 0x00, sizeof(DeviceModule));
	module->active = active_module_touch_key;
	module->notified = NULL;
	handle->add_module(handle, module);
	return module;
}
