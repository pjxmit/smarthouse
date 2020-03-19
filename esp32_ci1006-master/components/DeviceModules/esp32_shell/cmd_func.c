#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "tcpip_adapter.h"


#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_eth.h"

#include "device_controller.h"
#include "device_info.h"

#define TERM_TAG "TERM_CTRL"
#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( configGENERATE_RUN_TIME_STATS == 1 ) )
static uint8_t pcWriteBuffer[1024];
#endif
static int task_list_run = 0;

static void list_task(void *pv)
{
#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )
	while(task_list_run)
	{
		vTaskDelay(500 / portTICK_PERIOD_MS);

		vTaskGetRunTimeStats((char *)&pcWriteBuffer);
    	printf("\033[2J\033[f\nTask            count         CPU\r\n%s\n", pcWriteBuffer);	
	}
#endif
	vTaskDelete(NULL);
}

//---------system------
/***********************************************************************************************************/
void cmd_task(void *ref, int argc, char *argv[])
{
    (void)argv;
#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )
    vTaskList((char *)&pcWriteBuffer);
    printf("Task            Status  Prio   Stack Number\r\n");
    printf("%s\r\n%d\n", pcWriteBuffer, strlen((char *)pcWriteBuffer));
	printf("Status: R-ready B-blocked S-suspended D-deleted \r\n");
#endif

#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )
    vTaskGetRunTimeStats((char *)&pcWriteBuffer);
    printf("\r\nTask            count         CPU\r\n");
    printf("%s\r\n%d\n", pcWriteBuffer, strlen((char *)pcWriteBuffer));
#endif
	printf("Freemem Size: %dKB,%dKB\n", esp_get_free_heap_size()/1024, heap_caps_get_free_size(MALLOC_CAP_8BIT|MALLOC_CAP_INTERNAL)/1024);
}


void cmd_task_list(void *ref, int argc, char *argv[])
{
    (void)argv;
	if(task_list_run)
		return;
	task_list_run = 1;
	xTaskCreatePinnedToCore(&list_task, 
						"list_task", 
						2048, 
						NULL, 
						4, 
						NULL, 
						0);
}

void cmd_task_list_stop(void *ref, int argc, char *argv[])
{
	task_list_run = 0;
}

void cmd_reboot(void *ref, int argc, char *argv[])
{
    esp_restart();
}
/***********************************************************************************************************/



//---------wifi------
/***********************************************************************************************************/
void cmd_smartconf(void *ref, int argc, char *argv[])
{
    DeviceController *device_control = (DeviceController *)ref;

	if(device_control->smartConfig)
		device_control->smartConfig();
}

void cmd_wifiInfo(void *ref, int argc, char *argv[])
{
    wifi_config_t w_config;
    memset(&w_config, 0x00, sizeof(wifi_config_t));
    if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &w_config)) {
        ESP_LOGI(TERM_TAG, "Connected Wifi SSID:\"%s\"", w_config.sta.ssid);
    } else {
        ESP_LOGE(TERM_TAG, "failed");
    }
}

void cmd_wifiSet(void *ref, int argc, char *argv[])
{
    wifi_config_t w_config;
    memset(&w_config, 0x00, sizeof(wifi_config_t));
    if (argc == 2) {
        memcpy(w_config.sta.ssid, argv[0], strlen(argv[0]));
        memcpy(w_config.sta.password, argv[1], strlen(argv[1]));
        esp_wifi_disconnect();
        ESP_LOGI(TERM_TAG, "Connected Wifi SSID:\"%s\" PASSWORD:\"%s\"", w_config.sta.ssid, w_config.sta.password);
        esp_wifi_set_config(WIFI_IF_STA, &w_config);
        esp_wifi_connect();
    } else {
        ESP_LOGE(TERM_TAG, "format error, eg:\nwifiset ssid pwd");
    }
}
/***********************************************************************************************************/




//---------eth------
/***********************************************************************************************************/
void cmd_ethSet(void *ref, int argc, char *argv[])
{
    wifi_config_t w_config;
    memset(&w_config, 0x00, sizeof(wifi_config_t));

	int dhcp = 0;
	if(argc > 0){
		dhcp = atoi(argv[0]);
		if(dhcp == 0) {
			if(argc != 4)
				goto ERROR;
			write_eth_para(argv[1], argv[2], argv[3], 0);
		} else {
			write_eth_para(NULL, NULL, NULL, 1);
		}
		vTaskDelay(500 / portTICK_PERIOD_MS);
		esp_restart();
	}
ERROR:
	ESP_LOGE(TERM_TAG, "format error, eg:\nethset 1 //enable dhcp\nethset 0 ip gw mask //disable dhcp");
}


/***********************************************************************************************************/


//---------ip------
/***********************************************************************************************************/
void cmd_ip_wlan(void *ref, int argc, char *argv[])
{
	uint8_t ap_mac[6];
	tcpip_adapter_ip_info_t local_ip;

	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);
	esp_wifi_get_mac(ESP_IF_WIFI_STA, ap_mac);

    ESP_LOGI(TERM_TAG, "eth ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR ", mac: " MACSTR,
       IP2STR(&local_ip.ip),
       IP2STR(&local_ip.netmask),
       IP2STR(&local_ip.gw),
       MAC2STR(ap_mac));
}

void cmd_ip_eth(void *ref, int argc, char *argv[])
{
	uint8_t mac[6];
	tcpip_adapter_ip_info_t local_ip;

	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_ETH, &local_ip);
	esp_eth_get_mac(mac);

    ESP_LOGI(TERM_TAG, "eth ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR ", mac: " MACSTR,
       IP2STR(&local_ip.ip),
       IP2STR(&local_ip.netmask),
       IP2STR(&local_ip.gw),
       MAC2STR(mac));
}
/***********************************************************************************************************/



//debug
/***********************************************************************************************************/

/***********************************************************************************************************/
