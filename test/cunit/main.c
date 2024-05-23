
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>



static char cunit_result_file[] = "/tmp/mx_test.cunit.log";


struct args
{
    bool automated;
    bool basic;
    bool console;
    char *file;
    int verbose;
    unsigned int logger;
};

static void show_help(const char *name);
static bool args_parse(int argc, char *argv[], struct args *args);


extern CU_ErrorCode cu_test_cba();
extern CU_ErrorCode cu_test_dart();
extern CU_ErrorCode cu_test_process();
extern CU_ErrorCode cu_test_avg();
extern CU_ErrorCode cu_test_message_list();
extern CU_ErrorCode cu_test_message_queue();


int main(int argc, char *argv[])
{
    struct args args;
    if (!args_parse(argc, argv, &args))
        return 1;

    if ( CU_initialize_registry() != CUE_SUCCESS )
        return CU_get_error();

    /* Add modules */
    cu_test_cba();
    cu_test_dart();
    cu_test_process();
    cu_test_avg();
    cu_test_message_list();
    cu_test_message_queue();

    if (args.basic) {
        /* Run all tests using the CUnit Basic interface */
        CU_basic_set_mode(args.verbose);
        CU_basic_run_tests();
    }

    if (args.automated) {
        /* Run all tests using the CUnit Automated interface */
        if (args.file != NULL)
            CU_set_output_filename(args.file);
        else
            CU_set_output_filename(cunit_result_file);
        CU_list_tests_to_file();
        CU_automated_run_tests();
    }

    if (args.console) {
        /* Run all tests using the CUnit Console interface */
        CU_console_run_tests();
    }

    if (args.file != NULL)
        free(args.file);
    CU_cleanup_registry();
    return CU_get_error();
}



static struct option options[] =
{
    {"auto",    no_argument,        0,  'a'},
    {"basic",   no_argument,        0,  'b'},
    {"console", no_argument,        0,  'c'},
    {"verbose", required_argument,  0,  'v'},
    {"file",    required_argument,  0,  'f'},
    {"logger",  required_argument,  0,  'l'},
    {"help",    no_argument,        0,  'h'},
    {0,         0,                  0,   0}
};


bool args_parse(int argc, char *argv[], struct args *args)
{
    int c;
    int option_idx = 0;
    bool success = true;

    args->verbose = CU_BRM_NORMAL;
    args->automated = false;
    args->basic = false;
    args->console = false;
    args->file = NULL;

    while (1) {
        long val;
        char *end = NULL;
        c = getopt_long(argc, argv, "abchv:f:l:", options, &option_idx);
        if (c == -1)
            break;

        switch (c) {
        case 'a':
            args->automated = true;
            break;
        case 'b':
            args->basic = true;
            break;
        case 'c':
            args->console = true;
            break;

        case 'v':
            end = NULL;
            val = strtol(optarg, &end, 10);
            if (*end != '\0') {
                fprintf(stderr, "Could not convert verbose value '%s'\n", optarg);
                success = false;
                break;
            }
            switch (val) {
            case 0:
                args->verbose = CU_BRM_SILENT;
                break;
            case 1:
                // args->verbose = CU_BRM_NORMAL;
                break;
            case 2:
                args->verbose = CU_BRM_VERBOSE;
                break;
            default:
                fprintf(stderr, "Verbose value not supported, using default\n");
            }
            break;

        case 'f':
            args->file = strdup(optarg);
            break;

        case 'l':
            end = NULL;
            val = strtol(optarg, &end, 16);
            if (*end != '\0') {
                fprintf(stderr, "Could not convert log value '%s'\n", optarg);
                success = false;
                break;
            }
            args->logger = (unsigned int)val;
            break;

        case 'h':
        case '?':
            show_help(argv[0]);
            success = false;
            break;

        default:
            fprintf(stderr, "Could not parse arguments\n");
            success = false;
            break;
        }
    }

    if (!args->basic && !args->console)
        args->automated = true;

    if (!success && args->file != NULL) {
        free(args->file);
        args->file = NULL;
    }

    return success;
}


void show_help(const char *name)
{
    printf("\nUsage: %s [options] ...\n", name);

    printf("\nOptions:\n");
    printf("  -a  --auto        cunit automated mode, default\n");
    printf("  -b  --basic       cunit basic mode\n");
    printf("  -c  --console     cunit interactive console mode\n");
    printf("  -v  --verbose     set verbose value [0-silent,1-normal,2-verbose]\n");
    printf("  -f  --file        output file when automated mode is set\n");
    printf("  -l  --logger      set logger value\n");
    printf("  -h  --help        help\n");

    printf("\nExamples:\n");
    printf("  %s --basic --verbos=2 --logger=0xFFFF\n", name);
    printf("  %s -b -a -v0 -f \"/tmp/cunit-result\"\n", name);
    printf("  %s --console\n", name);
    printf("\n");
}
