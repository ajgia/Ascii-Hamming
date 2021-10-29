#ifndef TEMPLATE_COMMON_H
#define TEMPLATE_COMMON_H
#include <stdlib.h>
#include <dc_posix/dc_unistd.h>
/*
 * This file is part of dc_dump.
 *
 *  dc_dump is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Foobar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with dc_dump.  If not, see <https://www.gnu.org/licenses/>.
 */


/**
 * A function to be documented.
 *
 * @param str a parameter to be documented.
 * @return a return value to be documented.
 */
int display(const char *str);
uint16_t set_bit(uint16_t byte, uint16_t mask);
uint8_t set_bit8(uint8_t byte, uint16_t mask);
void print_mask(uint16_t byte, uint16_t mask);
uint16_t get_mask(uint16_t byte, uint16_t  mask);
uint8_t get_mask8(uint8_t byte, uint16_t mask);
size_t powerOfTwo(size_t x);
bool isEven(size_t x);
bool isEvenParitySetting(const char * parity);

char* constructFilePathArray(const struct dc_posix_env *env, const struct dc_error *err, const char * prefix);
void destroyArray(char* arr);
#endif // TEMPLATE_COMMON_H
