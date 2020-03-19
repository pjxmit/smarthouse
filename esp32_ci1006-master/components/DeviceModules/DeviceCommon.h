#ifndef __DEVICE_COMMON_H__
#define __DEVICE_COMMON_H__

typedef enum DeviceNotifyType {
    DEVICE_NOTIFY_TYPE_KEY,
    DEVICE_NOTIFY_TYPE_WIFI,
    DEVICE_NOTIFY_TYPE_IOT,
    DEVICE_NOTIFY_TYPE_SYS,
    DEVICE_NOTIFY_TYPE_CI1006,
    DEVICE_NOTIFY_TYPE_CI1006_INSTRUCTIONS,
} DeviceNotifyType;

typedef int DeviceNotifyMsg;

typedef enum DeviceNotifyTouchMsg {
    DEVICE_NOTIFY_KEY_UNDEFINED,
	DEVICE_NOTIFY_KEY_TP_1_S,
    DEVICE_NOTIFY_KEY_TP_1_L,
    DEVICE_NOTIFY_KEY_TP_1_R,
    DEVICE_NOTIFY_KEY_TP_2_S,
    DEVICE_NOTIFY_KEY_TP_2_L,
    DEVICE_NOTIFY_KEY_TP_2_R,
} DeviceNotifyTouchMsg;

typedef enum DeviceNotifyWifiMsg {
    DEVICE_NOTIFY_WIFI_UNDEFINED,
    DEVICE_NOTIFY_WIFI_GOT_IP,
    DEVICE_NOTIFY_WIFI_DISCONNECTED,
    DEVICE_NOTIFY_WIFI_SETTING_TIMEOUT,
    DEVICE_NOTIFY_WIFI_SC_DISCONNECTED,//smart config disconnect
    DEVICE_NOTIFY_WIFI_BLE_DISCONNECTED,// ble config disconnect
} DeviceNotifyWifiMsg;

typedef enum DeviceNotifySysMsg {
    DEVICE_NOTIFY_SYS_UNDEFINED,
    DEVICE_NOTIFY_SYS_OTA,
    DEVICE_NOTIFY_SYS_SMART_CONF_END,
} DeviceNotifySysMsg;

typedef enum DeviceNotifyCi1006Msg {
    DEVICE_NOTIFY_CI1006_UNDEFINED,
    DEVICE_NOTIFY_CI1006_WAKEUP,
    DEVICE_NOTIFY_CI1006_WAKEUP_END,
    DEVICE_NOTIFY_CI1006_VAD,
    DEVICE_NOTIFY_CI1006_VAD_END,
} DeviceNotifyCi1006Msg;


//
typedef struct DeviceNotification {
    void *receiver;
    DeviceNotifyType type;
    void *data;
    int len;
} DeviceNotification;

#endif
