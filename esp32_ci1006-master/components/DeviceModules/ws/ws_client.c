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

#include "device_controller.h"
#include "device_info.h"
#include "DeviceCommon.h"

#include "esp_transport.h"
#include "esp_transport_ws.h"
#include "esp_transport_tcp.h"
#include "ringbuffer.h"
#include "cJSON.h"


#define MULTICAST_STR_EOF 			"\r\n"
#define MULTICAST_BODY_EOF 			"\r\n\r\n"
#define MULTICAST_REQ_HEADER 		"e-audio/1.0 SDP-REQ"MULTICAST_STR_EOF
#define MULTICAST_NOTIFY_HEADER 	"e-audio/1.0 SDP-PUSH"MULTICAST_STR_EOF
#define WEBSOCKET_STR               "Websock-Port:"

#define MCAST_PORT 10800
#define MCAST_ADDR "239.255.255.250"  // 多播地址
//#define MCAST_ADDR "192.168.0.111"  // 多播地址
#define MCAST_DATA MULTICAST_REQ_HEADER  // 多播内容
#define MCAST_INTERVAL 5  //多播时间间隔
#define WS_TIMEOUT     100 //ms


#define WEBS_OPCODE_TEXT    0x01
#define WEBS_OPCODE_BINARY  0x02
#define WEBS_OPCODE_PING    0x09

static const char *TAG = "WS";


static esp_transport_handle_t tcp_handle = NULL, ws_handle = NULL;

static DeviceController *device_control = NULL;
static int run_flag = 1;

static float peak_level;

//websocket
static struct in_addr ws_addr;
static int ws_port;

// buffer
static int BUFFER_SIZE = 10*2048; // 10* 64ms
static int speech_force_start_f = 0;
static int speech_force_end_f = 0;
static int speech_end_f = 0;

static tRingBuffer st_data;

static tRingBuffer* st = &st_data;



static char temp[2048*10];

static int is_wakeup = 0;
static int ws_keep_flag = 0;
static esp_timer_handle_t test_o_handle = 0;

extern void instructions_upload(int value);

int get_wakeup_status(void){
	return is_wakeup;
}

float get_peaklevel(void){
	return peak_level;
}

static int speech_force_end(void)
{
	speech_force_end_f = 1;
	speech_force_start_f = 0;
	return 0;
}

static int speech_end(void)
{
	speech_end_f = 1;
	speech_force_start_f = 0;
	return 0;
}


static void timer_once_cb(void *arg) {
    esp_timer_delete(test_o_handle);
	test_o_handle = 0;
	is_wakeup = 0;
	ws_keep_flag = 0;
	speech_force_end();
	instructions_upload(0);
	ESP_LOGI(TAG, "0000000000\n\n\n\n\n");
}


static esp_timer_create_args_t test_once_arg = { 
        .callback = &timer_once_cb,
        .arg = NULL,
        .name = "WSOnceTimer"
        };

static void timer_start(int timeout_ms) {
	esp_timer_create(&test_once_arg, &test_o_handle);
	esp_timer_start_once(test_o_handle, timeout_ms*1000);
}

static void timer_stop(void) {
	if(test_o_handle) {
		esp_timer_stop(test_o_handle);
	    esp_timer_delete(test_o_handle);
		test_o_handle = 0;
	}
}


