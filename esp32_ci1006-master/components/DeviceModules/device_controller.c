#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "led.h"
#include "DeviceCommon.h"
#include "device_controller.h"
//#include "device_info.h"

#define CONT_TAG "DEVICE_CO"

#define LINE_1 "\
.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:.\n\
.     *                                       .\n\
.    /.\\                                     .\n\
.   /..'\\                                     .\n\
.   /'.'\\               ESP32                 .\n\
.  /.''.'\\                                    .\n\
.  /.'.'.\\                                    .\n\
. /.'HAO '\\                                  .\n\
. ^^^[_]^^^                                   .\n\
.                                             .\n\
.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:.\n"


static void add_module(DeviceController *device, DeviceModule *module)
{
	DeviceModule *tmp;

	if(device == NULL || module == NULL)
		return;

	tmp = device->modules;
	module->next = NULL;
	if(tmp == NULL) {
		
		device->modules = module;
	} else {
		while(tmp->next != NULL) {
			tmp = tmp->next;
		}
		tmp->next = module;
	}
}

static void active_all_modules(DeviceController *device)
{
	DeviceModule *tmp;

	if(device == NULL || device->modules == NULL)
		return;

	tmp = device->modules;
	while(tmp->next != NULL) {
		if(tmp->active)
			tmp->active(device);
		tmp = tmp->next;
	}
	if(tmp->active)
		tmp->active(device);
	
}

static void notify_modules(DeviceController *device, enum DeviceNotifyType type, void *data, int len)
{
	DeviceModule *tmp;

	if(device == NULL || device->modules == NULL)
		return;

	tmp = device->modules;
	while(tmp->next != NULL) {
		if(tmp->notified)
			tmp->notified(device, type, data, len);
		tmp = tmp->next;
	}
	if(tmp->notified)
		tmp->notified(device, type, data, len);
}

int device_controller_init(DeviceController **handle){

	DeviceController *device = (DeviceController *) calloc(1, sizeof(DeviceController));

	memset(device, 0x00, sizeof(DeviceController));
	device->add_module = add_module;
	device->active_all_modules = active_all_modules;
	device->notify_modules = notify_modules;
	
	*handle = device;
	nvs_flash_init();
	UserGpioInit();
	printf("\n%s", LINE_1);
	return 0;
}
