#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_err.h"
#include "esp_system.h"
#include "sdkconfig.h"

#include "device_controller.h"
#include "DeviceCommon.h"
#include "wifismartconfig.h"
#include "wifibleconfig.h"
#include "led.h"
#include "userconfig.h"

#define WIFICONFIG_TAG                              "WIFI_MODULE"
#define WIFI_EVT_QUEUE_LEN                          4
#define SMARTCONFIG_TIMEOUT_TICK                    (90000 / portTICK_RATE_MS)
#define WIFI_SSID                                   CONFIG_ESP_AUDIO_WIFI_SSID
#define WIFI_PASS                                   CONFIG_ESP_AUDIO_WIFI_PWD
#define WIFI_SMART_CONFIG_TYPE                      SC_TYPE_ESPTOUCH_AIRKISS


#if USE_WIFI_MANAGER_TASK
static QueueHandle_t xQueueWifi;
#endif

static TaskHandle_t SmartConfigHandle;
extern EventGroupHandle_t g_sc_event_group;

typedef enum {
    SMARTCONFIG_NO,
    SMARTCONFIG_YES
} SmartconfigStatus;

typedef enum {
    WifiState_Unknow,
    WifiState_Config_Timeout,
    WifiState_Connecting,
    WifiState_Connected,
    WifiState_Disconnected,
    WifiState_ConnectFailed,
    WifiState_GotIp,
    WifiState_SC_Disconnected,//smart config Disconnected
    WifiState_BLE_Disconnected,
} WifiState;


static wifi_config_t sta_config = {
    .sta = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
        .bssid_set = false
    }
};

static DeviceController *device_control = NULL;

static void ReportWifiEvent(void *evt)
{
    WifiState state = *((WifiState *) evt);
    DeviceNotifyMsg msg = DEVICE_NOTIFY_WIFI_UNDEFINED;
    switch (state) {
    case WifiState_GotIp:
        msg = DEVICE_NOTIFY_WIFI_GOT_IP;
        break;
    case WifiState_Disconnected:
        msg = DEVICE_NOTIFY_WIFI_DISCONNECTED;
        break;
    case WifiState_SC_Disconnected:
        msg = DEVICE_NOTIFY_WIFI_SC_DISCONNECTED;
        break;
    case WifiState_BLE_Disconnected:
        msg = DEVICE_NOTIFY_WIFI_BLE_DISCONNECTED;
        break;
    case WifiState_Config_Timeout:
        msg = DEVICE_NOTIFY_WIFI_SETTING_TIMEOUT;
        break;
    case WifiState_Connecting:
    case WifiState_Connected:
    case WifiState_ConnectFailed:
    default:
        break;
    }

    if (msg != DEVICE_NOTIFY_WIFI_UNDEFINED) {
		if(device_control->notify_modules)
			device_control->notify_modules(device_control, DEVICE_NOTIFY_TYPE_WIFI, &msg, sizeof(DeviceNotifyMsg));
    }
}

