
#include <CUnit/Basic.h>

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "mx/core/dart.h"
#include "mx/timer.h"
#include "mx/misc.h"


#if DEBUG_DART
void LOG_DATA(const char *prefix, uint8_t *data, uint32_t data_len)
{
    printf("%s", prefix);
    for (uint32_t i=0; i<data_len; i++) {
        printf(" %02X", data[i]);
    }
    printf("\n");
}
#endif


static void test_receiving_problems(void);
static void test_receiving_messages(void);
static void test_sending_problems(void);
static void test_resending(void);
static void test_sending_reports(void);
static void test_sending_requests(void);
static void test_abandon_requests(void);
static void test_sending_reports_while_waiting_for_response(void);

static void test_idle_scenario(void);
static void test_waiting_scenario(void);
static void test_sending_scenario(void);
static void test_receiving_scenario(void);

static void test_deferred_msg_content(void);


CU_ErrorCode cu_test_dart()
{
    CU_pSuite suite;

    // General suite
    suite = CU_add_suite("One", NULL, NULL);
    if ( !suite ) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_add_test(suite, "Receiving problems",                            test_receiving_problems);
    CU_add_test(suite, "Receiving messages",                            test_receiving_messages);
    CU_add_test(suite, "Sending problems",                              test_sending_problems);
    CU_add_test(suite, "Resending",                                     test_resending);
    CU_add_test(suite, "Sending reports",                               test_sending_reports);
    CU_add_test(suite, "Sending requests",                              test_sending_requests);
    CU_add_test(suite, "Abandon requests",                              test_abandon_requests);
    CU_add_test(suite, "Sending reports while waiting for responses",   test_sending_reports_while_waiting_for_response);

    CU_add_test(suite, "Idle scenario",                                 test_idle_scenario);
    CU_add_test(suite, "Waiting scenario",                              test_waiting_scenario);
    CU_add_test(suite, "Sending scenario",                              test_sending_scenario);
    CU_add_test(suite, "Receiving scenario",                            test_receiving_scenario);

    CU_add_test(suite, "Deffered message content",                      test_deferred_msg_content);

    return CU_get_error();
}


static bool pin_wrk_state = true;
static bool pin_rdy_state = true;

void dart_uart_open(void) {}
void dart_uart_close(void) {}
void dart_uart_send(uint8_t *buffer, int32_t length)
{
    UNUSED(buffer);
    UNUSED(length);
}
bool dart_pin_get_state(int pin_e)
{
    if (pin_e == DART_RDY_PIN)
        return pin_rdy_state;
    if (pin_e == DART_WRK_PIN)
        return pin_wrk_state;

    return true;
}
void dart_pin_set_state(int pin_e, bool state)
{
    if (pin_e == DART_RDY_PIN)
        pin_rdy_state = state;
    if (pin_e == DART_WRK_PIN)
        pin_wrk_state = state;
}



/**
 * Function simulating receving data
 *
 */
void sim_receive_data(struct dart *drt, uint8_t *data, size_t len)
{
    for (size_t i=0; i<len; i++) {
        dart_handle_received_char(drt, data[i]);
    }
}


struct dart_validator
{
    int code;
    msgtype_t msg_type;
    dart_len_t msg_length;
};

void dart_validator_init(struct dart_validator *self)
{
    self->code = -1;
    self->msg_type = 0;
    self->msg_length = 0;
}


void clbk_validator(int code, void *param1, void *param2, void *private)
{
    struct dart_validator *dv = (struct dart_validator*)private;
    dv->code = code;

    switch (code) {
        case DART_CLBK_TRANSFER_DONE:
        case DART_CLBK_TRANSFER_FAILURE:
        case DART_CLBK_MESSAGE_RECEIVED:
        case DART_CLBK_MESSAGE_ABANDONED: {
            struct msg *msg = (struct msg*)param1;
            size_t msg_len = (size_t)param2;
            dv->msg_type = msg->type;
            dv->msg_length = msg_len;
        }   break;
    }
}


