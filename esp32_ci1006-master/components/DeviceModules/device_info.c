#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "sdkconfig.h"


#include "esp_wifi.h"
#include "esp_eth.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "device_info.h"
#include "userconfig.h"

#define INFO_TAG          "DEVICE_INFO"

#define DEFAULT_PART_NAME "sonapara"
#define PATA_ETH          "paraeth"

#define BUFFER_LEN 15

typedef struct _eth_para{
	int  dhcp;
	char ip[BUFFER_LEN+1];
	char gw[BUFFER_LEN+1];
	char mask[BUFFER_LEN+1];
}eth_para;


esp_err_t read_eth_para(char *ip, char *gw, char *mask, int *dhcp)
{
	esp_err_t err;
	nvs_handle my_handle;
	size_t tmp_len = sizeof(eth_para);

	eth_para tmp;

	if(ip == NULL || gw == NULL || mask == NULL)
		return ESP_FAIL;

	memset(&tmp, 0x00, tmp_len);

	err = nvs_open(DEFAULT_PART_NAME, NVS_READWRITE, &my_handle);
	if(err == ESP_OK) {
		err = nvs_get_blob(my_handle, PATA_ETH, &tmp, &tmp_len);
		switch (err) {
            case ESP_OK:
				ESP_LOGI(INFO_TAG, "read_PATA_ETH_nvs(ok)");
				*dhcp = tmp.dhcp;
				snprintf(ip, BUFFER_LEN, "%s", tmp.ip);
				snprintf(gw, BUFFER_LEN, "%s", tmp.gw);
				snprintf(mask, BUFFER_LEN, "%s", tmp.mask);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
				tmp_len = sizeof(eth_para);
			#ifdef CONFIG_PHY_DISABLE_DHCP
				tmp.dhcp = 0;
				snprintf(tmp.ip, BUFFER_LEN, "%s", CONFIG_PHY_IP);
				snprintf(tmp.gw, BUFFER_LEN, "%s", CONFIG_PHY_GW);
				snprintf(tmp.mask, BUFFER_LEN, "%s", CONFIG_PHY_MASK);

				*dhcp = 0;
				snprintf(ip, BUFFER_LEN, "%s", CONFIG_PHY_IP);
				snprintf(gw, BUFFER_LEN, "%s", CONFIG_PHY_GW);
				snprintf(mask, BUFFER_LEN, "%s", CONFIG_PHY_MASK);
			#else
				tmp.dhcp = 1;
			#endif
				nvs_set_blob(my_handle, PATA_ETH, &tmp, tmp_len);
				err = nvs_commit(my_handle);
				ESP_LOGI(INFO_TAG, "read_PATA_ETH_nvs(first)");
                break;
            default :
                printf("Error (%d) reading!\n", err);
		}
		nvs_close(my_handle);
	}
	return err;
}

esp_err_t write_eth_para(char *ip, char *gw, char *mask, int dhcp)
{
	esp_err_t err;
	nvs_handle my_handle;
	size_t tmp_len = sizeof(eth_para);
	eth_para tmp;

	if((dhcp == 0) && (ip == NULL || gw == NULL || mask == NULL))
		return ESP_FAIL;

	memset(&tmp, 0x00, tmp_len);

	tmp.dhcp = dhcp;
	if(!dhcp) {
		snprintf(tmp.ip, BUFFER_LEN, "%s", ip);
		snprintf(tmp.gw, BUFFER_LEN, "%s", gw);
		snprintf(tmp.mask, BUFFER_LEN, "%s", mask);
	}

	err = nvs_open(DEFAULT_PART_NAME, NVS_READWRITE, &my_handle);
	if(err == ESP_OK) {
		nvs_set_blob(my_handle, PATA_ETH, &tmp, tmp_len);
		nvs_commit(my_handle);
		nvs_close(my_handle);
	}
	return err;
}



void get_id(char *id)
{
	uint8_t mac[6];

#ifdef CONFIG_ETH_MODE
	esp_eth_get_mac(mac);
#else
	esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
#endif
	sprintf(id, "%02x%02x%02x%02x%02x%02x", MAC2STR(mac));
	//sprintf(id, MACSTR, MAC2STR(mac));
	ESP_LOGI(INFO_TAG, "device_id: %s", id);
}
