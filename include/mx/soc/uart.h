
#ifndef __MX_SOC_UART_H_
#define __MX_SOC_UART_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


/*
enum uart_parity_e
{
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN,
    UART_PARITY_ODD,
};
*/


enum uart_event_e
{
    UART_TX_COMPLETE,
    UART_RX_RECEIVED,
};



struct uart_stats
{
    uint32_t rx_bytes;
    uint32_t tx_bytes;
    uint32_t overrun_errors;
    uint32_t parity_errors;
    uint32_t queue_errors;
};



struct uart_drv;



typedef void (*uart_callback_t)(int uart_event);


void uart_open(struct uart_drv *drv, uart_callback_t clbk);

void uart_enable(struct uart_drv *drv);
void uart_disable(struct uart_drv *drv);

void uart_configure_frame(struct uart_drv *drv, uint8_t data_bits, uint8_t stop_bits, uint8_t parity);
void uart_configure_baudrate(struct uart_drv *drv, uint32_t baud);

bool uart_is_transfer_possible(struct uart_drv *drv);
bool uart_is_transfer_complete(struct uart_drv *drv);

int  uart_write(struct uart_drv *drv, const uint8_t *buffer, size_t len);
int  uart_read(struct uart_drv *drv, uint8_t *buffer, size_t len);
int  uart_flush(struct uart_drv *drv);
size_t uart_peek(struct uart_drv *drv);


const struct uart_stats* uart_get_stats(struct uart_drv *drv);
void uart_clear_stats(struct uart_drv *drv);



#endif /* __MX_SOC_UART_H_ */
