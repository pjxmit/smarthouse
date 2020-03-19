#ifndef _DEVICE_INFO_H_
#define _DEVICE_INFO_H_

#ifdef __cplusplus
extern "C" {
#endif

void get_id(char *id);

esp_err_t read_eth_para(char *ip, char *gw, char *mask, int *dhcp);
esp_err_t write_eth_para(char *ip, char *gw, char *mask, int dhcp);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // _DEVICE_INFO_H_