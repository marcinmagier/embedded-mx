
#ifndef __MX_SOC_UART_H_
#define __MX_SOC_UART_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>




enum uart_event_e
{
    UART_TX_COMPLETE,
    UART_RX_RECEIVED,
};

struct uart_drv;

typedef void (*uart_callback_t)(int uart_event);


void uart_open(struct uart_drv *drv, uart_callback_t clbk);

void uart_enable(struct uart_drv *drv);
void uart_disable(struct uart_drv *drv);

bool uart_transfer_complete(struct uart_drv *drv);

int  uart_write(struct uart_drv *drv, const uint8_t *buffer, size_t len);
int  uart_read(struct uart_drv *drv, uint8_t *buffer, size_t len);
int  uart_flush(struct uart_drv *drv);
size_t uart_peek(struct uart_drv *drv);


#endif /* __MX_SOC_UART_H_ */