static esp_err_t wifiEvtHandlerCb(void* ctx, system_event_t* evt)
{
    WifiState g_WifiState;
    static char lastState = -1;

    if (evt == NULL) {
        return ESP_FAIL;
    }

    switch (evt->event_id) {
        case SYSTEM_EVENT_WIFI_READY:
            ESP_LOGI(WIFICONFIG_TAG, "+WIFI:READY");
            break;

        case SYSTEM_EVENT_SCAN_DONE:
            ESP_LOGI(WIFICONFIG_TAG, "+SCANDONE");
            break;

        case SYSTEM_EVENT_STA_START:
            g_WifiState = WifiState_Connecting;
            ReportWifiEvent(&g_WifiState);
            ESP_LOGI(WIFICONFIG_TAG, "+WIFI:STA_START");
			LedIndicatorSet(LedIndexNet, LedWorkState_NetDisconnect);
            break;

        case SYSTEM_EVENT_STA_STOP:

            ESP_LOGI(WIFICONFIG_TAG, "+WIFI:STA_STOP");
            break;

        case SYSTEM_EVENT_STA_CONNECTED:
            g_WifiState = WifiState_Connected;
			ReportWifiEvent(&g_WifiState);
            ESP_LOGI(WIFICONFIG_TAG, "+JAP:WIFICONNECTED");
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(WIFICONFIG_TAG, "+JAP:DISCONNECTED,%u,%u", 0, evt->event_info.disconnected.reason);
			EventBits_t uxBits = xEventGroupWaitBits(g_sc_event_group, SC_PAIRING, true, false, 0);
			if (!(uxBits & SC_PAIRING))// WiFi connected event
			{
				wifi_config_t w_config;

				if (lastState != SYSTEM_EVENT_STA_DISCONNECTED) {
					g_WifiState = WifiState_Disconnected;
					ESP_LOGI(WIFICONFIG_TAG, "disconnect!");
					LedIndicatorSet(LedIndexNet, LedWorkState_NetDisconnect);
					ReportWifiEvent(&g_WifiState);
                }

				if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &w_config)) {
			        if(w_config.sta.ssid[0] != 0){
			            //if set before
			            ESP_LOGI(WIFICONFIG_TAG, "Autoconnect to Wi-Fi SSID:%s", w_config.sta.ssid);
			            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &w_config));
			        }else{
						ESP_LOGI(WIFICONFIG_TAG, "Autoconnect to default Wi-Fi ssid:%s,pwd:%s.", WIFI_SSID, WIFI_PASS);
			            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
			        }
			    }
                //esp_wifi_set_config(WIFI_IF_STA, &sta_config);
                if (ESP_FAIL == esp_wifi_connect()) {
                    ESP_LOGE(WIFICONFIG_TAG, "Autoconnect Wi-Fi failed");
                }
            } else {
                if (lastState != SYSTEM_EVENT_STA_DISCONNECTED) {
                    g_WifiState = WifiState_SC_Disconnected;
					ESP_LOGI(WIFICONFIG_TAG, "smart config!");
					LedIndicatorSet(LedIndexNet, LedWorkState_NetSetting);
					ReportWifiEvent(&g_WifiState);
                }

                ESP_LOGI(WIFICONFIG_TAG, "don't Autoconnect due to smart_config or ble config!");
            }

            lastState = SYSTEM_EVENT_STA_DISCONNECTED;
            break;

        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
            ESP_LOGI(WIFICONFIG_TAG, "+JAP:AUTHCHANGED,%d,%d",
                     evt->event_info.auth_change.new_mode,
                     evt->event_info.auth_change.old_mode);
            break;

        case SYSTEM_EVENT_STA_GOT_IP: {
            if (lastState != SYSTEM_EVENT_STA_GOT_IP) {
                g_WifiState = WifiState_GotIp;
				ReportWifiEvent(&g_WifiState);
                LedIndicatorSet(LedIndexNet, LedWorkState_NetConnectOk);
				ESP_LOGI(WIFICONFIG_TAG, "Freemem Size: %dKB,%dKB", esp_get_free_heap_size()/1024, heap_caps_get_free_size(MALLOC_CAP_8BIT|MALLOC_CAP_INTERNAL)/1024);
            }

            wifi_config_t w_config;
            memset(&w_config, 0x00, sizeof(wifi_config_t));

            if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &w_config)) {
                ESP_LOGI(WIFICONFIG_TAG, ">>>>>> Connected Wifi SSID:%s <<<<<<< \r\n", w_config.sta.ssid);
            } else {
                ESP_LOGE(WIFICONFIG_TAG, "Got wifi config failed");
            }

            ESP_LOGI(WIFICONFIG_TAG, "SYSTEM_EVENT_STA_GOTIP");
            lastState = SYSTEM_EVENT_STA_GOT_IP;
            break;
        }

        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
            ESP_LOGI(WIFICONFIG_TAG, "+WPS:SUCCEED");
            // esp_wifi_wps_disable();
            //esp_wifi_connect();
            break;

        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
            ESP_LOGI(WIFICONFIG_TAG, "+WPS:FAILED");
            break;

        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
            ESP_LOGI(WIFICONFIG_TAG, "+WPS:TIMEOUT");
            // esp_wifi_wps_disable();
            //esp_wifi_disconnect();
            break;

        case SYSTEM_EVENT_STA_WPS_ER_PIN: {
//                char pin[9] = {0};
//                memcpy(pin, event->event_info.sta_er_pin.pin_code, 8);
//                ESP_LOGI(APP_TAG, "+WPS:PIN: [%s]", pin);
            break;
        }

        case SYSTEM_EVENT_AP_START:
            ESP_LOGI(WIFICONFIG_TAG, "+WIFI:AP_START");
            break;

        case SYSTEM_EVENT_AP_STOP:
            ESP_LOGI(WIFICONFIG_TAG, "+WIFI:AP_STOP");
            break;

        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(WIFICONFIG_TAG, "+SOFTAP:STACONNECTED,"MACSTR,
                     MAC2STR(evt->event_info.sta_connected.mac));
            break;

        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(WIFICONFIG_TAG, "+SOFTAP:STADISCONNECTED,"MACSTR,
                     MAC2STR(evt->event_info.sta_disconnected.mac));
            break;

        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            ESP_LOGI(WIFICONFIG_TAG, "+PROBEREQ:"MACSTR",%d",MAC2STR(evt->event_info.ap_probereqrecved.mac),
                evt->event_info.ap_probereqrecved.rssi);
        break;
    default:
        break;
    }
    return ESP_OK;
}

