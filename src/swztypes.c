#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<stdio.h>

#include "swzlisp.h"

#define SWZ_TYPE_HEADER     \
    &_swztypes_,            \
    NULL,                   \
    'w'

// -*-
static SWZObject* _swz_eval_error(SWZLisp *swz, SWZEnv *env, SWZObject *obj){
    (void)env;
    (void)obj;
    return swz_error(swz, SWZE_EVAL, "cannot evaluate this object");
}