static int udp_multicast_loop(void) {

	int sock;
	int len, ret;
	char buff[256];
	char port[16];

	char *start, *end;

	fd_set readset;
	struct timeval tv_out;
	struct sockaddr_in mcast_addr, recv_addr;
	socklen_t socklen = sizeof(recv_addr);

	memset(&mcast_addr,0,sizeof(mcast_addr));
	mcast_addr.sin_family=AF_INET;
	mcast_addr.sin_addr.s_addr=inet_addr(MCAST_ADDR);
	mcast_addr.sin_port=htons(MCAST_PORT);

	sock=socket(AF_INET,SOCK_DGRAM,0);
	if(sock < 0) {
		ESP_LOGE(TAG, "udp_multicast_loop socket error");
		return -1;
	}

    tv_out.tv_sec = MCAST_INTERVAL;
    tv_out.tv_usec = 0;

	sendto(sock, MCAST_DATA, strlen(MCAST_DATA), 0, (struct sockaddr*)&mcast_addr, sizeof(mcast_addr));
	while(run_flag) {
    	FD_ZERO(&readset);
    	FD_SET(sock, &readset);
		ret = select(sock + 1, &readset, NULL, NULL, &tv_out);
		if(ret < 0) {
			ESP_LOGE(TAG, "udp_multicast_loop select error");
			//error
			return -1;
		} else if(ret == 0) {
			len = sendto(sock, MCAST_DATA, strlen(MCAST_DATA), 0, (struct sockaddr*)&mcast_addr, sizeof(mcast_addr));
            printf("%d -- %d [%s:%d]\n", strlen(MCAST_DATA), sizeof(MCAST_DATA), MCAST_ADDR, MCAST_PORT);
			ESP_LOGI(TAG, "send multicast: %s", MCAST_DATA);
			continue;
		}

		len = recvfrom(sock,buff,sizeof(buff)-1,0,(struct sockaddr*)&recv_addr,&socklen);
		if(len < 0) {
			ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
		} else {
			buff[len] = 0;
			ws_addr = recv_addr.sin_addr;
			ESP_LOGI(TAG, "Recv multicast from(%s:%d):%s", inet_ntoa(recv_addr.sin_addr), recv_addr.sin_port, buff);
			//process the data
			start = strstr(buff, WEBSOCKET_STR);
			end = strstr(buff, MULTICAST_BODY_EOF);
			if(strncmp(MULTICAST_NOTIFY_HEADER, buff, strlen(MULTICAST_NOTIFY_HEADER))== 0 && start != NULL && end != NULL) {
				memset(port, 0x00, sizeof(port));
				memcpy(port, start+strlen(WEBSOCKET_STR), end - (start + strlen(WEBSOCKET_STR)));
				ws_port = atoi(port);
				ESP_LOGI(TAG, "ws server: %s, %d", inet_ntoa(ws_addr), ws_port);
				break;
			}
		}
	}
	shutdown(sock, 0);
	close(sock);
	return 0;
}




void buffer_push(char* data, int size, float level) 
{
	peak_level = level;
	ring_buffer_overwrite(st, data, size);
}

static int buffer_get(char*data, int size)
{
	return ring_buffer_read(st, data, size);
}

static int ws_send_text(const char *buff, int len)
{
	int ret = -1;
	if(ws_handle){
		ret = ws_write_with_opcode(ws_handle, buff, len, WS_TIMEOUT, WEBS_OPCODE_TEXT);
	}
	return ret;
}

static int ws_send_binary(const char *buff, int len)
{
	int ret = -1;
	if(ws_handle){
		ret = ws_write_with_opcode(ws_handle, buff, len, WS_TIMEOUT, WEBS_OPCODE_BINARY);
	}
	return ret;
}

static int ws_send_ping(void)
{
	int ret = -1;
	if(ws_handle){
		ret = ws_write_with_opcode(ws_handle, NULL, 0, WS_TIMEOUT, WEBS_OPCODE_PING);
	}
	return ret;
}


//{"cmd":"action","payload":{"type":1,"value":1}}

static void data_process(char *buffer){
	cJSON *jroot = NULL;
	cJSON *tmp;

	jroot = cJSON_Parse(buffer);
    if (jroot != NULL) {
		//cmd
		tmp = cJSON_GetObjectItem(jroot, "cmd");
		if(tmp) {
			if(strcmp(tmp->valuestring, "action") == 0) {
				//ESP_LOGI(TAG, "1111111111\n\n\n\n\n");
				if(is_wakeup) {
					ws_keep_flag = 1;
					ESP_LOGI(TAG, "1111111111\n\n\n\n\n");
					timer_stop();
					timer_start(10*1000);
				}
			}
		}
		cJSON_Delete(jroot);
	}
}

static void ws_client_loop(void) {
	char host_ip[32];
	int host_port = ws_port;
	int ret = 0;
	char buff[1024];
	char id[16];

	memset(id, 0x00, sizeof(id));
	sprintf(host_ip, "%s", inet_ntoa(ws_addr));

	tcp_handle = esp_transport_tcp_init();
	ws_handle = esp_transport_ws_init(tcp_handle);

	//esp_transport_ws_set_path(ws_handle, "/netty3/websocket");

	
	ESP_LOGI(TAG, "ws_client_loop: %s, %d", host_ip, host_port);
	ret = esp_transport_connect(ws_handle, host_ip, host_port, MCAST_INTERVAL*1000);
	if(ret < 0) {
		ESP_LOGE(TAG, "ws_client_loop connect error");
		goto end;
	}
	get_id(id);
	sprintf(buff, "{\"devinfo\":{\"Catalog Code\":2,\"Vendor Code\":3,\"Model Code\":4,\"Series Number\":\"%s\",\"Hardware verion\":\"H001\",\"Firmware Version\":\"V0.0.1\"}}", id);
	ws_send_text(buff, strlen(buff));

	while(run_flag) {
		ret = esp_transport_read(ws_handle, buff, sizeof(buff)-1, MCAST_INTERVAL*1000);
		if(ret > 0) {
			buff[ret] = 0;
			ESP_LOGI(TAG, "ws recv: %s", buff);
			data_process(buff);
		}else if (ret == 0){
			//ESP_LOGI(TAG, "ws recv timeout");
			//ws_send_ping();
		}else if (ret < 0){
			ESP_LOGI(TAG, "ws server close");
			run_flag = 0;
			break;
		}
		
	}
end:
	esp_transport_close(ws_handle);
	esp_transport_destroy(ws_handle);
	ws_handle = NULL;
	ESP_LOGI(TAG, "ws_client_loop exit");
}

