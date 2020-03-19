/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/socket.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "userconfig.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

#include "device_controller.h"
#include "DeviceCommon.h"
#include "driver/uart.h"
#include "led.h"

#include "iot_ota.h"


#define WAIT_TIMEOUT_IN_SECOND 90
static DeviceController *device_control = NULL;

static const char *TAG = "ota";


static void ota_event_upload(void)
{
	DeviceNotifyMsg msg = DEVICE_NOTIFY_SYS_OTA;

	if(device_control->notify_modules)
		device_control->notify_modules(device_control, DEVICE_NOTIFY_TYPE_SYS, &msg, sizeof(DeviceNotifyMsg));
}

static void ota_task_http(void *pvParameter)
{
	esp_err_t ret = ESP_FAIL;

	LedIndicatorSet(LedIndexNet, LedWorkState_NetSetting);
	ret = iot_ota_start_url((const char *)pvParameter, WAIT_TIMEOUT_IN_SECOND*1000 / portTICK_RATE_MS);
	LedIndicatorSet(LedIndexNet, LedWorkState_NetConnectOk);
	if(ret == ESP_OK)
		esp_restart();
}

static int ota_process(char type, void *data, int len)
{
	static char ota_start = 0;
	void (*ota_task)(void *pvParameter);
	char buf[512];

	if(ota_start)
		return -1;

	ota_start = 1;
	ota_event_upload();
	if(type == OTA_TYPE_TCP_CLIENT) {
		ota_task = ota_task_http;
	}
    ESP_LOGI(TAG, "esp_ota_begin succeeded");
	strcpy(buf, (char *)data);
    if(xTaskCreatePinnedToCore(ota_task,
							"ota_task",
							WIFI_OTA_TASK_STACK_SIZE,
							buf,
							WIFI_OTA_TASK_PRIORITY,
							NULL, WIFI_OTA_TASK_CORE) != pdPASS) {
    	ESP_LOGE(TAG, "ERROR creating ota_task! Out of memory?");
	}
	return 0;

}

DeviceModule *create_module_ota(DeviceController *handle)
{
	DeviceModule *module = (DeviceModule *) calloc(1, sizeof(DeviceModule));

	device_control = handle;
	memset(module, 0x00, sizeof(DeviceModule));
	module->active = NULL;
	module->notified = NULL;
	handle->otaProcess = ota_process;
	handle->add_module(handle, module);
	return module;
}
