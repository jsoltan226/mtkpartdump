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