struct test_msg
{
    msgtype_t type;
    uint8_t a;
    uint8_t b;
    uint8_t c;
}
__attribute__((packed));


bool clbk_deferred_msg_content(struct dart *drt, msgtype_t msgtype)
{
    switch (msgtype) {
        case (0x11 | MSG_REPORT):
            break;  // Just send empty message

        case (0x12 | MSG_REPORT): {
            uint8_t payload[] = { 0x01, 0x02 };
            dart_push_msg_payload(drt, msgtype, payload, sizeof(payload));
        }   return true;

        case (0x13 | MSG_REPORT): {
            struct test_msg test_msg;
            test_msg.type = msgtype;
            test_msg.a = 'A';
            test_msg.b = 'B';
            test_msg.c = 'C';
            dart_push_msg(drt, (struct msg*)&test_msg, sizeof(test_msg));
        }   return true;
    }

    return false;
}



#define DART_RX_BUFFER_LEN              128
#define DART_MEMORY_POOL_SIZE           512

static uint8_t dart_rx_buffer[DART_RX_BUFFER_LEN];
static uint8_t dart_memory_pool[DART_MEMORY_POOL_SIZE];



void test_receiving_problems(void)
{
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_validator_init(&dv);
    dart_set_callback(drt, &dv, clbk_validator);

    // Incomplete message
    dart_handle_received_char(drt, DART_SYNC);
    dart_handle_received_char(drt, 0x02);
    CU_ASSERT_FALSE(dart_is_idle(drt))
    clock_update(100, 0);

    // Invalid start
    dart_handle_received_char(drt, 0x00);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_INCOMPLETE);

    // Invalid start codes
    dart_handle_received_char(drt, DART_ACK);
    dart_handle_received_char(drt, DART_BAD);
    dart_handle_received_char(drt, DART_CAN);
    dart_handle_received_char(drt, DART_DONE);
    dart_handle_received_char(drt, DART_EOC);

    dart_handle_transfer_done(drt);

    dart_clean(drt);
}






#if (DART_LEN_SIZE == 1) && (DART_CRC_SIZE == 1)
uint8_t msg_invalid_crc[] = {0x55, 0x01, 0x01, 0xFF};
uint8_t msg_invalid_len[] = {0x55, 0xFF};
uint8_t msg_valid[] = {0x55, 0x02, 0x12, 0x34, 0xD9};
#endif

#if (DART_LEN_SIZE == 2) && (DART_CRC_SIZE == 2)
uint8_t msg_invalid_crc[] = {0x55, 0x00, 0x01, 0x01, 0xFF, 0xFF};
uint8_t msg_invalid_len[] = {0x55, 0x00, 0xFF};
uint8_t msg_valid[] = {0x55, 0x00, 0x02, 0x12, 0x34, 0x0E, 0xC9};
#endif

void clbk_receiving_messages(int code, void *param1, void *param2, void *private)
{
    struct dart_validator *dv = (struct dart_validator*)private;
    dv->code = code;

    if (code == DART_CLBK_MESSAGE_RECEIVED) {
        struct msg *msg = (struct msg*)param1;
        size_t msg_len = (size_t)param2;
        CU_ASSERT_EQUAL(msg_len, 2);
        CU_ASSERT_EQUAL(msg->type, 0x12);
        struct msg_p1 *msg_p1 = (struct msg_p1*)msg;
        CU_ASSERT_EQUAL(msg_p1->param1, 0x34);
    }
}

void test_receiving_messages(void)
{
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_set_callback(drt, &dv, clbk_receiving_messages);

    // Invalid CRC
    dart_validator_init(&dv);
    sim_receive_data(drt, msg_invalid_crc, sizeof(msg_invalid_crc));
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_CORRUPTED);

    // Invalid length
    dart_validator_init(&dv);
    sim_receive_data(drt, msg_invalid_len, sizeof(msg_invalid_len));
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_CORRUPTED);

    // Valid message
    dart_validator_init(&dv);
    sim_receive_data(drt, msg_valid, sizeof(msg_valid));
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_MESSAGE_RECEIVED);

    dart_clean(drt);
}



