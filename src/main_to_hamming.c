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
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_fcntl.h>
#include <dc_posix/dc_unistd.h>
#include <dc_util/bits.h>
#include <dc_util/dump.h>
#include <dc_util/types.h>

#define BUF_SIZE 1024

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

struct application_settings
{
    struct dc_opt_settings opts;
    struct dc_setting_string *parity;
    struct dc_setting_string *prefix;
};

static struct dc_application_settings *create_settings( const struct dc_posix_env *env,
                                                        struct dc_error *err);

static int destroy_settings(const struct dc_posix_env *env,
                            struct dc_error *err,
                            struct dc_application_settings **psettings);
static int run( const struct dc_posix_env *env,
                struct dc_error *err,
                struct dc_application_settings *settings);
static void error_reporter(const struct dc_error *err);
static void trace_reporter(const struct dc_posix_env *env,
                          const char *file_name,
                          const char *function_name,
                          size_t line_number);
void createHammingWord( const struct dc_posix_env *env,
                        const struct dc_error *err, 
                        char, 
                        bool*);
void setParityBits( const struct dc_posix_env *env,
                        const struct dc_error *err,
                        bool isEvenParity,
                        uint16_t * dest);  
void copyUint8_tIntoHammingFormatUint16_t ( const struct dc_posix_env *env,
                                            const struct dc_error *err, 
                                            const uint8_t,
                                            uint16_t * dest);
static uint16_t set_bit(uint16_t byte, uint16_t mask);
static void print_mask(uint16_t byte, uint16_t mask);
static uint16_t get_mask(uint16_t byte, uint16_t  mask);
size_t powerOfTwo(size_t x);
bool isEven(size_t x);
bool isEvenParitySetting(const char * parity);
void writeToFiles(const struct dc_posix_env *env, const struct dc_error *err, uint16_t * sourcePtr, size_t numCodeWords, const char * prefix);
static uint8_t set_bit8(uint8_t byte, uint16_t mask);

int main(int argc, char * argv[]) {
    dc_posix_tracer tracer;
    dc_error_reporter reporter;
    struct dc_posix_env env;
    struct dc_error err;
    struct dc_application_info *info;
    int ret_val;

    reporter = error_reporter;
    tracer = NULL;
    // tracer = trace_reporter;
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
                    "even"},
            {(struct dc_setting *)settings->prefix,
                    dc_options_set_string,
                    "prefix",
                    required_argument,
                    'r',
                    "PREFIX",
                    dc_string_from_string,
                    "prefix",
                    dc_string_from_config,
                    "abc"}
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
    dc_setting_string_destroy(env, &app_settings->parity);
    dc_setting_string_destroy(env, &app_settings->prefix);
    dc_free(env, app_settings->opts.opts, app_settings->opts.opts_count);
    dc_free(env, *psettings, sizeof(struct application_settings));

    if(env->null_free)
    {
        *psettings = NULL;
    }

    return 0;
}

