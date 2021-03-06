/*
 * Copyright (c) 2019 Maciej Suminski <orson@orson.net.pl>
 *
 * This source code is free software; you can redistribute it
 * and/or modify it in source code form under the terms of the GNU
 * General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef IO_CAPTURE_H
#define IO_CAPTURE_H

#include <stdint.h>

typedef enum { F32MHZ, F16MHZ, F8MHZ, F4MHZ, F2MHZ, F1MHZ } clock_freq_t;

void ioc_init(void);
void ioc_set_clock(clock_freq_t freq);
void ioc_fetch(uint8_t *destination, uint32_t sample_count);
int  ioc_busy(void);
void ioc_set_handler(void (*func)(void*), void* param);

#endif /* IO_CAPTURE_H */