void test_sending_problems(void)
{
    int ret;
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    CU_ASSERT_TRUE(dart_is_idle(drt))
    CU_ASSERT_FALSE(dart_is_msg_pending(drt, DART_MSG_PRIO_ANY, 0));
    CU_ASSERT_FALSE(dart_get_current_msgtype(drt, NULL));

    struct test_msg msg;
    msg.type = MSG_REPORT;

    // Not running
    drt->running = false;
    ret = dart_send_msg_ex(drt, DART_MSG_PRIO_REPORT, (struct msg*)&msg, sizeof(msg));
    CU_ASSERT_EQUAL(ret, DART_ERR_NOT_POSSIBLE);
    drt->running = true;

    // Invalid priority
    ret = dart_send_msg_ex(drt, 55, (struct msg*)&msg, sizeof(msg));
    CU_ASSERT_EQUAL(ret, DART_ERR_NOT_POSSIBLE);

    // Invalid guessed priority
    msg.type = MSG_REQUEST | MSG_RESPONSE;
    ret = dart_send_msg(drt, (struct msg*)&msg, sizeof(msg));
    CU_ASSERT_EQUAL(ret, DART_ERR_NOT_POSSIBLE);

    // Invalid paload length, second message is not suitable for internal cba allocation buffer
    msg.type = MSG_REPORT;
    msg.a = 11;
    ret = dart_send_msg(drt, (struct msg*)&msg, 250);
    ret = dart_send_msg(drt, (struct msg*)&msg, 250);
    CU_ASSERT_EQUAL(ret, DART_ERR_NO_MEMORY);
    // Confirm first message
    dart_handle_received_char(drt, DART_ACK);


    // Fill entire queue
    ret = dart_send_msg(drt, (struct msg*)&msg, sizeof(msg));
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);
    CU_ASSERT_FALSE(dart_is_idle(drt))
    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_ANY, 0));
    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_REPORT, 0));
    CU_ASSERT_FALSE(dart_is_msg_pending(drt, DART_MSG_PRIO_REQUEST, 0));
    CU_ASSERT_FALSE(dart_is_msg_pending(drt, DART_MSG_PRIO_RESPONSE, 0));

    ret = dart_send_msg(drt, (struct msg*)&msg, sizeof(msg));
    CU_ASSERT_EQUAL(ret, DART_PENDING);
    ret = dart_send_msg(drt, (struct msg*)&msg, sizeof(msg));
    CU_ASSERT_EQUAL(ret, DART_PENDING);

    dart_handle_received_char(drt, DART_ACK);
    dart_handle_received_char(drt, DART_ACK);
    dart_handle_received_char(drt, DART_ACK);

    CU_ASSERT_TRUE(dart_is_idle(drt))

    dart_clean(drt);
}


void test_resending(void)
{
    int ret;
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_validator_init(&dv);
    dart_set_callback(drt, &dv, clbk_validator);

    CU_ASSERT_TRUE(dart_is_idle(drt))

    ret = dart_send_msgtype(drt, 0x11);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);
    ret = dart_send_msgtype(drt, 0x12);
    CU_ASSERT_EQUAL(ret, DART_PENDING);

    clock_update(10000, 0);
    dart_handle_time(drt);
    CU_ASSERT_EQUAL(dv.code, -1);
    clock_update(10000, 0);
    dart_handle_time(drt);
    CU_ASSERT_EQUAL(dv.code, -1);
    clock_update(10000, 0);
    dart_handle_time(drt);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_FAILURE);
    CU_ASSERT_EQUAL(dv.msg_type, 0x11);

    dart_validator_init(&dv);
    clock_update(10000, 0);
    dart_handle_time(drt);
    CU_ASSERT_EQUAL(dv.code, -1);
    clock_update(10000, 0);
    dart_handle_time(drt);
    CU_ASSERT_EQUAL(dv.code, -1);
    clock_update(10000, 0);
    dart_handle_time(drt);
    CU_ASSERT_EQUAL(dv.msg_type, 0x12);
    // DART_CLBK_TRANSFER_COMPLETE replaced DART_CLBK_TRANSFER_FAILURE
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);

    CU_ASSERT_TRUE(dart_is_idle(drt))

    dart_clean(drt);
}





