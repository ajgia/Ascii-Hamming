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

#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <dc_posix/dc_unistd.h>


int display(const char *str)
{
    printf("%s\n", str);

    return 0;
}

uint16_t set_bit(uint16_t byte, uint16_t mask) {
    uint16_t set;
    set = byte | mask;
    return set;
}

uint8_t set_bit8(uint8_t byte, uint16_t mask) {
    uint8_t set;
    set = byte | mask;
    return set;
}

void print_mask(uint16_t byte, uint16_t mask) {
    uint16_t masked;
    masked = byte & mask;
    printf("%u\n", masked);
}

uint16_t get_mask(uint16_t byte, uint16_t  mask) {
    uint16_t masked;
    masked = byte & mask;
    return masked;
}

uint8_t get_mask8(uint8_t byte, uint8_t mask) {
    uint8_t masked;
    masked = byte & mask;
    return masked;
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