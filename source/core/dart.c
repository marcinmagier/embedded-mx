
#include "mx/core/dart.h"

#include "mx/misc.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#if DEBUG_DART
  #include "mx/trace.h"
#endif





#define DART_SYNC_IDX           0
#define DART_LEN_IDX            1
#define DART_DATA_IDX           (DART_LEN_IDX + DART_LEN_SIZE)
#define DART_CRC_IDX(len)       (DART_DATA_IDX + len)

#define DART_SYNC_BYTES         1
#define DART_LEN_BYTES          (DART_SYNC_BYTES + DART_LEN_SIZE)
#define DART_FRAME_BYTES(len)   (DART_LEN_BYTES + len + DART_CRC_SIZE)



static uint8_t* dart_get_data(uint8_t *buffer)
{
    return &buffer[DART_DATA_IDX];
}


static dart_len_t dart_get_data_len(uint8_t *buffer)
{
#if DART_LEN_SIZE == 1
    return buffer[DART_LEN_IDX];
#elif DART_LEN_SIZE == 2
    return (uint16_t)(buffer[DART_LEN_IDX] << 8 | buffer[DART_LEN_IDX+1]);
#else
    return 0;
#endif
}

static void dart_set_data_len(uint8_t *buffer, dart_len_t data_len)
{
#if DART_LEN_SIZE == 1
    buffer[DART_LEN_IDX] = (uint8_t)(data_len & 0xFF);
#elif DART_LEN_SIZE == 2
    buffer[DART_LEN_IDX] = (uint8_t)((data_len >> 8) & 0xFF);
    buffer[DART_LEN_IDX+1] = (uint8_t)(data_len & 0xFF);
#endif
}


static uint32_t dart_get_crc_value(uint8_t *buffer, dart_len_t data_len)
{
#if DART_CRC_SIZE == 1
    return buffer[DART_CRC_IDX(data_len)];
#elif DART_CRC_SIZE == 2
    return (uint16_t)(buffer[DART_CRC_IDX(data_len)] << 8 | buffer[DART_CRC_IDX(data_len)+1]);
#else
    return 0;
#endif
}


static void dart_set_crc_value(uint8_t *buffer, dart_len_t data_len, uint32_t crc)
{
#if DART_CRC_SIZE == 1
    buffer[DART_CRC_IDX(data_len)] = (uint8_t)(crc & 0xFF);
#elif DART_CRC_SIZE == 2
    buffer[DART_CRC_IDX(data_len)] = (uint8_t)((crc >> 8) & 0xFF);
    buffer[DART_CRC_IDX(data_len)+1] = (uint8_t)(crc & 0xFF);
#endif
}


static uint32_t dart_calculate_crc(uint8_t *buffer, uint32_t length)
{
#if DART_CRC_SIZE == 1
    uint8_t crc = 0xFF;
    for (uint32_t i=0; i<length; i++) {
        crc ^= buffer[i];
    }
    return crc;
#elif DART_CRC_SIZE == 2
    uint16_t crc = 0xFFFF;
    uint16_t x;
    for (uint32_t i=0; i<length; i++) {
        x = crc >> 8 ^ buffer[i];
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
    }
    return crc;
#else
    return 0;
#endif
}





#define DART_BYTE_TIMER_VAL             30          // 30 milliseconds
#define DART_CLOSING_TIMER_VAL          50          // 50 milliseconds
#define DART_CHILL_TIMER_VAL            100         // 100 milliseconds
#define DART_ACK_TIMER_VAL              300         // 300 milliseconds
#define DART_WAKEUP_TIMER_VAL           500         // 500 milliseconds
#define DART_RESPONSE_TIMER_VAL         3000        // 3 seconds

#define DART_WAKEUP_ATTEMPTS            3
#define DART_TX_ATTEMPTS                3





int dart_transfer_msg(struct dart *self, struct msg_ptr *msg);



/**
 * Initialize module
 *
 */