void test_sending_reports(void)
{
    int ret;
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_validator_init(&dv);
    dart_set_callback(drt, &dv, clbk_validator);

    CU_ASSERT_TRUE(dart_is_idle(drt))
    CU_ASSERT_FALSE(dart_is_msg_pending(drt, DART_MSG_PRIO_ANY, 0));

    ret = dart_send_msgtype(drt, 0x11);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);
    ret = dart_send_msgtype(drt, 0x12);
    CU_ASSERT_EQUAL(ret, DART_PENDING);
    ret = dart_send_msgtype(drt, 0x13);
    CU_ASSERT_EQUAL(ret, DART_PENDING);

    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_ANY, 0x11));
    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_ANY, 0x12));
    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_ANY, 0x13));
    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_REPORT, 0x11));
    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_REPORT, 0x12));
    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_REPORT, 0x13));

    msgtype_t current_msgtype;
    bool pending;

    pending = dart_get_current_msgtype(drt, &current_msgtype);
    CU_ASSERT_TRUE(pending);
    CU_ASSERT_EQUAL(current_msgtype, 0x11);

    dart_handle_received_char(drt, DART_ACK);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_DONE);
    CU_ASSERT_EQUAL(dv.msg_type, 0x11);


    pending = dart_get_current_msgtype(drt, &current_msgtype);
    CU_ASSERT_TRUE(pending);
    CU_ASSERT_EQUAL(current_msgtype, 0x12);

    dart_validator_init(&dv);
    dart_handle_received_char(drt, DART_ACK);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_DONE);
    CU_ASSERT_EQUAL(dv.msg_type, 0x12);


    pending = dart_get_current_msgtype(drt, &current_msgtype);
    CU_ASSERT_TRUE(pending);
    CU_ASSERT_EQUAL(current_msgtype, 0x13);

    dart_validator_init(&dv);
    dart_handle_received_char(drt, DART_ACK);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);
    // DART_CLBK_TRANSFER_DONE was received before DART_CLBK_TRANSFER_COMPLETE
    CU_ASSERT_EQUAL(dv.msg_type, 0x13);

    CU_ASSERT_TRUE(dart_is_idle(drt))

    dart_clean(drt);
}



#if (DART_LEN_SIZE == 1) && (DART_CRC_SIZE == 1)
uint8_t msg_report_11[] = {0x55, 0x01, MSG_REPORT | 0x11, 0xEE};
uint8_t msg_request_11[] = {0x55, 0x01, MSG_REQUEST | 0x11, 0xAE};
uint8_t msg_response_11[] = {0x55, 0x01, MSG_RESPONSE | 0x11, 0x6E};
uint8_t msg_response_12[] = {0x55, 0x01, MSG_RESPONSE | 0x12, 0x6D};
#endif

#if (DART_LEN_SIZE == 2) && (DART_CRC_SIZE == 2)
uint8_t msg_report_11[] = {0x55, 0x00, 0x01, MSG_REPORT | 0x11, 0xE3, 0xE0};
uint8_t msg_request_11[] = {0x55, 0x00, 0x01, MSG_REQUEST | 0x11, 0xAB, 0x24};
uint8_t msg_response_11[] = {0x55, 0x00, 0x01, MSG_RESPONSE | 0x11, 0x72, 0x68};
uint8_t msg_response_12[] = {0x55, 0x00, 0x01, MSG_RESPONSE | 0x12, 0x42, 0x0B};
#endif

