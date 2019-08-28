#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "fmt.h"
#include "isrpipe.h"
#include "isrpipe/read_timeout.h"
#include "periph/uart.h"

#include "config.h"
#include "strato_wrapper.h"
#include "strato3.h"

#define ENABLE_DEBUG        (1)
#include "debug.h"

static uint8_t _rx_buf_mem[STRATO3_UART_BUFSIZE];
isrpipe_t strato3_uart_isrpipe = ISRPIPE_INIT(_rx_buf_mem);
static uart_t strato3_dev;
static char cmd[] = "Ti\n";

/* Code parts are taken from https://github.com/RIOT-OS/RIOT/pull/7086 */
static ssize_t _readline(isrpipe_t *isrpipe, char *resp_buf, size_t len, uint32_t timeout)
{
    DEBUG("%s\n", __func__);
    ssize_t res = -1;
    char *resp_pos = resp_buf;

    while (len) {
        int read_res;
        if ((read_res = isrpipe_read_timeout(isrpipe, (uint8_t *)resp_pos, 1, timeout)) == 1) {
            if (*resp_pos == '\r') {
                continue;
            }
            if (*resp_pos == '\n') {
                *resp_pos = '\0';
                res = resp_pos - resp_buf;
                goto out;
            }

            resp_pos += read_res;
            len -= read_res;
        }
        else if (read_res == -ETIMEDOUT) {
            res = -ETIMEDOUT;
            break;
        }
    }

out:
    return res;
}

void strato_print(const strato3_data_t *data)
{
    printf("strato3_data = {\n");
    printf("  uptime:           %02u:%02u:%02u\n", data->uptime.hour, data->uptime.min, data->uptime.sec);
    printf("  time:             %02u:%02u:%02u\n", data->time.hour, data->time.min, data->time.sec);
    printf("  date:             %u-%02u-%02u\n", data->date.year, data->date.month, data->date.day);
    printf("  valid:            %c\n", data->valid ? 'Y' : 'N');
    printf("  satellites:       %u\n", data->satellites);
    print_str("  latitude:         "); print_float(strato3_tocoord(&data->latitude), 7); puts("");
    print_str("  longitude:        "); print_float(strato3_tocoord(&data->longitude), 7); puts("");
    print_str("  speed (knt):      "); print_float(strato3_tofloat(&data->speed_knt), 4); puts("");
    print_str("  speed (kph):      "); print_float(strato3_tofloat(&data->speed_kph), 1); puts("");
    print_str("  course:           "); print_float(strato3_tofloat(&data->course), 1); puts("");
    print_str("  altitude:         "); print_float(strato3_tofloat(&data->altitude), 1); puts("");
    print_str("  temperature(in):  "); print_float(strato3_tofloat(&data->temperature_board), 1); puts("");
    print_str("  temperature(ex):  "); print_float(strato3_tofloat(&data->temperature), 1); puts("");
    print_str("  humidity:         "); print_float(strato3_tofloat(&data->humidity), 1); puts("");
    print_str("  pressure:         "); print_float(strato3_tofloat(&data->pressure), 3); puts("");
    print_str("  voltage:          "); print_float(strato3_tofloat(&data->voltage), 1); puts("");
    printf("  state:            %d\n", (int)data->state);
    printf("}\n");
}

int strato_json(const strato3_data_t *data, char *buf, size_t len)
{
    if (!buf || !len) {
        DEBUG("%s: invalid buffer!\n", __func__);
        return (-1);
    }
    memset(buf, '\0', len);
    int pos = 0;
    char fmtbuf[32];
    pos += snprintf(buf, len, "{\'dt\':\'%u-%02u-%02uT%02u:%02u:%02uZ\',",
                    data->date.year, data->date.month, data->date.day,
                    data->time.hour, data->time.min, data->time.sec);
    memset(fmtbuf, '\0', 32);
    fmt_float(fmtbuf, strato3_tocoord(&data->latitude), 7);
    pos += snprintf(buf+pos, len-pos, "\'lat\':%s,", fmtbuf);
    memset(fmtbuf, '\0', 32);
    fmt_float(fmtbuf, strato3_tocoord(&data->longitude), 7);
    pos += snprintf(buf+pos, len-pos, "\'lon\':%s,", fmtbuf);
    memset(fmtbuf, '\0', 32);
    fmt_float(fmtbuf, strato3_tofloat(&data->altitude), 1);
    pos += snprintf(buf+pos, len-pos, "\'alt\':%s,", fmtbuf);
    memset(fmtbuf, '\0', 32);
    fmt_float(fmtbuf, strato3_tofloat(&data->temperature), 1);
    pos += snprintf(buf+pos, len-pos, "\'t\':%s,", fmtbuf);
    memset(fmtbuf, '\0', 32);
    fmt_float(fmtbuf, strato3_tofloat(&data->humidity), 1);
    pos += snprintf(buf+pos, len-pos, "\'h\':%s,", fmtbuf);
    memset(fmtbuf, '\0', 32);
    fmt_float(fmtbuf, strato3_tofloat(&data->pressure), 3);
    pos += snprintf(buf+pos, len-pos, "\'p\':%s}", fmtbuf);
    buf[pos] = '\0';
    return 0;
}

