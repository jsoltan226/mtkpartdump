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
#include "pressable-obj.h"
#include "util.h"
#include "log.h"
#include <stdlib.h>

#define MODULE_NAME "pressable-obj"

pressable_obj_t * pressable_obj_create(void)
{
    pressable_obj_t *po = calloc(1, sizeof(pressable_obj_t));
    if (po == NULL)
        s_log_fatal("calloc failed for %s", "new pressable obj");

    return po;
}

void pressable_obj_update(pressable_obj_t *po, bool state)
{
    if (po == NULL) return;

    /* Down is active if on the current tick the object is pressed,
     * but wasn't pressed on the previous one */
    po->down = (state && !po->pressed && !po->force_released);

    /* Up is active immidiately after object was released. */
    po->up = (!state && po->pressed && !po->force_released);

    /* Update the po->pressed member only if the object isn't force released */
    po->pressed = state && !po->force_released;

    /* Time pressed should always be incremented when the object is pressed */
    if(state) {
        po->time++;
    } else {
        /* Otherwise reset the time, as well as the force_released state */
        po->time = 0;
        po->force_released = false;
    }
}

void pressable_obj_reset(pressable_obj_t *po)
{
    if (po == NULL) return;
    memset(po, 0, sizeof(pressable_obj_t));
}

void pressable_obj_force_release(pressable_obj_t *po)
{
    if (po == NULL) return;
    /* The pressed, up and down values as well as the time
     * should be 0 until the force_released state is reset */
    po->pressed = false;
    po->up = false;
    po->down = false;
    po->time = 0;
    po->force_released = true;
}

void pressable_obj_destroy(pressable_obj_t **po_p)
{
    if (po_p == NULL || *po_p == NULL) return;

    u_nzfree(po_p);
}