void dart_init(struct dart *self, void *mpool, uint16_t mpool_size, void *rx_buffer, uint16_t rx_buffer_size)
{
    self->callback = NULL;
    self->callback_private = NULL;
    self->deferred_msg_callback = NULL;

    dart_reset(self);

    cba_init(&self->cba, mpool, mpool_size);

    self->rx_buffer = rx_buffer;
    self->rx_buffer_size = rx_buffer_size;

    self->running = true;
}


/**
 * Cleanup module
 *
 */
void dart_clean(struct dart *self)
{
    dart_reset(self);

    cba_clean(&self->cba);
}


/**
 * Reset module
 */
void dart_reset(struct dart *self)
{
    self->transfering = NULL;
    self->pending_request = NULL;

    self->wakeup_attempts = 0;
    timer_stop(&self->wakeup_timer);

    self->tx_attempts = 0;
    timer_stop(&self->tx_ack_timer);
    timer_stop(&self->tx_response_timer);

    self->rx_buffer_bytes = 0;
    timer_stop(&self->rx_byte_timer);

    timer_stop(&self->closing_timer);

    msg_queue_init(&self->queue);

    cba_reset(&self->cba);
}


/**
 * Callback setter
 *
 */
void dart_set_callback(struct dart *self, void *private, dart_callback_fn callback)
{
    self->callback = callback;
    self->callback_private = private;
}


/**
 * Deffered message callback setter
 */
void dart_set_deferred_msg_callback(struct dart *self, dart_deferred_msg_callback_fn callback)
{
    self->deferred_msg_callback = callback;
}


/**
 * Callback caller
 *
 */
static void dart_callback(struct dart *self, int code, void *param1, void *param2)
{
    if (self->callback)
        self->callback(code, param1, param2, self->callback_private);
}


/**
 * Deferred message callback caller
 *
 */
static bool dart_deferred_msg_callback(struct dart *self, struct msg_ptr *msg_ptr)
{
    if (self->deferred_msg_callback)
        return self->deferred_msg_callback(self, msg_ptr->msg.type);

    return false;
}





/**
 * Find non empty message queue
 *
 */
static struct msg_list* dart_find_next_message_queue(struct dart *self)
{
    for (int i=0; i<MSG_PRIO_LENGTH; i++) {
        struct msg_list *list = msg_queue_get_msg_list(&self->queue, i);
        if (msg_list_length(list) > 0) {
            struct msg_ptr *msg_ptr = msg_list_peek(list);
            if (self->pending_request && ((msg_ptr->msg.type & MSG_TYPE_MASK) == MSG_REQUEST))
                continue;   // Currently waiting for response, request could not be sent now

            return list;
        }
    }

    return NULL;
}


/**
 * Check if sending or will sent in the future
 *
 */
bool dart_is_sending(struct dart *self)
{
    if (self->transfering)
        return true;       // Sending

    if (self->pending_request)
        return true;       // Waiting for response

    if (dart_find_next_message_queue(self))
        return true;       // Message is waiting for transfer

    return false;
}


/**
 * Check if receiving
 *
 */
bool dart_is_receiving(struct dart *self)
{
    if (self->rx_buffer_bytes != 0)
        return true;

    return false;
}


/**
 * Check if idle
 *
 */
bool dart_is_idle(struct dart *self)
{
    if (dart_is_sending(self) || dart_is_receiving(self))
        return false;

    return true;
}


/**
 * Check if given message is pending
 *
 */
bool dart_is_msg_pending(struct dart *self, uint8_t prio, msgtype_t type)
{
    if (self->pending_request && self->pending_request->msg.type == type)
        return true;

    if (msg_queue_find_msgtype(&self->queue, &prio, type))
        return true;

    return false;
}


/**
 * Returns message type that is currently being processed
 *
 */
bool dart_get_current_msgtype(struct dart *self, msgtype_t *type)
{
    if (self->transfering) {
        struct msg_ptr *msg_ptr = msg_list_peek(self->transfering);
        *type = msg_ptr->msg.type;
        return true;
    }

    if (self->pending_request) {
        *type = self->pending_request->msg.type;
        return true;
    }


    return false;
}


/**
 * Finalize and clean transferred message
 *
 */
