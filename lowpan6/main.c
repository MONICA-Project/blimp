#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "fmt.h"
#include "periph/uart.h"
#include "xtimer.h"

#include "net/af.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/netapi.h"
#include "net/gnrc/netif.h"
#include "net/gnrc.h"
#include "net/gnrc/netif/hdr.h"
#include "net/gnrc/udp.h"

#include "config.h"
#include "strato_wrapper.h"
#include "strato3.h"

#define ENABLE_DEBUG        (0)
#include "debug.h"

#define STRBUF_LEN          (120U)
static char strbuf[STRBUF_LEN];

extern int coap_init(void);
extern void coap_put_data(char *data, char *path);

static int comm_init(void)
{
    DEBUG("[MAIN] %s\n", __func__);
    uint16_t pan = COMM_PAN;
    uint16_t chan = COMM_CHAN;
    /* get the PID of the first radio */
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);
    if (netif == NULL) {
        DEBUG("[MAIN] ERROR: no network interface found!\n");
        return 1;
    }
    kernel_pid_t iface = netif->pid;
    /* initialize the radio */
    gnrc_netapi_set(iface, NETOPT_NID, 0, &pan, 2);
    gnrc_netapi_set(iface, NETOPT_CHANNEL, 0, &chan, 2);
    return 0;
}

int main(void)
{
    // some initial infos
    puts(" MONICA Blimp Demo \n");
    puts("===================\n");
    // init 6lowpan interface
    LED0_ON;
    DEBUG(".. init network\n");
    if (comm_init() != 0) {
        return 1;
    }
    DEBUG(".. init sensor\n");
    /* Initialize and enable gps */
    strato_init(STRATO3_UART_DEV, STRATO3_UART_BAUDRATE);
    DEBUG(".. init coap\n");
    /* Initialize and enable gps */
    if (coap_init() != 0) {
        return 1;
    }

    DEBUG(".. init DONE!\n\n");
    while (1) {
        strato3_data_t data;
        memset(&data, 0, sizeof(strato3_data_t));
        printf("\n.. read sensor data\n");
        if(strato_read(&data) == 0) {
            strato_print(&data);
            /* send location */
            strato_json_ogc_l(&data, strbuf, STRBUF_LEN);
            DEBUG(" * LOCATION: %s\n", strbuf);
            coap_put_data(strbuf, CONFIG_PATH_LOCATION);
            /* send humidity */
            strato_json_ogc_h(&data, strbuf, STRBUF_LEN);
            DEBUG(" * HUMIDITY: %s\n", strbuf);
            coap_put_data(strbuf, CONFIG_PATH_HUMITIDY);
            /* send pressure */
            strato_json_ogc_p(&data, strbuf, STRBUF_LEN);
            DEBUG(" * PRESSURE: %s\n", strbuf);
            coap_put_data(strbuf, CONFIG_PATH_PRESSURE);
            /* send temperature */
            strato_json_ogc_t(&data, strbuf, STRBUF_LEN);
            DEBUG(" * TEMPERATURE: %s\n", strbuf);
            coap_put_data(strbuf, CONFIG_PATH_TEMPERATURE);
        }
        else {
            DEBUG(" ! got error !\n");
        }
        xtimer_sleep(CONFIG_INTERVAL_S);
    }

    return 0;
}
