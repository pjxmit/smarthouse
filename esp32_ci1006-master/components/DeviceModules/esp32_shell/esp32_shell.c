#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"


#include "driver/uart.h"

#include "userconfig.h"
#include "cmd_func.h"
#include "device_controller.h"
#include "DeviceCommon.h"


#define uart_num UART_NUM_0

#define _shell_clr_line(stream)   chprintf(stream, "\033[K")

#define SHELL_NEWLINE_STR   "\r\n"
#define SHELL_MAX_ARGUMENTS 10
#define SHELL_PROMPT_STR "\r\n>"
#define SHELL_MAX_LINE_LENGTH 512

#define SHELL_TAG "SHELL_MODULE"

typedef void (*shellcmd_tt)(void *self, int argc, char *argv[]);

/**
 * @brief   Custom command entry type.
 */
typedef struct {
    const char      *sc_name;           /**< @brief Command name.       */
    shellcmd_tt      sc_function;        /**< @brief Command function.   */
} ESPShellCommand;


static ESPShellCommand *scp = NULL;
static int shell_run = 0;

static char *parse_arguments(char *str, char **saveptr) {
    char *p;

    if (str != NULL)
        *saveptr = str;

    p = *saveptr;
    if (!p) {
        return NULL;
    }

    /* Skipping white space.*/
    p += strspn(p, " \t");

    if (*p == '"') {
        /* If an argument starts with a double quote then its delimiter is another
           quote.*/
        p++;
        *saveptr = strpbrk(p, "\"");
    }
    else {
        /* The delimiter is white space.*/
        *saveptr = strpbrk(p, " \t");
    }

    /* Replacing the delimiter with a zero.*/
    if (*saveptr != NULL) {
        *(*saveptr)++ = '\0';
    }

    return *p != '\0' ? p : NULL;
}



static void list_commands(ESPShellCommand *scp) {
    ESPShellCommand *_scp = scp;
    while (_scp->sc_name != NULL) {
        printf("%s \r\n", _scp->sc_name);
        _scp++;
    }
}


static bool cmdexec(void *ref, char *name, int argc, char *argv[]) {
    ESPShellCommand *_scp = scp;
    while (_scp->sc_name != NULL) {
        if (strcmp(_scp->sc_name, name) == 0) {
            _scp->sc_function(ref, argc, argv);
            return false;
        }
        _scp++;
    }
    return true;
}

static bool shellGetLine(char *line, unsigned size) {
    char *p = line;
    char c;
    char tx[3];
    while (true) {
        uart_read_bytes(uart_num, (uint8_t*)&c, 1, portMAX_DELAY);

        if ((c == 8) || (c == 127)) {  // backspace or del
            if (p != line) {
                tx[0] = c;
                tx[1] = 0x20;
                tx[2] = c;
                uart_write_bytes(uart_num, (const char*)tx, 3);
                p--;
            }
            continue;
        }
        if (c == '\n' || c == '\r') {
            tx[0] = c;
            uart_write_bytes(uart_num, (const char*)tx, 1);
            *p = 0;
            return false;
        }

        if (c < 0x20)
            continue;
        if (p < line + size - 1) {
            tx[0] = c;
            uart_write_bytes(uart_num, (const char*)tx, 1);
            *p++ = (char)c;
        }
    }
}


static void shell_task(void *pv)
{
    int n;

    char *lp, *cmd, *tokp, line[SHELL_MAX_LINE_LENGTH];
    char *args[SHELL_MAX_ARGUMENTS + 1];


    while (shell_run) {
        printf(">");

        if (shellGetLine(line, sizeof(line))) {

        }

        lp = parse_arguments(line, &tokp);
        cmd = lp;
        n = 0;
        while ((lp = parse_arguments(NULL, &tokp)) != NULL) {
            if (n >= SHELL_MAX_ARGUMENTS) {
                printf("too many arguments"SHELL_NEWLINE_STR);
                cmd = NULL;
                break;
            }
            args[n++] = lp;
        }
        args[n] = NULL;
        if (cmd != NULL) {
            if (cmdexec(pv, cmd, n, args)) {
                printf("%s", cmd);
                printf("?"SHELL_NEWLINE_STR);
                list_commands(scp);
            }
        }

    }
    vTaskDelete(NULL);
}


static ESPShellCommand command[] = {

	{"---------system------", NULL},
	{"task", cmd_task},
	{"top",  cmd_task_list},
	{"stop", cmd_task_list_stop},
	{"reboot", cmd_reboot},

	{"---------wifi------", NULL},
	{"wifiinfo", cmd_wifiInfo},
	{"wifiset", cmd_wifiSet},
	{"smart", cmd_smartconf},

	{"---------eth------", NULL},
	{"ethset", cmd_ethSet},

	{"---------ip------", NULL},
	{"ipwlan", cmd_ip_wlan},
	{"ipeth", cmd_ip_eth},
    {NULL, NULL}
};

static void active_module_shell(DeviceController *device)
{
	ESP_LOGI(SHELL_TAG, "active");

	if(shell_run)
		return;
	shell_run = 1;
    scp = command;

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    uart_param_config(uart_num, &uart_config);
    uart_driver_install(uart_num,  256, 0, 0, NULL, 0);

	xTaskCreatePinnedToCore(&shell_task, 
							"shell_task", 
							MODULE_SHELL_TASK_STACK_SIZE, 
							device, 
							MODULE_SHELL_TASK_PRIORITY, 
							NULL, 
							MODULE_SHELL_TASK_CORE);
}

DeviceModule *create_module_shell(DeviceController *handle)
{
	DeviceModule *module = (DeviceModule *) calloc(1, sizeof(DeviceModule));

	memset(module, 0x00, sizeof(DeviceModule));
	module->active = active_module_shell;
	module->notified = NULL;

	handle->add_module(handle, module);
	return module;
}

