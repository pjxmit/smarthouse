#ifndef _DEVICE_CONTROLLER_H_
#define _DEVICE_CONTROLLER_H_

#define OTA_TYPE_UART       0x01
#define OTA_TYPE_TCP_SERVER 0x02
#define OTA_TYPE_TCP_CLIENT 0x03

enum DeviceNotifyType;

typedef struct DeviceModule DeviceModule;
typedef struct DeviceController DeviceController;

struct DeviceModule {
	DeviceModule *next;

	void (*active)(DeviceController *device);
    void (*notified)(DeviceController *device, enum DeviceNotifyType type, void *data, int len);
};

struct DeviceController {
	DeviceModule *modules;

	void (*add_module)(DeviceController *device, DeviceModule *module);
	void (*active_all_modules)(DeviceController *device);
	void (*notify_modules)(DeviceController *device, enum DeviceNotifyType type, void *data, int len);
	void (*smartConfig)(void);
	/*
	// otaProcess: 
	// type :  0x01 ---use uart, data are uart port and firmware len(4 bytes + 4 bytes)
	//           0x02 --- as a tcp server, data is tcp server port
	//           0x03 --- pull date from a http server, not support now
	// return :0 --- successful, other failed
	*/
	int (*otaProcess)(char type, void *data, int len);
	void *private; // for user to extend
};

int device_controller_init(DeviceController **handle);
#endif //_DEVICE_CONTROLLER_H_