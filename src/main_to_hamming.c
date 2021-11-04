/**
 * ASCII to Hamming
 * Alex Giasson
 * A00982145 
*/

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <dc_posix/dc_unistd.h>
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/defaults.h>
#include <dc_application/environment.h>
#include <dc_application/options.h>
#include <dc_application/application.h>
#include <dc_application/settings.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_fcntl.h>
#include <dc_util/bits.h>
#include <dc_util/dump.h>
#include <dc_util/types.h>
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/options.h>

#define BUF_SIZE 1024

/**
 * Bit masks
 */ 
const uint16_t MASK_00000000_00000001 = UINT16_C(0x00000001);
const uint16_t MASK_00000000_00000010 = UINT16_C(0x00000002);
const uint16_t MASK_00000000_00000100 = UINT16_C(0x00000004);
const uint16_t MASK_00000000_00001000 = UINT16_C(0x00000008);
const uint16_t MASK_00000000_00010000 = UINT16_C(0x00000010);
const uint16_t MASK_00000000_00100000 = UINT16_C(0x00000020);
const uint16_t MASK_00000000_01000000 = UINT16_C(0x00000040);
const uint16_t MASK_00000000_10000000 = UINT16_C(0x00000080);
const uint16_t MASK_00000001_00000000 = UINT16_C(0x00000100);
const uint16_t MASK_00000010_00000000 = UINT16_C(0x00000200);
const uint16_t MASK_00000100_00000000 = UINT16_C(0x00000400);
const uint16_t MASK_00001000_00000000 = UINT16_C(0x00000800);
const uint16_t MASK_00010000_00000000 = UINT16_C(0x00001000);
const uint16_t MASK_00100000_00000000 = UINT16_C(0x00002000);
const uint16_t MASK_01000000_00000000 = UINT16_C(0x00004000);
const uint16_t MASK_10000000_00000000 = UINT16_C(0x00008000);

/**
 * Bit mask array
 */ 
static const uint16_t masks_16[] = {
    MASK_00000000_00000001,
    MASK_00000000_00000010,
    MASK_00000000_00000100, 
    MASK_00000000_00001000,
    MASK_00000000_00010000,
    MASK_00000000_00100000,
    MASK_00000000_01000000,
    MASK_00000000_10000000,
    MASK_00000001_00000000,
    MASK_00000010_00000000,
    MASK_00000100_00000000,
    MASK_00001000_00000000,
    MASK_00010000_00000000,
    MASK_00100000_00000000,
    MASK_01000000_00000000,
    MASK_10000000_00000000
};

/**
 * DC Application settings
 */ 
struct application_settings
{
    struct dc_opt_settings opts;
    struct dc_setting_string *parity;
    struct dc_setting_string *prefix;
};

// Defining this here. I don't know why it's not working through the includes
struct dc_setting_string
{
    struct dc_setting parent;
    char *string;
};

/**
 * Create DC Application settings
 */ 
static struct dc_application_settings *create_settings( const struct dc_posix_env *env,
                                                        struct dc_error *err);
/**
 * Destroy DC application settings
 */ 
static int destroy_settings(const struct dc_posix_env *env,
                            struct dc_error *err,
                            struct dc_application_settings **psettings);
/**
 * Convert ascii input to hamming files
 */ 
static int run( const struct dc_posix_env *env,
                struct dc_error *err,
                struct dc_application_settings *settings);
/**
 * Error reporter
 */ 
static void error_reporter(const struct dc_error *err);
/**
 * Trace reporter
 */ 
static void trace_reporter(const struct dc_posix_env *env,
                          const char *file_name,
                          const char *function_name,
                          size_t line_number);


/**
 * Set parity bits of a uint16_t
 * @param env dc_env
 * @param err dc_err
 * @param isEvenParity boolean for parity
 * @param dest uint16_t pointer to destination to modify
 */ 
void setParityBits( const struct dc_posix_env *env,
                        const struct dc_error *err,
                        bool isEvenParity,
                        uint16_t * dest);  
/**
 * Copy 8 bit char into 12 bit hamming format documented in darcy_design.txt
 * Parity bits are not set here.
 * @param env dc_env
 * @param err dc_err
 * @param input uint8_t input
 * @param dest uint16_t destination for hamming code format
 */ 
void copyUint8_tIntoHammingFormatUint16_t ( const struct dc_posix_env *env,
                                            const struct dc_error *err, 
                                            const uint8_t input,
                                            uint16_t * dest);
/**
 * Write hamming codeword to the 12 files, 1 bit of each word per file.
 * @param env dc_env
 * @param err dc_err
 * @param sourcePtr pointer to uint16_t memory block of input data
 * @param numCodeWords number of code words to write to files
 * @param prefix prefix argument used to generate file names
 */ 
void writeToFiles(const struct dc_posix_env *env, struct dc_error *err, uint16_t * sourcePtr, size_t numCodeWords, const char * prefix);

/**
 * Main
 */ 
