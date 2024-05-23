
#include <CUnit/Basic.h>

#include "mx/core/process.h"
#include "mx/core/process-timer.h"
#include "mx/core/message.h"
#include "mx/timer.h"
#include "mx/misc.h"

#include <stdio.h>



static void test_process_start(void);
static void test_process_exit(void);
static void test_process_poll(void);
static void test_process_broadcast(void);
static void test_process_post_synch_msg(void);
static void test_process_post_alloc_msg(void);
static void test_process_send_msg(void);
static void test_process_timer(void);
static void test_process_problems(void);


CU_ErrorCode cu_test_process()
{
    // Test logging to terminal
    CU_pSuite suite = CU_add_suite("Test process module", NULL, NULL);
    if ( !suite ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_add_test(suite, "Test start process",                    test_process_start);
    CU_add_test(suite, "Test exit process",                     test_process_exit);
    CU_add_test(suite, "Test poll process",                     test_process_poll);
    CU_add_test(suite, "Test broadcast msg",                    test_process_broadcast);
    CU_add_test(suite, "Test post synch msg to process",        test_process_post_synch_msg);
    CU_add_test(suite, "Test post allocated msg to process",    test_process_post_alloc_msg);
    CU_add_test(suite, "Test send msg to process",              test_process_send_msg);
    CU_add_test(suite, "Test process timers",                   test_process_timer);
    CU_add_test(suite, "Test process problems",                 test_process_problems);


    return CU_get_error();
}





struct mx_msg_custom
{
    msgtype_t type;
    uint8_t p8;
    uint16_t p16;
    uint32_t p32;
} __attribute__((packed));


#define MSG_CUSTOM_P8   0x5A
#define MSG_CUSTOM_P16  0x03CF
#define MSG_CUSTOM_P32  0xDEADBEEF



struct msg_p4 last_msg;

enum test_msgtype_e
{
    TEST_EV_EMPTY = 0x20,
    TEST_EV_CUSTOM,
    TEST_EV_DATA,
    TEST_EV_FORWARD,
    TEST_EV_EXIT,
    TEST_EV_POLL,
    TEST_EV_BROADCAST,
    TEST_EV_P0 = 0x30,
    TEST_EV_P1,
    TEST_EV_P2,
    TEST_EV_P3,
    TEST_EV_P4,
};



// Process declaration
PROCESS_NAME(proc1);
// Process structure
PROCESS(proc1, "PROC1");
// Process thread
PROCESS_THREAD(proc1, ev, msg)
{
    struct msg_p4 *real_msg = (struct msg_p4*)msg;

    PROCESS_BEGIN();

    while (1) {
        last_msg.type = ev;
        if (ev == PROCESS_EV_EXIT)
            break;
        if (ev == TEST_EV_POLL) {
            process_poll(self);
        }
        if (ev == TEST_EV_FORWARD) {
            process_send_msg_p0(self, TEST_EV_P0);
            process_send_msg_p1(self, TEST_EV_P1, 0);
            process_send_msg_p2(self, TEST_EV_P2, 0, 0);
            process_send_msg_p3(self, TEST_EV_P3, 0, 0, 0);
            process_send_msg_p4(self, TEST_EV_P4, 0, 0, 0, 0);
        }
        if (ev == TEST_EV_CUSTOM) {
            struct mx_msg_custom *custom_msg = (struct mx_msg_custom*)msg;
            CU_ASSERT_EQUAL(custom_msg->p8, MSG_CUSTOM_P8);
            CU_ASSERT_EQUAL(custom_msg->p16, MSG_CUSTOM_P16);
            CU_ASSERT_EQUAL(custom_msg->p32, MSG_CUSTOM_P32);
        }
        if (ev >= TEST_EV_P1)
            last_msg.param1 = real_msg->param1;
        if (ev >= TEST_EV_P2)
            last_msg.param2 = real_msg->param2;
        if (ev >= TEST_EV_P3)
            last_msg.param3 = real_msg->param3;
        if (ev >= TEST_EV_P4)
            last_msg.param4 = real_msg->param4;

        PROCESS_YIELD();
    }

    PROCESS_END();
    return PT_ENDED;
}


// Process declaration
PROCESS_NAME(proc2);
// Process structure
PROCESS(proc2, "PROC2");
// Process thread
PROCESS_THREAD(proc2, ev, msg)
{
    UNUSED(msg);
    PROCESS_BEGIN();

    while (1) {
        if (ev == TEST_EV_EXIT)
            break;

        PROCESS_YIELD();
    }

    PROCESS_END();
    return PT_ENDED;
}










void test_process_start(void)
{
    uint32_t process_buffer[256];

    process_init(process_buffer, sizeof(process_buffer));
    process_start(&proc1);
    CU_ASSERT_EQUAL(last_msg.type, PROCESS_EV_INIT);
    last_msg.type = 0;

    // Outside of any processes
    struct process *current = process_get_current();
    CU_ASSERT_PTR_NULL(current);

    // Start process again
    process_start(&proc1);
    CU_ASSERT_EQUAL(last_msg.type, 0);      // Should be ignored

    // Send initialization event
    proc1.state = PROCESS_STATE_NONE;
    process_send_msg_p0(&proc1, PROCESS_EV_INIT);
    process_run();
    CU_ASSERT_EQUAL(proc1.state, PROCESS_STATE_RUNNING);

    process_exit(&proc1);
}


void test_process_exit(void)
{
    uint32_t process_buffer[256];

    process_init(process_buffer, sizeof(process_buffer));
    process_start(&proc1);

    process_start(&proc2);
    process_exit(&proc2);
    CU_ASSERT_EQUAL(last_msg.type, PROCESS_EV_FINISHED);
    last_msg.type = 0;

    // Exit not started process
    process_exit(&proc2);
    CU_ASSERT_EQUAL(last_msg.type, 0);

    process_exit(&proc1);

    // Change processes order
    process_start(&proc2);
    process_start(&proc1);
    process_send_msg_p0(&proc2, TEST_EV_EXIT);
    process_run();
    CU_ASSERT_EQUAL(last_msg.type, PROCESS_EV_FINISHED);

    process_exit(&proc1);
}


void test_process_poll(void)
{
    uint32_t process_buffer[256];

    process_init(process_buffer, sizeof(process_buffer));
    process_start(&proc1);
    last_msg.type = 0;

    process_poll(&proc1);
    CU_ASSERT_EQUAL(last_msg.type, 0);
    process_run();
    CU_ASSERT_EQUAL(last_msg.type, PROCESS_EV_POLL);
    last_msg.type = 0;

    process_exit(&proc1);
}


void test_process_broadcast(void)
{
    uint32_t process_buffer[256];

    process_init(process_buffer, sizeof(process_buffer));
    process_start(&proc2);
    process_start(&proc1);
    last_msg.type = 0;

    process_send_msg_p0(NULL, TEST_EV_BROADCAST);
    CU_ASSERT_EQUAL(last_msg.type, 0);
    process_run();
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_BROADCAST);

    process_exit(&proc1);
    process_exit(&proc2);
}


