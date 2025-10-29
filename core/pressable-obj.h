#ifndef PRESSABLEOBJ_H
#define PRESSABLEOBJ_H
#include "static-tests.h"

#include <stdbool.h>
#include "int.h"

typedef struct {
    bool up, down, pressed, force_released;
    i32 time;
} pressable_obj_t;

pressable_obj_t * pressable_obj_create(void);

void pressable_obj_update(pressable_obj_t *po, bool state);

void pressable_obj_reset(pressable_obj_t *po);
void pressable_obj_force_release(pressable_obj_t *po);

void pressable_obj_destroy(pressable_obj_t **po_p);

#endif
