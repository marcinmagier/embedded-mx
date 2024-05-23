

#ifndef __MX_DART_H_
#define __MX_DART_H_


#include "mx/core/message.h"
#include "mx/core/message-queue.h"

#include "mx/timer.h"
#include "mx/cba.h"

#include <stdbool.h>
#include <stdint.h>



#ifndef DART_LEN_SIZE
  #define DART_LEN_SIZE                     1
#endif
#ifndef DART_CRC_SIZE
  #define DART_CRC_SIZE                     1
#endif


#if (DART_LEN_SIZE != 1) && (DART_LEN_SIZE != 2)
  #error Unsupported DART_LEN_SIZE value
#endif
#if (DART_CRC_SIZE != 1) && (DART_CRC_SIZE != 2)
  #error Unsupported DART_CRC_SIZE value
#endif





#define DART_SYNC       0x55            ///< Synchronization
#define DART_ACK        0xAA            ///< Acknowledge
#define DART_BAD        0xBB            ///< Bad message (invalid crc)
#define DART_CAN        0xCC            ///< Ready for communication
#define DART_DONE       0xDD            ///< No more messages
#define DART_EOC        0xEE            ///< End of communication



enum dart_status_e
{
    DART_SUCCESS = 0,
    DART_PENDING,
    DART_WAITING,
    DART_IDLE,

    DART_ERR_NOT_POSSIBLE = -1,
    DART_ERR_NOT_SUPPORTED = -2,
    DART_ERR_BAD_LENGTH = -3 ,
    DART_ERR_NO_MEMORY = -4,
    DART_ERR_MSG_NOT_FOUND = -5,
};


enum dart_msg_priority_e
{
    DART_MSG_PRIO_RESPONSE = MSG_PRIO_HIGH,
    DART_MSG_PRIO_REPORT = MSG_PRIO_NORMAL,
    DART_MSG_PRIO_REQUEST = MSG_PRIO_LOW,

    DART_MSG_PRIO_ANY = MSG_PRIO_ANY,       // May be used to check if message is pending
};



enum dart_callback_code_e
{
    DART_CLBK_IDLE,                 ///< Module ready to go idle
    DART_CLBK_WAITING,              ///< Module waiting for connection
    DART_CLBK_TRANSFER_DONE,        ///< Message successfully transferred, ACK was received
    DART_CLBK_TRANSFER_FAILURE,     ///< Message was not transferred, NACK was received or ACK was not received
    DART_CLBK_TRANSFER_COMPLETE,    ///< All messages successfully transferred, idle mode
    DART_CLBK_TRANSFER_INCOMPLETE,  ///< Receiving message was timeouted
    DART_CLBK_TRANSFER_CORRUPTED,   ///< Received invalid message
    DART_CLBK_TRANSFER_IMPOSSIBLE,  ///< Message was not transferred, connection not possible
    DART_CLBK_MESSAGE_RECEIVED,     ///< Received valid message
    DART_CLBK_MESSAGE_ABANDONED,    ///< No RESPONSE received for the REQUEST message
};




#if DART_LEN_SIZE == 1
  typedef uint8_t dart_len_t;
#elif DART_LEN_SIZE == 2
  typedef uint16_t dart_len_t;
#else
  typedef uint32_t dart_len_t;
#endif





struct dart;

typedef void (*dart_callback_fn)(int code, void *param1, void *param2, void *private);
typedef bool (*dart_deferred_msg_callback_fn)(struct dart *self, msgtype_t msgtype);





struct dart
{
    struct cba cba;

    struct msg_queue queue;
    struct msg_list *transfering;
    struct msg_ptr* pending_request;

    struct timer wakeup_timer;
    uint8_t wakeup_attempts;

    struct timer tx_ack_timer;
    struct timer tx_response_timer;
    uint8_t tx_attempts;

    uint8_t *rx_buffer;
    uint16_t rx_buffer_size;
    uint16_t rx_buffer_bytes;
    struct timer rx_byte_timer;

    dart_callback_fn callback;
    void *callback_private;

    dart_deferred_msg_callback_fn deferred_msg_callback;

    struct timer closing_timer;

    bool running;
};



void dart_init(struct dart *self, void *mpool, uint16_t mpool_size, void *rx_buffer, uint16_t rx_buffer_size);
void dart_clean(struct dart *self);
void dart_reset(struct dart *self);

void dart_set_callback(struct dart *self, void *private, dart_callback_fn callback);
void dart_set_deferred_msg_callback(struct dart *self, dart_deferred_msg_callback_fn callback);

bool dart_is_idle(struct dart *self);
bool dart_is_sending(struct dart *self);
bool dart_is_receiving(struct dart *self);

bool dart_is_msg_pending(struct dart *self, uint8_t prio, msgtype_t type);
bool dart_get_current_msgtype(struct dart *self, msgtype_t *type);

void dart_handle_transfer_done(struct dart *self);
int dart_handle_time(struct dart *self);
void dart_handle_received_char(struct dart *self, uint8_t ch);

int dart_send_msg_ex(struct dart *self, uint8_t prio, struct msg *msg, dart_len_t msg_len);
int dart_send_msgtype_ex(struct dart *self, uint8_t prio, msgtype_t msgtype);

int dart_send_msg(struct dart *self, struct msg *msg, dart_len_t msg_len);
int dart_send_msgtype(struct dart *self, msgtype_t msgtype);


// Functions may be used within deffered message definition callback
int dart_push_msg(struct dart *self, struct msg *msg, dart_len_t msg_len);
int dart_push_msg_payload(struct dart *self, msgtype_t msgtype, uint8_t *payload, dart_len_t payload_len);




enum dart_pin_e
{
    DART_WRK_PIN,
    DART_RDY_PIN,
};


extern bool dart_pin_get_state(int pin_e);
extern void dart_pin_set_state(int pin_e, bool state);

//extern void dart_uart_open(void);
//extern void dart_uart_close(void);
extern void dart_uart_send(uint8_t *buffer, int32_t length);




#endif /* __MX_DART_H_ */
