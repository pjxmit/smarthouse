#ifndef _TERMINAL_CTRL_H_
#define _TERMINAL_CTRL_H_
//SYSTEM
void cmd_task(void *ref, int argc, char *argv[]);
void cmd_task_list(void *ref, int argc, char *argv[]);
void cmd_task_list_stop(void *ref, int argc, char *argv[]);
void cmd_reboot(void *ref, int argc, char *argv[]);
//WIFI
void cmd_smartconf(void *ref, int argc, char *argv[]);
void cmd_wifiInfo(void *ref, int argc, char *argv[]);
void cmd_wifiSet(void *ref, int argc, char *argv[]);

//ETH
void cmd_ethSet(void *ref, int argc, char *argv[]);

//IP
void cmd_ip_wlan(void *ref, int argc, char *argv[]);
void cmd_ip_eth(void *ref, int argc, char *argv[]);



#endif