int main(int argc, char * argv[]) {
    dc_posix_tracer tracer;
    dc_error_reporter reporter;
    struct dc_posix_env env;
    struct dc_error err;
    struct dc_application_info *info;
    int ret_val;

    reporter = error_reporter;
    tracer = trace_reporter;
    tracer = NULL;
    dc_error_init(&err, reporter);
    dc_posix_env_init(&env, tracer);
    info = dc_application_info_create(&env, &err, "To Hamming Application");
    ret_val = dc_application_run(&env, &err, info, create_settings, destroy_settings, run, dc_default_create_lifecycle, dc_default_destroy_lifecycle, NULL, argc, argv);
    dc_application_info_destroy(&env, &info);
    dc_error_reset(&err);
    return ret_val;
}

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err)
{
    static bool default_verbose = false;
    struct application_settings *settings;

    DC_TRACE(env);
    settings = dc_malloc(env, err, sizeof(struct application_settings));

    if(settings == NULL)
    {
        return NULL;
    }

    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->parity = dc_setting_string_create(env, err);
    settings->prefix = dc_setting_string_create(env, err);

    struct options opts[] = {
            {(struct dc_setting *)settings->opts.parent.config_path,
                    dc_options_set_path,
                    "config",
                    required_argument,
                    'c',
                    "CONFIG",
                    dc_string_from_string,
                    NULL,
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *)settings->parity,
                    dc_options_set_string,
                    "parity",
                    required_argument,
                    'p',
                    "PARITY",
                    dc_string_from_string,
                    "parity",
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *)settings->prefix,
                    dc_options_set_string,
                    "prefix",
                    required_argument,
                    'f',
                    "PREFIX",
                    dc_string_from_string,
                    "prefix",
                    dc_string_from_config,
                    NULL}
    };

    // note the trick here - we use calloc and add 1 to ensure the last line is all 0/NULL
    settings->opts.opts_count = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size = sizeof(struct options);
    settings->opts.opts = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags = "m:";
    settings->opts.env_prefix = "DC_EXAMPLE_";

    return (struct dc_application_settings *)settings;
}

static int destroy_settings(const struct dc_posix_env *env,
                            __attribute__((unused)) struct dc_error *err,
                            struct dc_application_settings **psettings) 
{
    struct application_settings *app_settings;

    DC_TRACE(env);
    app_settings = (struct application_settings *)*psettings;
    
    if ( app_settings->parity->string != NULL)
        dc_setting_string_destroy(env, &(app_settings->parity));
    if ( app_settings->prefix->string != NULL)
        dc_setting_string_destroy(env, &(app_settings->prefix));
    dc_free(env, app_settings->opts.opts, app_settings->opts.opts_count);
    dc_free(env, *psettings, sizeof(struct application_settings));

    if(env->null_free)
    {
        *psettings = NULL;
    }

    return 0;
}

static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings) {
    struct application_settings *app_settings;
    const char *parity;
    const char *prefix;

    char chars[BUF_SIZE];
    ssize_t nread;
    int ret_val;
    bool isEvenParity;
    
    DC_TRACE(env);
    ret_val = EXIT_SUCCESS;

    // Create and get settings
    app_settings = (struct application_settings *)settings;
    parity = dc_setting_string_get(env, app_settings->parity);
    prefix = dc_setting_string_get(env, app_settings->prefix);

    if (prefix == NULL || parity == NULL) {
        display("Error: Prefix and parity arguments are required. Exiting.");
        return EXIT_FAILURE;
    }
    int paritySetting = isEvenParitySetting(parity);

    if ( paritySetting != 0 && paritySetting != 1 ) {
        display("fuck");
        return EXIT_FAILURE;
    }
    else 
        isEvenParity = (bool)paritySetting;


    if (dc_error_has_no_error(err)) {
        // Read from stdin
        while((nread = dc_read(env, err, STDIN_FILENO, chars, BUF_SIZE)) > 0) {
            // printf("nread: %d\n", nread);
            if(dc_error_has_error(err)) {
                ret_val = 1;
            }

            uint16_t * dest = (uint16_t*)calloc((size_t)nread, sizeof(uint16_t));

            // Loop to process each char of line
            for(size_t i = 0; i < (size_t)nread; i++) {
                copyUint8_tIntoHammingFormatUint16_t(env, err, chars[i], (dest + i));
                setParityBits(env, err, isEvenParity, (dest + i));
            }

            // Write *dest to files 1 through 12
            writeToFiles(env, err, dest, (size_t)nread, prefix);

            free(dest);
        }

        // printf("ret_val: %d\n", ret_val);

    }

    return ret_val;
}

/**
 * Copies a Uint8_t into Uint16_t in "Hamming format"
 * Hamming-format: DPPD PDDD PDDD 0000  , where D = Data bit, P = parity bit, 0 = padding
 * DOES NOT SET THE PARITY BITS
 */ 