void dart_finalize_transfer(struct dart *self)
{
    if (self->transfering) {
        struct msg_ptr *msg_ptr = msg_list_pop(self->transfering);
        if (msg_ptr) {
            msg_ptr_free(&self->cba, msg_ptr);
        }
        self->transfering = NULL;
    }
}


/**
 * Trigger new transfer
 *
 */
int dart_trigger_transfer(struct dart *self)
{
    if (self->transfering)
        return DART_PENDING;

    if (!dart_pin_get_state(DART_WRK_PIN)) {
        // We are triggering communication
        if (self->wakeup_attempts++ < DART_WAKEUP_ATTEMPTS) {
            // We need to notify the other module about last message
            dart_pin_set_state(DART_WRK_PIN, true);
            if (!timer_running(&self->wakeup_timer))
                timer_start(&self->wakeup_timer, TIMER_MS, DART_WAKEUP_TIMER_VAL);
        }
        else {
            // Looks like receiver is not responding
            self->wakeup_attempts = 0;
            dart_callback(self, DART_CLBK_TRANSFER_IMPOSSIBLE, NULL, NULL);
            return DART_ERR_NOT_POSSIBLE;
        }
    }
    if (!dart_pin_get_state(DART_RDY_PIN)) {
        // Waiting for connection
        dart_callback(self, DART_CLBK_WAITING, NULL, NULL);
        return DART_WAITING;
    }

    self->transfering = dart_find_next_message_queue(self);
    if (!self->transfering) {
        if (self->pending_request)
            return DART_PENDING;
        return DART_IDLE;
    }

    self->tx_attempts = 0;
    self->wakeup_attempts = 0;
    timer_stop(&self->closing_timer);
    timer_stop(&self->wakeup_timer);
    return dart_transfer_msg(self, msg_list_peek(self->transfering));
}





/**
 * Transfer buffer through uart
 *
 */
int dart_push(struct dart *self, uint8_t *buffer, uint32_t length)
{
    UNUSED(self);

#if DEBUG_DART
    TRACE_DATA("TX:", buffer, length);
#endif
    dart_uart_send(buffer, length);

    return DART_SUCCESS;
}


/**
 * Transfer single byte
 *
 */
int dart_push_byte(struct dart *self, uint8_t byte)
{
    return dart_push(self, &byte, 1);
}


/**
 *  Compose and transfer frame based on given message data
 *
 */
int dart_push_frame(struct dart *self, uint8_t *data, dart_len_t data_len)
{
    uint32_t buffer_len = DART_FRAME_BYTES(data_len);
    uint8_t buffer[buffer_len];

    buffer[DART_SYNC_IDX] = DART_SYNC;
    dart_set_data_len(buffer, data_len);

    memcpy(dart_get_data(buffer), data, data_len);

    uint32_t crc = dart_calculate_crc(data, data_len);
    dart_set_crc_value(buffer, data_len, crc);

    dart_push(self, buffer, buffer_len);

    return DART_SUCCESS;
}


/**
 * Push message object
 *
 */
int dart_push_msg(struct dart *self, struct msg *msg, dart_len_t msg_len)
{
    return dart_push_frame(self, (uint8_t*)msg, msg_len);
}


/**
 * Push message constructed from msgtype and payload
 *
 */
int dart_push_msg_payload(struct dart *self, msgtype_t msgtype, uint8_t *payload, dart_len_t payload_len)
{
    uint32_t msg_size = sizeof(struct msg_ptr) + payload_len;
    uint8_t msg_buffer[msg_size];

    struct msg_ptr *msg_ptr = (struct msg_ptr*)msg_buffer;
    msg_ptr->length = sizeof(msgtype) + payload_len;

    struct msg_data *msg = (struct msg_data*)&msg_ptr->msg;
    msg->type = msgtype;
    if (payload_len)
        memcpy(&msg->data, payload, payload_len);

    return dart_push_msg(self, (struct msg*)msg, msg_ptr->length);
}


/**
 * Transfer message object
 *
 */
int dart_transfer_msg(struct dart *self, struct msg_ptr *msg_ptr)
{
    bool transferred = false;

    if (msg_ptr->length == sizeof(struct msg))
        transferred = dart_deferred_msg_callback(self, msg_ptr);

    if (!transferred)
        dart_push_frame(self, (uint8_t*)&msg_ptr->msg, msg_ptr->length);

    timer_start(&self->tx_ack_timer, TIMER_MS, DART_ACK_TIMER_VAL);
    return DART_SUCCESS;
}