// Return 1 (true) for even, 0 for odd
// TODO: use dc_strncmp
bool isEvenParitySetting(const char * parity) {
    if (strcmp(parity, "odd") == 0)
        return false;
    else if (strcmp(parity, "even") == 0)
        return true;
    // Default case, default to even
    else {
        return true;
    }
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
    ret_val = 0;

    // Create settings
    app_settings = (struct application_settings *)settings;
    // Get settings
    parity = dc_setting_string_get(env, app_settings->parity);
    prefix = dc_setting_string_get(env, app_settings->prefix);

    // Process parity argument
    isEvenParity = isEvenParitySetting(parity);
    // printf("evenParity: %d\n", isEvenParity);


    if (dc_error_has_no_error(err)) {
        // Read from stdin
        while((nread = dc_read(env, err, STDIN_FILENO, chars, BUF_SIZE)) > 0) {
            // printf("nread: %d\n", nread);
            if(dc_error_has_error(err)) {
                ret_val = 1;
            }

            // These initialization statements are good. They do result in zeroed out memory of the appropriate size
            uint16_t * arr = (uint16_t*)calloc(nread, sizeof(uint16_t));
            uint16_t * dest = (uint16_t*)calloc(nread, sizeof(uint16_t));

            // display("dc_writes:");
            // dc_write(env, err, STDOUT_FILENO, arr, (nread * 2 ));
            // dc_write(env, err, STDOUT_FILENO, dest, (nread * 2 ));

            // Loop to process each char of line
            for(size_t i = 0; i < nread; i++) {
                 arr[i] = (uint16_t) chars[i];
                //  dest[i] = (uint16_t)'h';

                copyUint8_tIntoHammingFormatUint16_t(env, err, chars[i], (dest + i));
                setParityBits(env, err, isEvenParity, (dest + i));
            }

            if(dc_error_has_error(err))
            {
                display("Error: word is made");
            }

            // display line("dc_writes:");
            dc_write(env, err, STDOUT_FILENO, arr, (nread * 2 ));
            dc_write(env, err, STDOUT_FILENO, dest, (nread * 2 ));
            if(dc_error_has_error(err))
            {
                display("Error: dc_writes made");
            }

            // Write *dest to files 1 through 12
            writeToFiles(env, err, dest, (size_t)nread, prefix);
            // if(dc_error_has_error(err))
            // {
            //     display("Error: write to files done");
            // }
            if(dc_error_has_error(err)) 
                ret_val = 2;

            free(arr);
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
    // Loops through data word
    size_t i = 0;
    // Loops through 16 bit destination
    size_t j = 0;

    // Statements for debugging masks
    // display("source");
    // for (int i = 0; i < 8; i++) {
    //     print_mask(c, masks_16[i]);
    // }
    //     *dest = set_bit(*dest, masks_16[0]);
    //     display("dest");
    // for (int i = 0; i < 16; i++) {
    //     print_mask(*dest, masks_16[i]);
    // }


    // Loop through data word
    for (i = 0; i < 8; i++) {
        // print_mask(flipped, masks_16[i]);
        // If not power of two, set bit to 1 if bit should be 1. (Remember all this memory is zeroed out, so we'll just do nothing if its not a 1)
        if (!powerOfTwo(j)) {
            if ( get_mask(c, masks_16[i])) {
                *dest = set_bit(*dest, masks_16[j]);
            }
            // Increment j either way
            ++j;
        }
        else {
            while (powerOfTwo(j)) {
                ++j;
            }
            if ( get_mask(c, masks_16[i])) {
                *dest = set_bit(*dest, masks_16[j]);
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

    for (size_t i = pow(2,0); i < pow(2,4); i <<= 1 ) {
        parityCount = 0;
        j = i;
        while ( j < pow(2,4) ) {
            k = 0;
            while (k < i) {
                if ( get_mask(*dest, masks_16[j++]) )
                    parityCount++;
                k++;
            }
            j += i;
        }

        // printf("parityCount: %d\n", parityCount);
        // printf("is even: %d\n",isEven(parityCount));

        // at end of for loop, set parity per bit depending on count
        if ( (!isEven(parityCount) && isEvenParity ) || ( isEven(parityCount) && !isEvenParity) ) {
            *dest = set_bit(*dest, masks_16[i]);
        }
    }
}

void writeToFiles(const struct dc_posix_env *env, const struct dc_error *err, uint16_t * sourcePtr, size_t numCodeWords, const char * prefix) {    
    char pathBeginning[BUF_SIZE] = "";
    const char rel[] = "./";
    const char ext[] = ".hamming";
    const char sep[] = "-";
    char pathArr[12][BUF_SIZE] = {{0}};

    int fd;

    // Make common filepath start
    strncat(pathBeginning, rel, strlen(rel));
    strncat(pathBeginning, prefix, strlen(prefix));
    strncat(pathBeginning, sep, strlen(sep));
    // printf("path beginning: %s\n", pathBeginning);

    // Make an array of strings containing the filepaths 1 through 12
    // Format: {prefix}-0.hamming, {prefix}-1.hamming , ...., {prefix}-11.hamming 
    for (size_t i = 0; i < 12 ; i++) {
        char path[BUF_SIZE] = "";


        strncpy(path, pathBeginning, strlen(pathBeginning));
        
        // Number append
        char buffer[10];
        snprintf(buffer, 10, "%d", i);
        strncat(path, buffer, strlen(buffer));

        strncat(path, ext, strlen(ext));
        // printf("path: %s\n", path);
        strncpy(pathArr[i], path, strlen(path));
        // printf("path: %s\n", pathArr[i]);
    }

    if (dc_error_has_error(err)) {
        error_reporter(err);
    }

    // Open write and close each file in this loop
    for (size_t i = 0; i < 12; i++) {
        fd = dc_open(env, err, pathArr[i], DC_O_CREAT | DC_O_TRUNC | DC_O_WRONLY, S_IRUSR | S_IWUSR);
        if (dc_error_has_error(err)) {
            error_reporter(err);
        }

        // TODO: start here. make this a pointer to the appropriate amount of bytes 
        char byteToWrite = '\0';

        // Generate byte to write per file
        // TODO: if numCodeWords > 8, do something 
        for(size_t j = 0; j < numCodeWords; j++) {
            // check the i'th bit of each code word
            if ( get_mask((*(sourcePtr+j)), masks_16[i]) )
                // set byte's j'th bit for code-word i'th bit
                byteToWrite = set_bit8(byteToWrite, masks_16[j]);
        }
        // char c = i + '0';
        // printf("%c", c);
        dc_write(env, err, STDOUT_FILENO, &byteToWrite, 1);
        dc_write(env, err, fd, &byteToWrite, 1);
        if (dc_error_has_error(err)) {
            error_reporter(err);
        }

        dc_close(env, err, fd);
    }
}

static uint16_t set_bit(uint16_t byte, uint16_t mask) {
    uint16_t set;
    set = byte | mask;
    return set;
}

static uint8_t set_bit8(uint8_t byte, uint16_t mask) {
    uint8_t set;
    set = byte | mask;
    return set;
}

static void print_mask(uint16_t byte, uint16_t mask) {
    uint16_t masked;
    masked = byte & mask;
    printf("%u\n", masked);
}

static uint16_t get_mask(uint16_t byte, uint16_t  mask) {
    uint16_t masked;
    masked = byte & mask;
    return masked;
}

static uint8_t get_mask8(uint8_t byte, uint8_t mask) {
    uint8_t masked;
    masked = byte & mask;
    return masked;
}

static void error_reporter(const struct dc_error *err) {
    fprintf(stderr, "ERROR: %s : %s : @ %zu : %d\n", err->file_name, err->function_name, err->line_number, 0);
    fprintf(stderr, "ERROR: %s\n", err->message);
}

static void trace_reporter(__attribute__((unused))  const struct dc_posix_env *env,
                                                    const char *file_name,
                                                    const char *function_name,
                                                    size_t line_number) {
    fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}

/**
 * Returns 1 if argument is a power of two, 0 otherwise
 */
size_t powerOfTwo(size_t x)
{
   //checks whether a number is zero or not
   if (x == 0)
      return 0;

   //true till x is not equal to 1
   while( x != 1)
   {
      //checks whether a number is divisible by 2
      if(x % 2 != 0)
         return 0;
         x /= 2;
   }
   return 1;
}

bool isEven(size_t x) {
    return (x % 2 == 0);
}