int strato_json_ogc_l(const strato3_data_t *data, char *buf, size_t len)
{
    if (!buf || !len) {
        DEBUG("%s: invalid buffer!\n", __func__);
        return (-1);
    }
    memset(buf, '\0', len);
    int pos = 0;
    char fmtbuf[32];
    pos += snprintf(buf, len, "{\"phenomenonTime\":\"%u-%02u-%02uT%02u:%02u:%02uZ\",",
                    data->date.year, data->date.month, data->date.day,
                    data->time.hour, data->time.min, data->time.sec);
    memset(fmtbuf, '\0', 32);
    fmt_float(fmtbuf, strato3_tocoord(&data->latitude), 7);
    pos += snprintf(buf+pos, len-pos, "\"result\":[%s,", fmtbuf);
    memset(fmtbuf, '\0', 32);
    fmt_float(fmtbuf, strato3_tocoord(&data->longitude), 7);
    pos += snprintf(buf+pos, len-pos, "%s]}", fmtbuf);
    buf[pos] = '\0';
    return 0;
}

int strato_json_ogc_h(const strato3_data_t *data, char *buf, size_t len)
{
    if (!buf || !len) {
        DEBUG("%s: invalid buffer!\n", __func__);
        return (-1);
    }
    memset(buf, '\0', len);
    int pos = 0;
    char fmtbuf[32];
    pos += snprintf(buf, len, "{\"phenomenonTime\":\"%u-%02u-%02uT%02u:%02u:%02uZ\",",
                    data->date.year, data->date.month, data->date.day,
                    data->time.hour, data->time.min, data->time.sec);
    memset(fmtbuf, '\0', 32);
    fmt_float(fmtbuf, strato3_tofloat(&data->humidity), 1);
    pos += snprintf(buf+pos, len-pos, "\"result\":%s}", fmtbuf);
    buf[pos] = '\0';
    return 0;
}

int strato_json_ogc_p(const strato3_data_t *data, char *buf, size_t len)
{
    if (!buf || !len) {
        DEBUG("%s: invalid buffer!\n", __func__);
        return (-1);
    }
    memset(buf, '\0', len);
    int pos = 0;
    char fmtbuf[32];
    pos += snprintf(buf, len, "{\"phenomenonTime\":\"%u-%02u-%02uT%02u:%02u:%02uZ\",",
                    data->date.year, data->date.month, data->date.day,
                    data->time.hour, data->time.min, data->time.sec);
    memset(fmtbuf, '\0', 32);
    fmt_float(fmtbuf, strato3_tofloat(&data->pressure), 3);
    pos += snprintf(buf+pos, len-pos, "\"result\":%s}", fmtbuf);
    buf[pos] = '\0';
    return 0;
}

int strato_json_ogc_t(const strato3_data_t *data, char *buf, size_t len)
{
    if (!buf || !len) {
        DEBUG("%s: invalid buffer!\n", __func__);
        return (-1);
    }
    memset(buf, '\0', len);
    int pos = 0;
    char fmtbuf[32];
    pos += snprintf(buf, len, "{\"phenomenonTime\":\"%u-%02u-%02uT%02u:%02u:%02uZ\",",
                    data->date.year, data->date.month, data->date.day,
                    data->time.hour, data->time.min, data->time.sec);
    memset(fmtbuf, '\0', 32);
    fmt_float(fmtbuf, strato3_tofloat(&data->temperature), 1);
    pos += snprintf(buf+pos, len-pos, "\"result\":%s}", fmtbuf);
    buf[pos] = '\0';
    return 0;
}

int strato_read(strato3_data_t *data)
{
    DEBUG("%s\n", __func__);

    char linebuf[STRATO3_LINE_BUFSIZE];
    memset(linebuf, 0, STRATO3_LINE_BUFSIZE);

    uart_write(strato3_dev, (uint8_t *)cmd, 3);
    ssize_t res = _readline(&strato3_uart_isrpipe,
                            linebuf, sizeof(linebuf),
                            STRATO3_UART_TIMEOUT);
    if (res > 3) {
        DEBUG(".. [%s]\n", linebuf);
        if (strato3_parse(linebuf, data) == 0) {
            return 0;
        }
    }
    DEBUG(".. timeout\n");
    return -ETIMEDOUT;
}

void strato_init(uart_t dev, uint32_t baud)
{
    DEBUG("%s\n", __func__);
    strato3_dev = dev;
    /* Initialize UART */
    uart_init(strato3_dev, baud, (uart_rx_cb_t) isrpipe_write_one, &strato3_uart_isrpipe);
}