void test_process_post_synch_msg(void)
{
    unsigned int events;
    uint32_t process_buffer[256];

    process_init(process_buffer, sizeof(process_buffer));
    process_start(&proc1);

    struct msg tmp_msg;

    tmp_msg.type = TEST_EV_EMPTY;
    process_handle_msg(&proc1, &tmp_msg);
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_EMPTY);
    events = process_events();
    CU_ASSERT_EQUAL(events, 0);

    tmp_msg.type = TEST_EV_P0;
    process_handle_msg(&proc1, &tmp_msg);
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_P0);
    events = process_events();
    CU_ASSERT_EQUAL(events, 0);

    process_handle_msg_p0(&proc1, TEST_EV_EMPTY);
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_EMPTY);
    events = process_events();
    CU_ASSERT_EQUAL(events, 0);

    process_exit(&proc1);
}


void test_process_post_alloc_msg(void)
{
    unsigned int events;
    uint32_t process_buffer[256];

    process_init(process_buffer, sizeof(process_buffer));
    process_start(&proc1);
    last_msg.type = 0;

    // Post allocated message
    struct msg *tmp_msg;

    // Message oversized
    tmp_msg = process_malloc(sizeof(process_buffer));
    CU_ASSERT_PTR_NULL(tmp_msg);

    // Free allocated message
    tmp_msg = process_malloc(sizeof(struct msg));
    CU_ASSERT_PTR_NOT_NULL(tmp_msg);
    process_free(tmp_msg);

    // Send allocated message
    tmp_msg = process_malloc(sizeof(struct msg));
    CU_ASSERT_PTR_NOT_NULL(tmp_msg);
    tmp_msg->type = TEST_EV_EMPTY;
    events = process_events();
    CU_ASSERT_EQUAL(events, 0);
    process_post_msg(&proc1, tmp_msg);
    CU_ASSERT_EQUAL(last_msg.type, 0);      // Message is not delivered yet
    events = process_events();
    CU_ASSERT_EQUAL(events, 1);             // Message is queued
    events = process_run();
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_EMPTY);
    events = process_events();
    CU_ASSERT_EQUAL(events, 0);

    process_exit(&proc1);
}