void test_sending_requests(void)
{
    int ret;
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_validator_init(&dv);
    dart_set_callback(drt, &dv, clbk_validator);

    CU_ASSERT_TRUE(dart_is_idle(drt))
    CU_ASSERT_FALSE(dart_is_msg_pending(drt, DART_MSG_PRIO_ANY, 0));

    ret = dart_send_msgtype(drt, MSG_REQUEST | 0x11);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);
    ret = dart_send_msgtype(drt, MSG_REQUEST | 0x12);
    CU_ASSERT_EQUAL(ret, DART_PENDING);

    dart_handle_received_char(drt, DART_ACK);   // Request was sent, we should wait for response
    CU_ASSERT_EQUAL(dv.code, -1);
    CU_ASSERT_EQUAL(dv.msg_type, 0);

    CU_ASSERT_FALSE(dart_is_idle(drt))
    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_ANY, MSG_REQUEST | 0x11));
    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_ANY, MSG_REQUEST | 0x12));
    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_REQUEST, MSG_REQUEST | 0x11));
    CU_ASSERT_TRUE(dart_is_msg_pending(drt, DART_MSG_PRIO_REQUEST, MSG_REQUEST | 0x12));

    msgtype_t current_msgtype;
    bool pending;

    pending = dart_get_current_msgtype(drt, &current_msgtype);
    CU_ASSERT_TRUE(pending);
    CU_ASSERT_EQUAL(current_msgtype, MSG_REQUEST | 0x11);

    // Not expected response
    sim_receive_data(drt, msg_report_11, sizeof(msg_report_11));
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_MESSAGE_RECEIVED);
    sim_receive_data(drt, msg_response_12, sizeof(msg_response_12));
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_MESSAGE_RECEIVED);

    dart_validator_init(&dv);
    // Expected response
    sim_receive_data(drt, msg_response_11, sizeof(msg_response_11));
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_DONE);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REQUEST | 0x11);

    // Second message should be sent
    pending = dart_get_current_msgtype(drt, &current_msgtype);
    CU_ASSERT_TRUE(pending);
    CU_ASSERT_EQUAL(current_msgtype, MSG_REQUEST | 0x12);

    dart_validator_init(&dv);
    dart_handle_received_char(drt, DART_ACK);   // Request was sent, we should wait for response
    CU_ASSERT_EQUAL(dv.code, -1);
    CU_ASSERT_EQUAL(dv.msg_type, 0);

    // Expected response
    sim_receive_data(drt, msg_response_12, sizeof(msg_response_12));
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REQUEST | 0x12);

    CU_ASSERT_TRUE(dart_is_idle(drt))

    dart_clean(drt);
}


void test_abandon_requests(void)
{
    int ret;
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_validator_init(&dv);
    dart_set_callback(drt, &dv, clbk_validator);

    CU_ASSERT_TRUE(dart_is_idle(drt))

    ret = dart_send_msgtype(drt, MSG_REQUEST | 0x11);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);
    ret = dart_send_msgtype(drt, MSG_REQUEST | 0x12);
    CU_ASSERT_EQUAL(ret, DART_PENDING);

    dart_handle_received_char(drt, DART_ACK);   // Request was sent, we should wait for response
    CU_ASSERT_EQUAL(dv.code, -1);
    CU_ASSERT_EQUAL(dv.msg_type, 0);

    msgtype_t current_msgtype;
    bool pending;

    pending = dart_get_current_msgtype(drt, &current_msgtype);
    CU_ASSERT_TRUE(pending);
    CU_ASSERT_EQUAL(current_msgtype, MSG_REQUEST | 0x11);

    clock_update(10000, 0);
    dart_handle_time(drt);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_MESSAGE_ABANDONED);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REQUEST | 0x11);

    pending = dart_get_current_msgtype(drt, &current_msgtype);
    CU_ASSERT_TRUE(pending);
    CU_ASSERT_EQUAL(current_msgtype, MSG_REQUEST | 0x12);

    dart_validator_init(&dv);
    dart_handle_received_char(drt, DART_ACK);   // Request was sent, we should wait for response
    CU_ASSERT_EQUAL(dv.code, -1);
    CU_ASSERT_EQUAL(dv.msg_type, 0);

    clock_update(10000, 0);
    dart_handle_time(drt);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REQUEST | 0x12);

    CU_ASSERT_TRUE(dart_is_idle(drt))

    dart_clean(drt);
}


