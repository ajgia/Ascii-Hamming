#ifndef TEMPLATE_COMMON_H
#define TEMPLATE_COMMON_H
#include <stdlib.h>
#include <dc_posix/dc_unistd.h>

/**
 * Prints to std_out the string argument followed by a newline
 * @param str a string pointer
 * @return an int
 */
int display(const char *str);
uint16_t set_bit(uint16_t byte, uint16_t mask);
/**
 * Sets mask bit of byte argument 
 * @param byte a uint8_t to set
 * @param mask a uint8_t mask
 * @return a uint8_t
 */ 
uint8_t set_bit8(uint8_t byte, uint16_t mask);
/**
 * Prints masked byte argument
 * @param byte a uint8_t to print
 * @param mask a uint16_t mask
 */ 
void print_mask8(uint8_t byte, uint16_t mask);
/**
 * Gets masked byte
 * @param byte a uint8_t to get from
 * @param mask a uint16_t mask
 * @return a uint8_t
 */ 
uint8_t get_mask8(uint8_t byte, uint16_t mask);
/**
 * Prints masked byte argument
 * @param byte a uint16_t to print
 * @param mask a uint16_t mask
 */ 
void print_mask(uint16_t byte, uint16_t mask);
/**
 * Gets masked byte
 * @param byte a uint16_t to get from
 * @param mask a uint16_t mask
 * @return a uint16_t
 */ 
uint16_t get_mask(uint16_t byte, uint16_t mask);

/**
 * @brief Checks if power of two
 * 
 * @param x arg to check
 * @return size_t 1 for true, 0 for false
 */
size_t powerOfTwo(size_t x);
/**
 * @brief Checks if arg is even
 * 
 * @param x size_t
 * @return true even
 * @return false odd
 */
bool isEven(size_t x);
/**
 * @brief Interpets string value into parity setting
 * 
 * @param parity char*
 * @return int 0 for odd, 1 for even, 2 for invalid string arg
 */
int isEvenParitySetting(const char * parity);

/**
 * @brief Returns array of filepaths in the format {{prefix}-i.hamming} where i is a number 1 through 11.
 * 
 * @param env 
 * @param err 
 * @param prefix 
 * @return char* array
 */
char* constructFilePathArray(const struct dc_posix_env *env, const struct dc_error *err, const char * prefix);
/**
 * @brief Destroys file path array
 * 
 * @param arr 
 */
void destroyArray(char* arr);
#endif // TEMPLATE_COMMON_H