void test_process_send_msg(void)
{
    unsigned int events;
    uint32_t process_buffer[256];

    process_init(process_buffer, sizeof(process_buffer));
    process_start(&proc1);
    last_msg.type = 0;

    events = process_events();
    CU_ASSERT_EQUAL(events, 0);
    process_send_msg_p0(&proc1, TEST_EV_P0);
    events = process_events();
    CU_ASSERT_EQUAL(events, 1);
    process_send_msg_p1(&proc1, TEST_EV_P1, 0x11);
    events = process_events();
    CU_ASSERT_EQUAL(events, 2);
    process_send_msg_p2(&proc1, TEST_EV_P2, 0x21, 0x22);
    events = process_events();
    CU_ASSERT_EQUAL(events, 3);
    process_send_msg_p3(&proc1, TEST_EV_P3, 0x31, 0x32, 0x33);
    events = process_events();
    CU_ASSERT_EQUAL(events, 4);
    process_send_msg_p4(&proc1, TEST_EV_P4, 0x41, 0x42, 0x43, 0x44);
    events = process_events();
    CU_ASSERT_EQUAL(events, 5);

    uint8_t data = 0xDA;
    process_send_msg_data(&proc1, TEST_EV_DATA, &data, 1);
    events = process_events();
    CU_ASSERT_EQUAL(events, 6);

    struct msg msg = {.type = TEST_EV_EMPTY};
    process_send_msg(&proc1, &msg, sizeof(msg));
    events = process_events();
    CU_ASSERT_EQUAL(events, 7);

    events = process_run();
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_P0);
    CU_ASSERT_EQUAL(events, 6);

    events = process_run();
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_P1);
    CU_ASSERT_EQUAL(last_msg.param1, 0x11);
    CU_ASSERT_EQUAL(events, 5);

    events = process_run();
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_P2);
    CU_ASSERT_EQUAL(last_msg.param1, 0x21);
    CU_ASSERT_EQUAL(last_msg.param2, 0x22);
    CU_ASSERT_EQUAL(events, 4);

    events = process_run();
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_P3);
    CU_ASSERT_EQUAL(last_msg.param1, 0x31);
    CU_ASSERT_EQUAL(last_msg.param2, 0x32);
    CU_ASSERT_EQUAL(last_msg.param3, 0x33);
    CU_ASSERT_EQUAL(events, 3);

    events = process_run();
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_P4);
    CU_ASSERT_EQUAL(last_msg.param1, 0x41);
    CU_ASSERT_EQUAL(last_msg.param2, 0x42);
    CU_ASSERT_EQUAL(last_msg.param3, 0x43);
    CU_ASSERT_EQUAL(last_msg.param4, 0x44);
    CU_ASSERT_EQUAL(events, 2);

    events = process_run();
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_DATA);
    CU_ASSERT_EQUAL(events, 1);

    events = process_run();
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_EMPTY);
    CU_ASSERT_EQUAL(events, 0);

    process_exit(&proc1);
}


void test_process_timer(void)
{
    unsigned int events;
    uint32_t process_buffer[256];

    process_init(process_buffer, sizeof(process_buffer));
    process_start(&proc1);
    last_msg.type = 0;

    events = process_events();
    CU_ASSERT_EQUAL(events, 0);

    clock_update(100, 0);
    CU_ASSERT_EQUAL(timer_tstamp(TIMER_SEC), timer_tstamp(TIMER_CHRONO));

    struct process_timer proc_timer1;
    struct process_timer proc_timer2;

    process_timer_start(&proc_timer1, &proc1, 100, TEST_EV_P1);
    // Start the same timer, should be ignored
    process_timer_start(&proc_timer1, &proc1, 100, TEST_EV_P1);
    process_timer_start(&proc_timer2, PROCESS_BROADCAST, 200, TEST_EV_P2);
    // Time not expired
    process_timer_handler();
    CU_ASSERT_EQUAL(last_msg.type, 0);
    // Update time
    clock_update(100, 0);
    process_timer_handler();
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_P1);
    // Update time
    clock_update(100, 0);
    process_timer_handler();
    CU_ASSERT_EQUAL(last_msg.type, TEST_EV_P2);


    // Stop timers
    process_timer_start(&proc_timer1, &proc1, 100, TEST_EV_P1);
    process_timer_start(&proc_timer2, &proc1, 100, TEST_EV_P2);
    process_timer_stop(&proc_timer1);
    process_timer_stop(&proc_timer2);
    clock_update(100, 0);
    last_msg.type = 0;
    process_timer_handler();
    CU_ASSERT_EQUAL(last_msg.type, 0);

    process_exit(&proc1);
}


void test_process_problems(void)
{
    int status;
    uint32_t process_buffer[4];

    process_init(process_buffer, sizeof(process_buffer));

    status = process_send_msg(&proc1, NULL, sizeof(process_buffer));
    CU_ASSERT_EQUAL(status, PROCESS_ERR_NO_MEMORY);

    status = process_send_msg_p0(&proc1, TEST_EV_EMPTY);
    CU_ASSERT_EQUAL(status, PROCESS_ERR_NO_MEMORY);
    status = process_send_msg_p1(&proc1, TEST_EV_EMPTY, 0);
    CU_ASSERT_EQUAL(status, PROCESS_ERR_NO_MEMORY);
    status = process_send_msg_p2(&proc1, TEST_EV_EMPTY, 0, 0);
    CU_ASSERT_EQUAL(status, PROCESS_ERR_NO_MEMORY);
    status = process_send_msg_p3(&proc1, TEST_EV_EMPTY, 0, 0, 0);
    CU_ASSERT_EQUAL(status, PROCESS_ERR_NO_MEMORY);
    status = process_send_msg_p4(&proc1, TEST_EV_EMPTY, 0, 0, 0, 0);
    CU_ASSERT_EQUAL(status, PROCESS_ERR_NO_MEMORY);

    status = process_send_msg_data(&proc1, TEST_EV_EMPTY, NULL, sizeof(process_buffer));
    CU_ASSERT_EQUAL(status, PROCESS_ERR_NO_MEMORY);
}
