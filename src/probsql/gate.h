#ifndef GATE_H
#define GATE_H
#include "enums.h"
#include "structs.h"

typedef struct
{
    // We use this to discriminate the union below.
    gate_type gate_type;

    // The information pertaining to this gate,
    // depending on what gate type it has.
    gate_info gate_info;
} Gate;

#endif