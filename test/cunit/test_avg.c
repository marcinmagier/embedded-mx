
#include <CUnit/Basic.h>

#include "mx/lib/avg.h"
#include "mx/misc.h"

#include <stdio.h>
#include <stdint.h>


static void test_average_basic(void);
static void test_average_ceil(void);
static void test_average_flor(void);
static void test_average_minima(void);
static void test_average_maxima(void);
static void test_average_extrema(void);


CU_ErrorCode cu_test_avg()
{
    // Test logging to terminal
    CU_pSuite suite = CU_add_suite("Test AVG module", NULL, NULL);
    if ( !suite ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_add_test(suite, "Test average basic",                test_average_basic);
    CU_add_test(suite, "Test average ceil",                 test_average_ceil);
    CU_add_test(suite, "Test average flor",                 test_average_flor);
    CU_add_test(suite, "Test average with minima",          test_average_minima);
    CU_add_test(suite, "Test average with maxima",          test_average_maxima);
    CU_add_test(suite, "Test average with extrema",         test_average_extrema);


    return CU_get_error();
}



uint32_t elements_flor[] = {99, 32, 76, 84, 93, 22, 1, 68, 90, 59};
uint32_t elements_ceil[] = {99, 32, 76, 84, 93, 22, 1, 68, 90, 89};

uint32_t elements[] = {9, 97, 15, 6, 3, 92, 1, 99, 32, 75, 82, 64, 95};


void test_average_basic(void)
{
    struct avg avg;
    bool status;
    uint32_t value;

    avg_init(&avg, NULL, 0, NULL, 0);
    status = avg_ready(&avg);
    CU_ASSERT_FALSE(status);
    value = avg_calculate(&avg);
    CU_ASSERT_EQUAL(value, 0);

    avg_add(&avg, 50);
    status = avg_ready(&avg);
    CU_ASSERT_TRUE(status);
    value = avg_calculate(&avg);
    CU_ASSERT_EQUAL(value, 50);

    avg_add(&avg, 50);
    status = avg_ready(&avg);
    CU_ASSERT_TRUE(status);
    value = avg_calculate(&avg);
    CU_ASSERT_EQUAL(value, 50);

}


void test_average_ceil(void)
{
    struct avg avg;
    bool status;
    uint32_t value;

    avg_init(&avg, NULL, 0, NULL, 0);

    for (unsigned int i=0; i<ARRAY_SIZE(elements_ceil); i++)
        avg_add(&avg, elements_ceil[i]);

    status = avg_ready(&avg);
    CU_ASSERT_TRUE(status);
    value = avg_calculate(&avg);
    CU_ASSERT_EQUAL(value, 65);
}


void test_average_flor(void)
{
    struct avg avg;
    bool status;
    uint32_t value;

    avg_init(&avg, NULL, 0, NULL, 0);

    for (unsigned int i=0; i<ARRAY_SIZE(elements_flor); i++)
        avg_add(&avg, elements_flor[i]);

    status = avg_ready(&avg);
    CU_ASSERT_TRUE(status);
    value = avg_calculate(&avg);
    CU_ASSERT_EQUAL(value, 62);
}


void test_average_minima(void)
{
    uint32_t avg_minima[3];
    struct avg avg;
    bool status;
    uint32_t value;

    avg_init(&avg, avg_minima, ARRAY_SIZE(avg_minima), NULL, 0);

    for (unsigned int i=0; i<ARRAY_SIZE(elements); i++)
        avg_add(&avg, elements[i]);

    status = avg_ready(&avg);
    CU_ASSERT_TRUE(status);
    value = avg_calculate(&avg);
    CU_ASSERT_EQUAL(value, 66);
}


void test_average_maxima(void)
{
    uint32_t avg_maxima[3];
    struct avg avg;
    bool status;
    uint32_t value;

    avg_init(&avg, NULL, 0, avg_maxima, ARRAY_SIZE(avg_maxima));

    for (unsigned int i=0; i<ARRAY_SIZE(elements); i++)
        avg_add(&avg, elements[i]);

    status = avg_ready(&avg);
    CU_ASSERT_TRUE(status);
    value = avg_calculate(&avg);
    CU_ASSERT_EQUAL(value, 38);
}


void test_average_extrema(void)
{
    uint32_t avg_minima[3];
    uint32_t avg_maxima[3];
    struct avg avg;
    bool status;
    uint32_t value;

    avg_init(&avg, avg_minima, ARRAY_SIZE(avg_minima), avg_maxima, ARRAY_SIZE(avg_maxima));

    for (unsigned int i=0; i<ARRAY_SIZE(elements); i++)
        avg_add(&avg, elements[i]);

    status = avg_ready(&avg);
    CU_ASSERT_TRUE(status);
    value = avg_calculate(&avg);
    CU_ASSERT_EQUAL(value, 53);
}
