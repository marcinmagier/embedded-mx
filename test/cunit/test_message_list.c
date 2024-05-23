
#include <CUnit/Basic.h>

#include "mx/core/message-list.h"
#include "mx/cba.h"
#include "mx/misc.h"

#include <stdio.h>
#include <stdint.h>


static void test_msg_list_remove(void);



CU_ErrorCode cu_test_message_list()
{
    // Test logging to terminal
    CU_pSuite suite = CU_add_suite("Test message list", NULL, NULL);
    if ( !suite ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_add_test(suite, "Test removing msg from list",       test_msg_list_remove);

    return CU_get_error();
}


#define CBA_BUFFER_LEN      256

static uint8_t cba_buffer[CBA_BUFFER_LEN];



void test_msg_list_remove(void)
{
    struct cba cba;
    struct msg_list mlist;

    cba_init(&cba, cba_buffer, sizeof(cba_buffer));

    msg_list_init(&mlist);

    struct msg_ptr *m1 = msg_ptr_malloc(&cba, 1);
    struct msg_ptr *m2 = msg_ptr_malloc(&cba, 1);
    struct msg_ptr *m3 = msg_ptr_malloc(&cba, 1);
    struct msg_ptr *m4 = msg_ptr_malloc(&cba, 1);

    msg_list_push(&mlist, m1);
    msg_list_push(&mlist, m2);
    msg_list_push(&mlist, m3);
    msg_list_push(&mlist, m4);
    CU_ASSERT_EQUAL(4, msg_list_length(&mlist));

    struct msg_ptr *tmp;

    tmp = msg_list_remove(&mlist, m3);
    CU_ASSERT_PTR_NOT_NULL(tmp);
    CU_ASSERT_PTR_EQUAL(m3, tmp);
    CU_ASSERT_EQUAL(3, msg_list_length(&mlist));

    tmp = msg_list_remove(&mlist, m4);
    CU_ASSERT_PTR_NOT_NULL(tmp);
    CU_ASSERT_PTR_EQUAL(m4, tmp);
    CU_ASSERT_EQUAL(2, msg_list_length(&mlist));

    tmp = msg_list_remove(&mlist, m1);
    CU_ASSERT_PTR_NOT_NULL(tmp);
    CU_ASSERT_PTR_EQUAL(m1, tmp);
    CU_ASSERT_EQUAL(1, msg_list_length(&mlist));

    tmp = msg_list_remove(&mlist, m2);
    CU_ASSERT_PTR_NOT_NULL(tmp);
    CU_ASSERT_PTR_EQUAL(m2, tmp);
    CU_ASSERT_EQUAL(0, msg_list_length(&mlist));

    tmp = msg_list_remove(&mlist, m1);
    CU_ASSERT_PTR_NULL(tmp);
}