//@warning: must be called by app_main task
static void WifiStartUp(void)
{
	wifi_config_t w_config;
    memset(&w_config, 0x00, sizeof(wifi_config_t));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    //TODO: used memorized SSID and PASSWORD
    ESP_ERROR_CHECK(esp_event_loop_init(wifiEvtHandlerCb, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &w_config)) {
        if(w_config.sta.ssid[0] != 0){
            //if set before
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &w_config));
        }else{
            ESP_LOGI(WIFICONFIG_TAG, "Connect to default Wi-Fi SSID:%s", sta_config.sta.ssid);
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
        }
    } else {
        ESP_LOGE(WIFICONFIG_TAG, "get wifi config failed!");
        return;
    }
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

	ESP_LOGI(WIFICONFIG_TAG, "SC Setup");
	SmartconfigSetup(WIFI_SMART_CONFIG_TYPE, true);

}

static void smart_event_upload(void)
{
	DeviceNotifyMsg msg = DEVICE_NOTIFY_SYS_SMART_CONF_END;

	if(device_control->notify_modules)
        device_control->notify_modules(device_control, DEVICE_NOTIFY_TYPE_SYS, &msg, sizeof(DeviceNotifyMsg));

}

/******************************************************************************
 * FunctionName : smartconfig_task
 * Description  : start the samrtconfig proces and call back
 * Parameters   : pvParameters
 * Returns      : none
*******************************************************************************/
static void WifiConfigTask(void *pvParameters)
{
	ESP_LOGI(WIFICONFIG_TAG, "WifiConfig task running");
    xEventGroupSetBits(g_sc_event_group, SC_PAIRING);
    esp_wifi_disconnect();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    wifi_mode_t curMode = WIFI_MODE_NULL;
    //WifiState g_WifiState;
    if ((ESP_OK == esp_wifi_get_mode(&curMode)) && (curMode == WIFI_MODE_STA)) {
        ESP_LOGD(WIFICONFIG_TAG, "Smartconfig start,heap=%d\n", esp_get_free_heap_size());
		smart_event_upload();
		LedIndicatorSet(LedIndexNet, LedWorkState_NetSetting);
        esp_err_t res = WifiSmartConnect((TickType_t) SMARTCONFIG_TIMEOUT_TICK);
        //LedIndicatorSet(LedIndexNet, LedWorkState_NetSetting);
        switch (res) {
        case ESP_ERR_INVALID_STATE:
            ESP_LOGE(WIFICONFIG_TAG, "Already in smartconfig, please wait");
            break;
        case ESP_ERR_TIMEOUT:
            //g_WifiState = WifiState_Config_Timeout;
		#if USE_WIFI_MANAGER_TASK
			//xQueueSend(xQueueWifi, &g_WifiState, 0);
		#else
			//ReportWifiEvent(&g_WifiState);
		#endif
            ESP_LOGI(WIFICONFIG_TAG, "Smartconfig timeout");
			esp_wifi_start();
			esp_wifi_connect();
            break;
        case ESP_FAIL:
            ESP_LOGI(WIFICONFIG_TAG, "Smartconfig failed, please try again");
            break;
        default:
            break;
        }
		if(res != ESP_ERR_INVALID_STATE)
			esp_restart();
    } else {
        ESP_LOGE(WIFICONFIG_TAG, "Invalid wifi mode");
    }
    vTaskDelete(NULL);
}

//public interface
static void WifiSmartConfig(void)
{
	//configASSERT(manager);
    if(xTaskCreatePinnedToCore(WifiConfigTask,
								"WifiConfigTask",
								WIFI_SMARTCONFIG_TASK_STACK_SIZE,
								NULL,
								WIFI_SMARTCONFIG_TASK_PRIORITY,
								&SmartConfigHandle, WIFI_SMARTCONFIG_TASK_CORE) != pdPASS) {
        ESP_LOGE(WIFICONFIG_TAG, "ERROR creating WifiConfigTask task! Out of memory?");
    }
}

static void active_module_wifi(DeviceController *device)
{
	ESP_LOGI(WIFICONFIG_TAG, "active");
	device_control = device;
	tcpip_adapter_init();
	WifiStartUp();
}

DeviceModule *create_module_wifi(DeviceController *handle)
{
	DeviceModule *module = (DeviceModule *) calloc(1, sizeof(DeviceModule));

	memset(module, 0x00, sizeof(DeviceModule));
	module->active = active_module_wifi;
	module->notified = NULL;

	handle->smartConfig = WifiSmartConfig;
	handle->add_module(handle, module);
	return module;
}