/**
 * Trigger new transfer message
 *
 * Send notification if there are no more pending messages
 */
void dart_trigger_next_transfer(struct dart *self)
{
    int status = dart_trigger_transfer(self);

    if (!self->pending_request && status == DART_IDLE) {
        // No more queued messages
        dart_callback(self, DART_CLBK_TRANSFER_COMPLETE, NULL, NULL);
    }
}


/**
 * Transfer current message again
 *
 * Send notification if case of permanent filure
 */
void dart_retry_transfer(struct dart *self)
{
    if (!self->transfering)
        return;

    struct msg_ptr *msg_ptr = msg_list_peek(self->transfering);
    if (msg_ptr) {
        timer_stop(&self->tx_ack_timer);
        if (++self->tx_attempts < DART_TX_ATTEMPTS) {
            // Resend message again
            dart_transfer_msg(self, msg_ptr);
        }
        else {
            // Report permanent transfer failure
            dart_callback(self, DART_CLBK_TRANSFER_FAILURE, &msg_ptr->msg, (void*)msg_ptr->length);
            dart_finalize_transfer(self);
            dart_trigger_next_transfer(self);
        }
    }
}


/**
 * Acknowledge current message
 *
 * If acknowledged message is request, put it into pending list.
 */
void dart_acknowledge_msg(struct dart *self)
{
    if (!self->transfering)
        return;

    struct msg_ptr *msg_ptr = msg_list_peek(self->transfering);
    if (!msg_ptr)
        return;

    timer_stop(&self->tx_ack_timer);
    if ((msg_ptr->msg.type & MSG_TYPE_MASK) == MSG_REQUEST) {
        // We need to wait for response if message is REQUEST
        timer_start(&self->tx_response_timer, TIMER_MS, DART_RESPONSE_TIMER_VAL);
        self->pending_request = msg_list_pop(self->transfering);
        self->transfering = NULL;
    }
    else {
        // Messgae is complete
        dart_callback(self, DART_CLBK_TRANSFER_DONE, &msg_ptr->msg, (void*)msg_ptr->length);
    }

    dart_finalize_transfer(self);
    dart_trigger_next_transfer(self);
}


/**
 * Finalize and clean pending request message
 *
 */
void dart_finalize_request_msg(struct dart *self)
{
    timer_stop(&self->tx_response_timer);
    cba_free(&self->cba, self->pending_request);
    self->pending_request = NULL;

    dart_trigger_next_transfer(self);
}


/**
 * Abandon pending request message
 *
 * Send appropriate notification
 */
void dart_abandon_request_msg(struct dart *self)
{
    if (self->pending_request) {
        dart_callback(self, DART_CLBK_MESSAGE_ABANDONED, &self->pending_request->msg, (void*)self->pending_request->length);
        dart_finalize_request_msg(self);
    }
}


/**
 * Handle received data
 *
 * Check if received message is a response for pending request.
 * Send appropriate notification.
 */
void dart_handle_received_msg(struct dart *self, struct msg *received_msg, size_t msg_len)
{
    // Save original msg type
    // It might be overwritten within callback, to unify RESPONSE/REPORT message handler
    msgtype_t received_msg_type = received_msg->type;

    dart_callback(self, DART_CLBK_MESSAGE_RECEIVED, received_msg, (void*)msg_len);

    // Check if received message is expected response
    if (!self->pending_request)
        return;     // We ware not waiting for response
    if ((received_msg_type & MSG_TYPE_MASK) != MSG_RESPONSE)
        return;     // Received message is not RESPONSE
    if ((self->pending_request->msg.type & MSG_ID_MASK) != (received_msg_type & MSG_ID_MASK))
        return;     // We are waiting for another response id

    dart_callback(self, DART_CLBK_TRANSFER_DONE, &self->pending_request->msg, (void*)msg_len);
    dart_finalize_request_msg(self);
}


/**
 * Receiving handler
 *
 */
