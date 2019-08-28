#include <stdint.h>

#include "periph/uart.h"
#include "strato3.h"

#define STRATO3_UART_BUFSIZE    (512U)
#define STRATO3_UART_TIMEOUT    (1000000UL)

#define STRATO3_LINE_BUFSIZE    (128U)

void strato_print(const strato3_data_t *data);

int strato_json(const strato3_data_t *data, char *buf, size_t len);

int strato_json_ogc_l(const strato3_data_t *data, char *buf, size_t len);

int strato_json_ogc_h(const strato3_data_t *data, char *buf, size_t len);

int strato_json_ogc_p(const strato3_data_t *data, char *buf, size_t len);

int strato_json_ogc_t(const strato3_data_t *data, char *buf, size_t len);

int strato_read(strato3_data_t *data);

void strato_init(uart_t dev, uint32_t baud);
