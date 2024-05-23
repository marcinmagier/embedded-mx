
#include <CUnit/Basic.h>

#include "mx/core/message-queue.h"
#include "mx/cba.h"
#include "mx/misc.h"

#include <stdio.h>
#include <stdint.h>


static void test_msg_queue_basic(void);



CU_ErrorCode cu_test_message_queue()
{
    // Test logging to terminal
    CU_pSuite suite = CU_add_suite("Test message queue", NULL, NULL);
    if ( !suite ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_add_test(suite, "Test msg queue basic operations",   test_msg_queue_basic);

    return CU_get_error();
}


#define CBA_BUFFER_LEN      256

static uint8_t cba_buffer[CBA_BUFFER_LEN];




void test_msg_queue_basic(void)
{
    struct cba cba;
    struct msg_queue mqueue;

    cba_init(&cba, cba_buffer, sizeof(cba_buffer));

    msg_queue_init(&mqueue);

    struct msg_ptr *m1 = msg_ptr_malloc(&cba, 1);
    struct msg_ptr *m2 = msg_ptr_malloc(&cba, 1);
    struct msg_ptr *m3 = msg_ptr_malloc(&cba, 1);
    struct msg_ptr *m4 = msg_ptr_malloc(&cba, 1);

    msg_queue_push(&mqueue, MSG_PRIO_HIGH, m1);
    msg_queue_push(&mqueue, MSG_PRIO_NORMAL, m2);
    msg_queue_push(&mqueue, MSG_PRIO_LOW, m3);
    msg_queue_push(&mqueue, MSG_PRIO_NORMAL, m4);
    CU_ASSERT_EQUAL(4, msg_queue_length(&mqueue));

    struct msg_ptr *tmp;
    struct msg_list *list;
    uint8_t prio;

    list = msg_queue_get_msg_list(&mqueue, MSG_PRIO_ANY);
    CU_ASSERT_PTR_NOT_NULL(list);

    tmp = msg_queue_peek(&mqueue, &prio);
    CU_ASSERT_PTR_NOT_NULL(tmp);
    CU_ASSERT_EQUAL(tmp, m1);
    CU_ASSERT_EQUAL(prio, MSG_PRIO_HIGH);
    tmp = msg_queue_pop(&mqueue, &prio);
    CU_ASSERT_PTR_NOT_NULL(tmp);
    CU_ASSERT_EQUAL(tmp, m1);
    CU_ASSERT_EQUAL(prio, MSG_PRIO_HIGH);

    tmp = msg_queue_pop(&mqueue, &prio);
    CU_ASSERT_PTR_NOT_NULL(tmp);
    CU_ASSERT_EQUAL(tmp, m2);
    CU_ASSERT_EQUAL(prio, MSG_PRIO_NORMAL);

    tmp = msg_queue_pop(&mqueue, &prio);
    CU_ASSERT_PTR_NOT_NULL(tmp);
    CU_ASSERT_EQUAL(tmp, m4);
    CU_ASSERT_EQUAL(prio, MSG_PRIO_NORMAL);

    tmp = msg_queue_pop(&mqueue, &prio);
    CU_ASSERT_PTR_NOT_NULL(tmp);
    CU_ASSERT_EQUAL(tmp, m3);
    CU_ASSERT_EQUAL(prio, MSG_PRIO_LOW);

    tmp = msg_queue_pop(&mqueue, &prio);
    CU_ASSERT_PTR_NULL(tmp);
    tmp = msg_queue_peek(&mqueue, &prio);
    CU_ASSERT_PTR_NULL(tmp);
    list = msg_queue_get_msg_list(&mqueue, MSG_PRIO_ANY);
    CU_ASSERT_PTR_NULL(list);

    tmp = msg_queue_push(&mqueue, MSG_PRIO_ANY, NULL);
    CU_ASSERT_PTR_NULL(tmp);
}