void test_sending_reports_while_waiting_for_response(void)
{
    int ret;
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_validator_init(&dv);
    dart_set_callback(drt, &dv, clbk_validator);

    CU_ASSERT_TRUE(dart_is_idle(drt))

    ret = dart_send_msgtype(drt, MSG_REQUEST | 0x11);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);

    dart_handle_received_char(drt, DART_ACK);   // Request was sent, we should wait for response
    CU_ASSERT_EQUAL(dv.code, -1);
    CU_ASSERT_EQUAL(dv.msg_type, 0);

    ret = dart_send_msgtype(drt, MSG_REPORT | 0x11);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);

    msgtype_t current_msgtype;
    bool pending;

    // Pending request has lowest priority
    pending = dart_get_current_msgtype(drt, &current_msgtype);
    CU_ASSERT_TRUE(pending);
    CU_ASSERT_EQUAL(current_msgtype, MSG_REPORT | 0x11);

    dart_handle_received_char(drt, DART_ACK);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_DONE);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REPORT | 0x11);

    pending = dart_get_current_msgtype(drt, &current_msgtype);
    CU_ASSERT_TRUE(pending);
    CU_ASSERT_EQUAL(current_msgtype, MSG_REQUEST | 0x11);


    // New request is not transferred until previous request is pending
    ret = dart_send_msgtype(drt, MSG_REQUEST | 0x12);
    CU_ASSERT_EQUAL(ret, DART_PENDING);

    clock_update(10000, 0);
    dart_handle_time(drt);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_MESSAGE_ABANDONED);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REQUEST | 0x11);

    pending = dart_get_current_msgtype(drt, &current_msgtype);
    CU_ASSERT_TRUE(pending);
    CU_ASSERT_EQUAL(current_msgtype, MSG_REQUEST | 0x12);

    dart_validator_init(&dv);
    dart_handle_received_char(drt, DART_ACK);   // Request was sent, we should wait for response
    CU_ASSERT_EQUAL(dv.code, -1);
    CU_ASSERT_EQUAL(dv.msg_type, 0);

    clock_update(10000, 0);
    dart_handle_time(drt);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REQUEST | 0x12);

    CU_ASSERT_TRUE(dart_is_idle(drt))

    dart_clean(drt);
}


void test_idle_scenario(void)
{
    int ret;
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_validator_init(&dv);
    dart_set_callback(drt, &dv, clbk_validator);

    dart_pin_set_state(DART_WRK_PIN, false);
    dart_pin_set_state(DART_RDY_PIN, false);

    CU_ASSERT_TRUE(dart_is_idle(drt))
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_IDLE);

    // Simulate state after transferring all messages
    dart_pin_set_state(DART_WRK_PIN, true);
    dart_pin_set_state(DART_RDY_PIN, true);

    ret = dart_handle_time(drt);
    // Expect closing state
    CU_ASSERT_EQUAL(ret, DART_IDLE);
    CU_ASSERT_TRUE(dart_is_idle(drt))
    CU_ASSERT_TRUE(dart_pin_get_state(DART_WRK_PIN));
    clock_update(100, 0);
    ret = dart_handle_time(drt);
    // Expect chill state
    CU_ASSERT_EQUAL(ret, DART_IDLE);
    CU_ASSERT_TRUE(dart_is_idle(drt))
    CU_ASSERT_FALSE(dart_pin_get_state(DART_WRK_PIN));

    clock_update(300, 0);
    dart_pin_set_state(DART_RDY_PIN, false);
    ret = dart_handle_time(drt);
    // Expect idle notification
    CU_ASSERT_EQUAL(ret, DART_IDLE);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_IDLE);

    dart_clean(drt);
}