void copyUint8_tIntoHammingFormatUint16_t (const struct dc_posix_env *env, const struct dc_error *err, const uint8_t c, uint16_t * dest) {
    // Counter for data word
    size_t i = 0;
    // Counter for 16 bit destination
    size_t j = 1;

    // Loop through data word
    for (i = 0; i < 8; i++) {
        // If not power of two, set bit to 1 if bit should be 1. (Remember all this memory is zeroed out, so we'll just do nothing if its not a 1)
        if (!powerOfTwo(j)) {
            if ( get_mask(c, masks_16[i])) {
                *dest = set_bit(*dest, masks_16[j-1]);
            }
            // Increment j either way
            ++j;
        }
        else {
            while (powerOfTwo(j)) {
                ++j;
            }
            if ( get_mask(c, masks_16[i])) {
                *dest = set_bit(*dest, masks_16[j-1]);
            }
            ++j;
        }
    }
}

/**
 * Takes a pointer to a "Hamming-formatted" Uint16 that does not have parity bits set yet, and sets them.
 * Hamming-format: DPPD PDDD PDDD 0000  , where D = Data bit, P = parity bit, 0 = padding
 */
    // For each 2^n parity bit for n in [0, 3] .... (2^0, 2^1, 2^2, 2^3)
    // 1. Calculate parity by checking bits starting at 2^nth bit, then skip 2^n bits, check 2^n bits, etc.
    //      a) for each bit check, get masked version at bit position and add result to a count
    //        b) result of count (whether count is even or odd) is the bit parity
    // 2. Set parity of 2^n bit depending on even or odd parity
void setParityBits(const struct dc_posix_env *env, const struct dc_error *err, bool isEvenParity, uint16_t *dest) {
    size_t parityCount;
    size_t j;
    size_t k;

    for (size_t i = pow(2,0); i <= pow(2,4); i <<= 1 ) {
        parityCount = 0;
        j = i;
        while ( j <= pow(2,4) ) {
            k = 0;
            while (k < i) {
                if ( get_mask(*dest, masks_16[j-1]) ) {
                    parityCount++;
                }
                ++j;
                ++k;
            }
            j += i;
        }

        // printf("parityCount: %d\n", parityCount);
        // printf("is even: %d\n",isEven(parityCount));

        // at end of each parity bit check, set parity per bit depending on count
        if ( (!isEven(parityCount) && isEvenParity ) || ( isEven(parityCount) && !isEvenParity) ) {
            *dest = set_bit(*dest, masks_16[i-1]);
        }
    }
}

void writeToFiles(const struct dc_posix_env *env, struct dc_error *err, uint16_t * sourcePtr, size_t numCodeWords, const char * prefix) {    
    char* pathArr;
    int fd;
    pathArr = constructFilePathArray(prefix);

    // Bit counter
    size_t l = 0;
    // Byte counter
    size_t k = 0;
    
    

    if (dc_error_has_error(err)) {
        error_reporter(err);
    }

    // Open, write and close each file in this loop
    for (size_t i = 0; i < 12; i++) {


        fd = dc_open(env, err, (pathArr+(i*BUF_SIZE)), DC_O_CREAT | DC_O_TRUNC | DC_O_WRONLY, S_IRUSR | S_IWUSR);
        if (dc_error_has_error(err)) {
            error_reporter(err);
        }

        // Calculate number of bytes to write from number of code words
        size_t numBytesToWrite = numCodeWords/8;
        if ( (numCodeWords%8) != 0 )
            ++numBytesToWrite;

        uint8_t *bytesToWrite = (uint8_t*)calloc(numBytesToWrite, sizeof(uint8_t));

        l = 0;
        k = 0;
        // Generate bytes to write per file
        for(size_t j = 0; j < numCodeWords; j++) {
            if ( l == 8) {
                l = 0;
                ++k;
            }
                
            // check the i'th bit of each code word
            if ( get_mask((*(sourcePtr+j)), masks_16[i]) ) {
                // set byte's j'th bit for code-word i'th bit
                // byteToWrite = set_bit8(byteToWrite, masks_16[j]);
                *(bytesToWrite+k) = set_bit8(*(bytesToWrite+k), masks_16[l]);
            }
            ++l;
            
        }

        dc_write(env, err, STDOUT_FILENO, bytesToWrite, numBytesToWrite);
        dc_write(env, err, fd, bytesToWrite, numBytesToWrite);

        if (dc_error_has_error(err)) {
            error_reporter(err);
        }

        close(fd);
        free(bytesToWrite);
        // dc_close(env, err, fd);
    }
    destroyArray(pathArr);
}

static void error_reporter(const struct dc_error *err) {
    fprintf(stderr, "ERROR: %s : %s : @ %zu : %d\n", err->file_name, err->function_name, err->line_number, 0);
    fprintf(stderr, "ERROR: %s\n", err->message);
}

static void trace_reporter ( const struct dc_posix_env *env,
                            const char *file_name,
                            const char *function_name,
                            size_t line_number) {
    fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}