static void vad_status_upload(int value)
{
	char buf[128];
	sprintf(buf, "{\"cmd\":\"speech\",\"payload\":{\"vad\":%d,\"level\":%f}}", value, peak_level);
	printf("%s\n", buf);
	ws_send_text(buf, strlen(buf));
}

void instructions_upload(int value)
{
	char buf[128];
	sprintf(buf, "{\"cmd\":\"instructions\",\"payload\":{\"value\":%d,\"level\":%f}}", value, peak_level);
	printf("%s\n", buf);
	ws_send_text(buf, strlen(buf));
}

static void ws_task(void *pv)
{
	static int run = 0;
	run = 1;

	while(run) {

		run_flag = 1;
		udp_multicast_loop();
		ws_client_loop();
	}
	vTaskDelete(NULL);
}

static void speech_task(void *pv)
{
	static int run = 0;
	int size = 0;

	if(!run) {
		ESP_LOGI(TAG,"start speech_task *******************\n");
		run = 1;
		vad_status_upload(0);
		speech_force_end_f = 0;
		speech_end_f = 0;
		while(!speech_force_end_f){
			size = buffer_get(temp, sizeof(temp));
            ESP_LOGI(TAG,"size = %d", size);
			if(size) {
				ws_send_binary(temp, size);
			} else {
				if(speech_end_f) {
					break;
				} else {
					ets_delay_us(20*1000);
				}
			}
            //ets_delay_us(20*1000);
		}
		vad_status_upload(1);
		run = 0;
		ESP_LOGI(TAG,"end speech_task *******************\n");
	}
	vTaskDelete(NULL);
}

static int speech_start(void)
{
	speech_force_end();
	speech_force_start_f = 1;
	xTaskCreatePinnedToCore(&speech_task,
					"speech_task",
					4096,
					NULL,
					WS_TASK_PRIORITY-1,
					NULL,
					WS_TASK_CORE);

	return 0;
	
}


static void ws_client_start(DeviceController *device)
{
	ESP_LOGI(TAG, "ws_clent_start");
	xTaskCreatePinnedToCore(&ws_task,
						"ws_task",
						WS_TASK_STACK_SIZE,
						device,
						WS_TASK_PRIORITY,
						NULL,
						WS_TASK_CORE);
}

static void ws_client_stop(void) {
	run_flag = 0;
}

static void notified_module_ws(DeviceController *device, enum DeviceNotifyType type, void *data, int len)
{
	DeviceNotifyMsg msg = *((DeviceNotifyMsg *) data);

	if(type == DEVICE_NOTIFY_TYPE_WIFI) {
		switch (msg) {
			case DEVICE_NOTIFY_WIFI_GOT_IP:
				ws_client_start(device);
				break;
			case DEVICE_NOTIFY_WIFI_DISCONNECTED:
				ws_client_stop();
				break;
			default:
				break;
		}
	} else if(type == DEVICE_NOTIFY_TYPE_CI1006) {
		switch (msg) {
			case DEVICE_NOTIFY_CI1006_WAKEUP:
				is_wakeup = 1;
				instructions_upload(1);
				break;
			case DEVICE_NOTIFY_CI1006_WAKEUP_END:
				if(!ws_keep_flag) {
					is_wakeup = 0;
					speech_force_end();
					instructions_upload(0);
				}
				break;
			case DEVICE_NOTIFY_CI1006_VAD:
				if(is_wakeup){
					ESP_LOGI(TAG, "DEVICE_NOTIFY_CI1006_VAD");
					speech_start();
				}
				break;
			case DEVICE_NOTIFY_CI1006_VAD_END:
				if(is_wakeup){
					ESP_LOGI(TAG, "DEVICE_NOTIFY_CI1006_VAD_END");
					speech_force_end();
				}
				break;
				
			default:
				break;
		}
	} else if(type == DEVICE_NOTIFY_TYPE_CI1006_INSTRUCTIONS) {
		instructions_upload((int)msg);
	}
}

DeviceModule *create_module_ws(DeviceController *handle)
{
	DeviceModule *module = (DeviceModule *) calloc(1, sizeof(DeviceModule));

	device_control = handle;
	memset(module, 0x00, sizeof(DeviceModule));
	module->active = NULL;
	module->notified = notified_module_ws;
	handle->add_module(handle, module);

	ring_buffer_init(st, BUFFER_SIZE);
	return module;
}
