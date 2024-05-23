
#include "mx/cba.h"

#include <CUnit/Basic.h>

#include <stdio.h>


static void test_allocation_problems(void);
static void test_allocation(void);
static void test_reusing_memory(void);


CU_ErrorCode cu_test_cba()
{
    // Test logging to terminal
    CU_pSuite suite = CU_add_suite("Test cba module", NULL, NULL);
    if ( !suite ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_add_test(suite, "Test allocation problems",          test_allocation_problems);
    CU_add_test(suite, "Test simple allocation",            test_allocation);

    CU_add_test(suite, "Test reusing memory",               test_reusing_memory);


    return CU_get_error();
}


uint8_t buffer[50];



void test_allocation_problems(void)
{
    void *chunk;
    struct cba cba;

    cba_init(&cba, buffer, sizeof(buffer));

    // No memory
    chunk = cba_malloc(&cba, sizeof(buffer));
    CU_ASSERT_PTR_NULL(chunk);

    cba_clean(&cba);
}


void test_allocation(void)
{
    void *chunk1;
    void *chunk2;
    void *tmp;

    struct cba cba;
    cba_init(&cba, buffer, sizeof(buffer));

    // Free NULL pointer
    cba_free(&cba, NULL);

    chunk1 = cba_malloc(&cba, 25);
    CU_ASSERT_PTR_NOT_NULL(chunk1);
    chunk2 = cba_malloc(&cba, 10);
    CU_ASSERT_PTR_NOT_NULL(chunk2);
    // No more memory
    tmp = cba_malloc(&cba, 5);
    CU_ASSERT_PTR_NULL(tmp);
    // Release memory
    cba_free(&cba, chunk1);
    tmp = cba_malloc(&cba, 5);
    CU_ASSERT_PTR_NOT_NULL(tmp);
    tmp = cba_malloc(&cba, 5);
    CU_ASSERT_PTR_NOT_NULL(tmp);
    // No more memory
    tmp = cba_malloc(&cba, 5);
    CU_ASSERT_PTR_NULL(tmp);


}


void test_reusing_memory(void)
{
    void *chunk1;
    void *chunk2;

    struct cba cba;
    cba_init(&cba, buffer, sizeof(buffer));

    chunk1 = cba_malloc(&cba, 1);
    CU_ASSERT_PTR_NOT_NULL(chunk1);
    cba_free(&cba, chunk1);
    chunk2 = cba_malloc(&cba, 1);
    CU_ASSERT_PTR_NOT_NULL(chunk2);
    // Memory should be reused, after free
    CU_ASSERT_EQUAL(chunk1, chunk2);

    cba_reset(&cba);

    chunk1 = cba_malloc(&cba, 1);
    CU_ASSERT_PTR_NOT_NULL(chunk1);
    cba_reset(&cba);
    chunk2 = cba_malloc(&cba, 1);
    CU_ASSERT_PTR_NOT_NULL(chunk2);
    // Memory should be reused, after reset
    CU_ASSERT_EQUAL(chunk1, chunk2);

    cba_clean(&cba);
}

