/* mtkpartdump - Mediatek partition dump tool
 * Copyright (C) 2025 Jan Sołtan <jsoltan226@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>
*/
#include "spinlock.h"
#include "int.h"
#include <stdatomic.h>

void spinlock_init(spinlock_t *lock)
{
    *lock = (atomic_flag)ATOMIC_FLAG_INIT;
}

void spinlock_acquire(spinlock_t *lock)
{
    while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire))
        ;
}

void spinlock_release(spinlock_t *lock)
{
    atomic_flag_clear_explicit(lock, memory_order_release);
}

i32 spinlock_try_acquire(spinlock_t *lock)
{
    return atomic_flag_test_and_set_explicit(lock, memory_order_acquire);
}
