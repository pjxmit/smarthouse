#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "device_controller.h"
#include "WifiManager.h"
#include "esp32_shell.h"
#include "ota_server.h"
#include "ws_client.h"
#include "TouchManager.h"
#include "sdkconfig.h"
#include "uart_module.h"
#include "i2s_mic.h"
#include "ethernet.h"

#define MAIN_TAG "APP_MAIN"

void app_main()
{
	DeviceController *device;

	device_controller_init(&device);
#ifdef CONFIG_ETH_MODE
	printf("\n*********\nETH MODE\n*********\n");
#else
	printf("\n*********\nWIFI MODE\n*********\n");
#endif

	//add modules
	//ethernet
#ifdef CONFIG_ETH_MODE
	create_module_ethernet(device);
#endif
	//ws
	create_module_ws(device);

	//i2s
	create_module_i2s(device);

	//ci1006
	create_module_ci1006(device);

	//wifi
#ifndef CONFIG_ETH_MODE
	create_module_wifi(device);
#endif
	//shell
	create_module_shell(device);

	//key
	//create_module_touch_key(device);

	//ota
	//create_module_ota(device);

	device->active_all_modules(device);
}