void dart_handle_received_char(struct dart *self, uint8_t ch)
{
    if (timer_running(&self->rx_byte_timer) && timer_expired(&self->rx_byte_timer)) {
#if DEBUG_DART
        TRACE_DATA("RXE:", self->rx_buffer, self->rx_buffer_bytes);
#endif
        dart_callback(self, DART_CLBK_TRANSFER_INCOMPLETE, NULL, NULL);
        timer_stop(&self->rx_byte_timer);
        self->rx_buffer_bytes = 0;
    }

    timer_stop(&self->closing_timer);
    timer_restart(&self->rx_byte_timer);
    self->rx_buffer[self->rx_buffer_bytes++] = ch;

    if (self->rx_buffer_bytes == DART_SYNC_BYTES) {
        if (ch == DART_SYNC) {
            // New message incoming
            timer_start(&self->rx_byte_timer, TIMER_MS, DART_BYTE_TIMER_VAL);
            if (!dart_pin_get_state(DART_WRK_PIN)) {
                // Probably the opponent started transfer just before our closing
                dart_pin_set_state(DART_WRK_PIN, true);
            }
            return;
        }
        if (dart_pin_get_state(DART_WRK_PIN)) {
            switch (ch) {
                case DART_ACK:
                    dart_acknowledge_msg(self);
                    break;
                case DART_BAD:
                    dart_retry_transfer(self);
                    break;
                case DART_CAN:
                    dart_trigger_transfer(self);
                    break;
                default:
                    // Missing sync byte, ignore
                    break;
            }
        }
#if DEBUG_DART
        TRACE_DATA("RX:", &ch, 1);
#endif
        self->rx_buffer_bytes = 0;
        timer_stop(&self->rx_byte_timer);
        return;
    }

    if (self->rx_buffer_bytes < DART_LEN_BYTES)
        return;     // Wait for payload length

    uint32_t data_len = dart_get_data_len(self->rx_buffer);
    uint32_t frame_len = DART_FRAME_BYTES(data_len);
    if (frame_len > self->rx_buffer_size) {
        // Frame length is not supported
#if DEBUG_DART
        TRACE_DATA("RXE:", self->rx_buffer, self->rx_buffer_bytes);
#endif
        dart_callback(self, DART_CLBK_TRANSFER_CORRUPTED, NULL, NULL);
        timer_stop(&self->rx_byte_timer);
        self->rx_buffer_bytes = 0;
        return;
    }
    if (self->rx_buffer_bytes < frame_len)
        return;     // Wait for entire frame

    // Frame complete
#if DEBUG_DART
    TRACE_DATA("RX:", self->rx_buffer, self->rx_buffer_bytes);
#endif
    uint32_t crc_received = dart_get_crc_value(self->rx_buffer, data_len);
    uint32_t crc_calculated = dart_calculate_crc(dart_get_data(self->rx_buffer), data_len);
    if (crc_calculated != crc_received) {
//        WARN("Invalid crc, expected %02X, received %02X", crc_calculated, crc_received);
        dart_push_byte(self, DART_BAD);
        dart_callback(self, DART_CLBK_TRANSFER_CORRUPTED, NULL, NULL);
    }
    else {
        dart_push_byte(self, DART_ACK);
        dart_handle_received_msg(self, (struct msg*)&self->rx_buffer[DART_DATA_IDX], data_len);
    }

    self->rx_buffer_bytes = 0;
    timer_stop(&self->rx_byte_timer);
}


/**
 * Handler called when outgoing transfer is complete
 *
 */
void dart_handle_transfer_done(struct dart *self)
{
    UNUSED(self);
}


/**
 * Time handler
 *
 * Should be called every 1ms
 */
