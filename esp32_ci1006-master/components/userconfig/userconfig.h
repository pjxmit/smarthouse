#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

// PIN CONFIG
/*************************************************************************************/
/* LED related */
#define LED_INDICATOR_NET    GPIO_NUM_22
#define GPIO_SEL_LED         (1<<LED_INDICATOR_NET)

/*GPIO CONTROL*/
#define GPIO_SWITCH_0        GPIO_NUM_4
#define GPIO_SWITCH_SEL_0    (1<<GPIO_SWITCH_0)

/*Touch Key*/
#define TOUCH_1               TOUCH_PAD_NUM2 // GPIO_2
#define TOUCH_2               TOUCH_PAD_NUM5 // GPIO_12

#ifdef CONFIG_ETH_MODE
/*UART_2*/
#define UART_2_TX            GPIO_NUM_17
#define UART_2_RX            GPIO_NUM_16

/* VAD related */
#define CPIO_VAD_VALUE    	 GPIO_NUM_5
#define GPIO_VAD_SEL         (1<<CPIO_VAD_VALUE)


//I2S_0 PIN CONFIG
#define I2S_0_PIN_DATA_OUT -1
#define I2S_0_PIN_DATA_IN  GPIO_NUM_34
#define I2S_0_PIN_BCK      GPIO_NUM_4
#define I2S_0_PIN_WS       GPIO_NUM_2
#else
/*UART_2*/
#define UART_2_TX            GPIO_NUM_4
#define UART_2_RX            GPIO_NUM_5

/* VAD related */
#define CPIO_VAD_VALUE    	 GPIO_NUM_19
#define GPIO_VAD_SEL         (1<<CPIO_VAD_VALUE)


//I2S_0 PIN CONFIG
#define I2S_0_PIN_DATA_OUT -1
#define I2S_0_PIN_DATA_IN  GPIO_NUM_35
#define I2S_0_PIN_BCK      GPIO_NUM_25
#define I2S_0_PIN_WS       GPIO_NUM_26
#endif

/*************************************************************************************/


//TASK CONFIG
/*************************************************************************************/
//ota
#define WIFI_OTA_TASK_PRIORITY                    15
#define WIFI_OTA_TASK_STACK_SIZE                  1024*8
#define WIFI_OTA_TASK_CORE                        0

//ws
#define WS_TASK_PRIORITY                          16
#define WS_TASK_STACK_SIZE                        1024*8
#define WS_TASK_CORE                              0

//peripheral
#define MODULE_PERIPHERAL_TASK_PRIORITY           11
#define MODULE_PERIPHERAL_TASK_STACK_SIZE         2048
#define MODULE_PERIPHERAL_TASK_CORE               1

//uart, ci1006
#define MODULE_UART_TASK_PRIORITY                 15
#define MODULE_UART_TASK_STACK_SIZE               2048
#define MODULE_UART_TASK_CORE                     1


//key
#define MODULE_KEY_TASK_PRIORITY                  5
#define MODULE_KEY_TASK_STACK_SIZE                4096
#define MODULE_KEY_TASK_CORE                      0

//vad
#define MODULE_VAD_TASK_PRIORITY                  14
#define MODULE_VAD_TASK_STACK_SIZE                4096
#define MODULE_VAD_TASK_CORE                      1

//shell
#define MODULE_SHELL_TASK_PRIORITY                3
#define MODULE_SHELL_TASK_STACK_SIZE              3072
#define MODULE_SHELL_TASK_CORE                    0


//wifi and smart config
#define WIFI_SMARTCONFIG_TASK_PRIORITY            14
#define WIFI_SMARTCONFIG_TASK_STACK_SIZE          3072
#define WIFI_SMARTCONFIG_TASK_CORE                0

// i2s
#define MODULE_I2S_TASK_PRIORITY                  13
#define MODULE_I2S_TASK_STACK_SIZE                4096
#define MODULE_I2S_TASK_CORE                      1
/*************************************************************************************/
#endif /* __USER_CONFIG_H__ */
