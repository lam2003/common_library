#include "sync/coroutine.h"

#include <setjmp.h>
#include <stdint.h>

extern "C" int save_context(jmp_buf jpf);
extern "C" int restore_context(jmp_buf jpf, int32_t code);
extern "C" int replace_stack(jmp_buf jpf, void* sp);