int dart_handle_time(struct dart *self)
{
    if (self->pending_request) {
        if (timer_expired(&self->tx_response_timer)) {
            dart_abandon_request_msg(self);
        }
    }

    if (self->transfering) {
        if (timer_expired(&self->tx_ack_timer)) {
            timer_stop(&self->tx_ack_timer);
            if (dart_pin_get_state(DART_RDY_PIN)) {
                dart_retry_transfer(self);
            } else {
                // Looks like the opponent is not ready
                self->transfering = NULL;
            }
        }
    } else {
        // Nothing is being transferred
        if (dart_find_next_message_queue(self)) {
            if (timer_expired(&self->wakeup_timer)) {
                // Looks like opponent is not responding
                timer_stop(&self->wakeup_timer);
                // Generate spike just in case opponent requires edge to wakup
                dart_pin_set_state(DART_WRK_PIN, false);
                return DART_WAITING;
            }
            else {
                // Re-try transfering
                return dart_trigger_transfer(self);
            }
        } else {
            // There are no more messages
            if (!self->pending_request && (self->rx_buffer_bytes == 0)) {
                if (dart_pin_get_state(DART_WRK_PIN)) {
                    if (timer_running(&self->closing_timer)) {
                        // Closing
                        if (timer_expired(&self->closing_timer)) {
                            // Wait some time before idle
                            dart_pin_set_state(DART_WRK_PIN, false);
                            timer_start(&self->closing_timer, TIMER_MS, DART_CHILL_TIMER_VAL);
                        }
                    }
                    else {
                        // Working, wait some time before closing
                        timer_start(&self->closing_timer, TIMER_MS, DART_CLOSING_TIMER_VAL);
                    }
                }
                else {
                    if (timer_running(&self->closing_timer)) {
                        // Chilling
                        if (timer_expired(&self->closing_timer)) {
                            timer_stop(&self->closing_timer);
                            if (!dart_pin_get_state(DART_RDY_PIN)) {
                                // Finally idle
                                dart_callback(self, DART_CLBK_IDLE, NULL, NULL);
                            }
                        }
                    }
                    else {
                        // Idle
                        if (dart_pin_get_state(DART_RDY_PIN)) {
                            dart_pin_set_state(DART_WRK_PIN, true);
                            return DART_PENDING;
                        }
                    }
                }
                return DART_IDLE;
            }
        }
    }

    return DART_PENDING;
}


/**
 * Guess priority from msg type
 *
 */
uint8_t dart_guess_priority(uint8_t msg_type)
{
    switch (msg_type & MSG_TYPE_MASK) {
        case MSG_REPORT:
            return DART_MSG_PRIO_REPORT;
        case MSG_REQUEST:
            return DART_MSG_PRIO_REQUEST;
        case MSG_RESPONSE:
            return DART_MSG_PRIO_RESPONSE;
    }

    // Could not guess
    return DART_MSG_PRIO_ANY;
}


/**
 * Send message data
 *
 */
int dart_send_msg_ex(struct dart *self, uint8_t prio, struct msg *msg, dart_len_t msg_len)
{
    if (!self->running)
        return DART_ERR_NOT_POSSIBLE;

    if (prio == DART_MSG_PRIO_ANY)
        prio = dart_guess_priority(msg->type);
    if (prio >= MSG_PRIO_LENGTH)
        return DART_ERR_NOT_POSSIBLE;

    struct msg_ptr *msg_ptr = msg_ptr_malloc(&self->cba, msg_len);
    if (!msg_ptr)
        return DART_ERR_NO_MEMORY;

    memcpy(&msg_ptr->msg, msg, msg_len);
    msg_ptr->length = msg_len;

    msg_queue_push(&self->queue, prio, msg_ptr);
    return dart_trigger_transfer(self);
}


/**
 * Send message type
 *
 * May be used for deffered messages.
 *
 */
int dart_send_msgtype_ex(struct dart *self, uint8_t prio, msgtype_t msgtype)
{
    struct msg tmp = {
        .type = msgtype
    };

    return dart_send_msg_ex(self, prio, &tmp, sizeof(tmp));
}


/**
 * Send message data, guess priority
 *
 */
int dart_send_msg(struct dart *self, struct msg *msg, dart_len_t msg_len)
{
    return dart_send_msg_ex(self, DART_MSG_PRIO_ANY, msg, msg_len);
}


/**
 * Send message type, guess priority
 */
int dart_send_msgtype(struct dart *self, msgtype_t msgtype)
{
    return dart_send_msgtype_ex(self, DART_MSG_PRIO_ANY, msgtype);
}
