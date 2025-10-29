#ifndef SPINLOCK_H_
#define SPINLOCK_H_

#include "int.h"
#include <stdatomic.h>

typedef atomic_flag spinlock_t;
#define SPINLOCK_INIT ATOMIC_FLAG_INIT
void spinlock_init(spinlock_t *lock);

void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);

i32 spinlock_try_acquire(spinlock_t *lock);

#endif /* SPINLOCK_H_ */
