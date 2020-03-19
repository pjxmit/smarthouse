/* ethernet Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_eth.h"

#include "rom/gpio.h"

#include "tcpip_adapter.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"

#ifdef CONFIG_PHY_LAN8720
#include "eth_phy/phy_lan8720.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config
#endif
#ifdef CONFIG_PHY_TLK110
#include "eth_phy/phy_tlk110.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_tlk110_default_ethernet_config
#endif

#include "device_controller.h"
#include "DeviceCommon.h"
#include "device_info.h"


static const char *TAG = "ETH";

#define PIN_PHY_POWER CONFIG_PHY_POWER_PIN
#define PIN_SMI_MDC CONFIG_PHY_SMI_MDC_PIN
#define PIN_SMI_MDIO CONFIG_PHY_SMI_MDIO_PIN

static DeviceController *device_control = NULL;


static void ReportEthEvent(system_event_t *event)
{
    DeviceNotifyMsg msg = DEVICE_NOTIFY_WIFI_UNDEFINED;
    switch (event->event_id) {
		case SYSTEM_EVENT_ETH_CONNECTED:
			break;
		case SYSTEM_EVENT_ETH_DISCONNECTED:
			msg = DEVICE_NOTIFY_WIFI_DISCONNECTED;
			break;
		case SYSTEM_EVENT_ETH_START:
			break;
		case SYSTEM_EVENT_ETH_GOT_IP:
			msg = DEVICE_NOTIFY_WIFI_GOT_IP;
			break;
		case SYSTEM_EVENT_ETH_STOP:
			msg = DEVICE_NOTIFY_WIFI_DISCONNECTED;
			break;
		default:
			break;

	}

    if (msg != DEVICE_NOTIFY_WIFI_UNDEFINED) {
		if(device_control->notify_modules)
			device_control->notify_modules(device_control, DEVICE_NOTIFY_TYPE_WIFI, &msg, sizeof(DeviceNotifyMsg));
    }
}


#ifdef CONFIG_PHY_USE_POWER_PIN
/**
 * @brief re-define power enable func for phy
 *
 * @param enable true to enable, false to disable
 *
 * @note This function replaces the default PHY power on/off function.
 * If this GPIO is not connected on your device (and PHY is always powered),
 * you can use the default PHY-specific power on/off function.
 */
static void phy_device_power_enable_via_gpio(bool enable)
{
    assert(DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable);

    if (!enable) {
        DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable(false);
    }

    gpio_pad_select_gpio(PIN_PHY_POWER);
    gpio_set_direction(PIN_PHY_POWER, GPIO_MODE_OUTPUT);
    if (enable == true) {
        gpio_set_level(PIN_PHY_POWER, 1);
        ESP_LOGI(TAG, "Power On Ethernet PHY");
    } else {
        gpio_set_level(PIN_PHY_POWER, 0);
        ESP_LOGI(TAG, "Power Off Ethernet PHY");
    }

    vTaskDelay(1); // Allow the power up/down to take effect, min 300us

    if (enable) {
        /* call the default PHY-specific power on function */
        DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable(true);
    }
}
#endif

/**
 * @brief gpio specific init
 *
 * @note RMII data pins are fixed in esp32:
 * TXD0 <=> GPIO19
 * TXD1 <=> GPIO22
 * TX_EN <=> GPIO21
 * RXD0 <=> GPIO25
 * RXD1 <=> GPIO26
 * CLK <=> GPIO0
 *
 */
static void eth_gpio_config_rmii(void)
{
    phy_rmii_configure_data_interface_pins();
    phy_rmii_smi_configure_pins(PIN_SMI_MDC, PIN_SMI_MDIO);
}

/**
 * @brief event handler for ethernet
 *
 * @param ctx
 * @param event
 * @return esp_err_t
 */
static esp_err_t eth_event_handler(void *ctx, system_event_t *event)
{
    tcpip_adapter_ip_info_t ip;

    switch (event->event_id) {
    case SYSTEM_EVENT_ETH_CONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Up");
        break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case SYSTEM_EVENT_ETH_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case SYSTEM_EVENT_ETH_GOT_IP:
        memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
        ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip));
        ESP_LOGI(TAG, "Ethernet Got IP Addr");
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip.ip));
        ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip.netmask));
        ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip.gw));
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        break;
    case SYSTEM_EVENT_ETH_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
	ReportEthEvent(event);
    return ESP_OK;
}

static void active_module_ethernet(DeviceController *device)
{
	ESP_LOGI(TAG, "active");
	device_control = device;
	tcpip_adapter_init();

	int  dhcp;
	char ip[16] = {0};
	char gw[16] = {0};
	char mask[16] = {0};
	read_eth_para(ip, gw, mask, &dhcp);

	ESP_LOGI(TAG, "dhcp = %d", dhcp);

	if(!dhcp){
		ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH));
	    /* static ip settings */
	    tcpip_adapter_ip_info_t sta_ip;
	    sta_ip.ip.addr = ipaddr_addr(ip);
	    sta_ip.gw.addr = ipaddr_addr(gw);
	    sta_ip.netmask.addr = ipaddr_addr(mask);
		ESP_LOGI(TAG, "ip: %s, gw: %s, mask: %s", ip, gw, mask);
	    tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &sta_ip);
	}

    ESP_ERROR_CHECK(esp_event_loop_init(eth_event_handler, NULL));

    eth_config_t config = DEFAULT_ETHERNET_PHY_CONFIG;
    config.phy_addr = CONFIG_PHY_ADDRESS;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;
    config.clock_mode = CONFIG_PHY_CLOCK_MODE;
#ifdef CONFIG_PHY_USE_POWER_PIN
    /* Replace the default 'power enable' function with an example-specific one
     that toggles a power GPIO. */
    config.phy_power_enable = phy_device_power_enable_via_gpio;
#endif
	//esp_eth_init(&config);
	//esp_eth_enable();

    ESP_ERROR_CHECK(esp_eth_init(&config));
    ESP_ERROR_CHECK(esp_eth_enable()) ;
}

DeviceModule *create_module_ethernet(DeviceController *handle)
{
	DeviceModule *module = (DeviceModule *) calloc(1, sizeof(DeviceModule));

	memset(module, 0x00, sizeof(DeviceModule));
	module->active = active_module_ethernet;
	module->notified = NULL;

	handle->add_module(handle, module);
	return module;
}