void test_waiting_scenario(void)
{
    int ret;
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_validator_init(&dv);
    dart_set_callback(drt, &dv, clbk_validator);

    dart_pin_set_state(DART_WRK_PIN, false);
    dart_pin_set_state(DART_RDY_PIN, false);

    // Send request
    ret = dart_send_msgtype(drt, MSG_REPORT | 0x11);
    CU_ASSERT_EQUAL(ret, DART_WAITING);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_WAITING);
    CU_ASSERT_TRUE(dart_pin_get_state(DART_WRK_PIN));

    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_WAITING);
    clock_update(500, 0);
    ret = dart_handle_time(drt);
    // Expect WRK pin spike to wakeup receiver
    CU_ASSERT_EQUAL(ret, DART_WAITING);
    CU_ASSERT_FALSE(dart_pin_get_state(DART_WRK_PIN));
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_WAITING);
    CU_ASSERT_TRUE(dart_pin_get_state(DART_WRK_PIN));
    clock_update(500, 0);
    ret = dart_handle_time(drt);
    // Expect WRK pin spike to wakeup receiver
    CU_ASSERT_EQUAL(ret, DART_WAITING);
    CU_ASSERT_FALSE(dart_pin_get_state(DART_WRK_PIN));
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_WAITING);
    CU_ASSERT_TRUE(dart_pin_get_state(DART_WRK_PIN));
    clock_update(500, 0);
    ret = dart_handle_time(drt);
    // Expect WRK pin spike to wakeup receiver
    CU_ASSERT_EQUAL(ret, DART_WAITING);
    CU_ASSERT_FALSE(dart_pin_get_state(DART_WRK_PIN));
    ret = dart_handle_time(drt);
    // Expect error
    CU_ASSERT_EQUAL(ret, DART_ERR_NOT_POSSIBLE);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_IMPOSSIBLE);
    CU_ASSERT_FALSE(dart_pin_get_state(DART_WRK_PIN));

    ret = dart_handle_time(drt);
    // Expect waiting again
    CU_ASSERT_EQUAL(ret, DART_WAITING);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_WAITING);
    CU_ASSERT_TRUE(dart_pin_get_state(DART_WRK_PIN));

    dart_pin_set_state(DART_RDY_PIN, true);
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);

    dart_handle_received_char(drt, DART_ACK);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REPORT | 0x11);

    CU_ASSERT_TRUE(dart_is_idle(drt))

    dart_clean(drt);
}


void test_sending_scenario(void)
{
    int ret;
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_validator_init(&dv);
    dart_set_callback(drt, &dv, clbk_validator);

    dart_pin_set_state(DART_WRK_PIN, false);
    dart_pin_set_state(DART_RDY_PIN, false);

    // Send request
    ret = dart_send_msgtype(drt, MSG_REQUEST | 0x11);
    CU_ASSERT_EQUAL(ret, DART_WAITING);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_WAITING);
    CU_ASSERT_TRUE(dart_pin_get_state(DART_WRK_PIN));
    dart_pin_set_state(DART_RDY_PIN, true);
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);


    dart_handle_received_char(drt, DART_ACK);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_WAITING);
    CU_ASSERT_EQUAL(dv.msg_type, 0);

    dart_validator_init(&dv);
    // Receive expected response
    sim_receive_data(drt, msg_response_11, sizeof(msg_response_11));
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REQUEST | 0x11);

    CU_ASSERT_TRUE(dart_is_idle(drt))

    dart_validator_init(&dv);

    // Start closing procedure
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_IDLE);
    clock_update(100, 0);
    ret = dart_handle_time(drt);
    CU_ASSERT_FALSE(dart_pin_get_state(DART_WRK_PIN));
    CU_ASSERT_EQUAL(ret, DART_IDLE);
    clock_update(300, 0);
    dart_pin_set_state(DART_RDY_PIN, false);
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_IDLE);

    // Test time handler when idle
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_IDLE);

    // Pretend that external module is not ready
    dart_pin_set_state(DART_RDY_PIN, false);
    dart_pin_set_state(DART_WRK_PIN, false);
    // Send request
    ret = dart_send_msgtype(drt, MSG_REPORT | 0x11);
    CU_ASSERT_EQUAL(ret, DART_WAITING);
    CU_ASSERT_TRUE(dart_pin_get_state(DART_WRK_PIN));
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_WAITING);

    CU_ASSERT_FALSE(dart_is_idle(drt));

    // External module is ready now
    dart_pin_set_state(DART_RDY_PIN, true);
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);

    dart_handle_received_char(drt, DART_ACK);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);
    // Before transfer complete, the transfer done was notified
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REPORT | 0x11);

    dart_clean(drt);
}


void test_receiving_scenario(void)
{
    int ret;
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_validator_init(&dv);
    dart_set_callback(drt, &dv, clbk_validator);

    CU_ASSERT_TRUE(dart_is_idle(drt))
    dart_pin_set_state(DART_RDY_PIN, true);
    dart_pin_set_state(DART_WRK_PIN, false);

    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_PENDING);
    CU_ASSERT_TRUE(dart_pin_get_state(DART_WRK_PIN));

    // Receive request
    sim_receive_data(drt, msg_request_11, sizeof(msg_request_11));
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_MESSAGE_RECEIVED);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REQUEST | 0x11);

    dart_validator_init(&dv);
    // Send response
    ret = dart_send_msgtype(drt, MSG_RESPONSE | 0x11);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);
    dart_handle_received_char(drt, DART_ACK);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_RESPONSE | 0x11);
    dart_handle_received_char(drt, DART_DONE);

    ret = dart_handle_time(drt);
    dart_validator_init(&dv);
    // Closing should be pending now
    clock_update(50, 0);
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_IDLE);
    CU_ASSERT_TRUE(dart_is_idle(drt))
    CU_ASSERT_FALSE(dart_pin_get_state(DART_WRK_PIN));
    // Chill
    clock_update(100, 0);
    dart_pin_set_state(DART_RDY_PIN, false);
    ret = dart_handle_time(drt);
    CU_ASSERT_EQUAL(ret, DART_IDLE);
    CU_ASSERT_TRUE(dart_is_idle(drt))
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_IDLE);

    // Received data while not ready
    dart_handle_received_char(drt, DART_ACK);

    dart_clean(drt);
}


void test_deferred_msg_content(void)
{
    int ret;
    struct dart _drt;
    struct dart *drt = &_drt;
    dart_init(drt, dart_memory_pool, sizeof(dart_memory_pool), dart_rx_buffer, sizeof(dart_rx_buffer));

    struct dart_validator dv;
    dart_validator_init(&dv);
    dart_set_callback(drt, &dv, clbk_validator);
    dart_set_deferred_msg_callback(drt, clbk_deferred_msg_content);

    dart_pin_set_state(DART_RDY_PIN, true);
    dart_pin_set_state(DART_WRK_PIN, true);

    ret = dart_send_msgtype(drt, MSG_REPORT | 0x11);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);
    dart_handle_received_char(drt, DART_ACK);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REPORT | 0x11);
    dart_handle_received_char(drt, DART_DONE);

    ret = dart_send_msgtype(drt, MSG_REPORT | 0x12);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);
    dart_handle_received_char(drt, DART_ACK);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REPORT | 0x12);
    dart_handle_received_char(drt, DART_DONE);

    ret = dart_send_msgtype(drt, MSG_REPORT | 0x13);
    CU_ASSERT_EQUAL(ret, DART_SUCCESS);
    dart_handle_received_char(drt, DART_ACK);
    CU_ASSERT_EQUAL(dv.code, DART_CLBK_TRANSFER_COMPLETE);
    CU_ASSERT_EQUAL(dv.msg_type, MSG_REPORT | 0x13);
    dart_handle_received_char(drt, DART_DONE);

    CU_ASSERT_TRUE(dart_is_idle(drt));

    dart_clean(drt);
}
