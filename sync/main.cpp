
#include <stdio.h>

typedef int context_buffer[8 * 2];

extern "C" int save_context(context_buffer* buf);

extern "C" void restore_context(context_buffer* buf, int retcode);

extern "C" void replace_stack(context_buffer* buf, void* sp);

context_buffer* b1 = (context_buffer*)(new int[sizeof(context_buffer)]);
context_buffer* b2 = (context_buffer*)(new int[sizeof(context_buffer)]);

bool fuck1()
{
    printf("fuck1\n");
    int ret = save_context(b1);
    if (ret == 0) {
        return true;
    }
    else if (ret == 1) {
        printf("fuck1 back %d\n", ret);
        restore_context(b2, 2);
        return false;
    }
    else {
        return false;
    }
}
void fuck2()
{
    printf("fuck2\n");
    int ret = save_context(b2);
    if (ret == 0) {
        restore_context(b1, 1);
        return;
    }
    printf("fuck2 back %d\n", ret);
}
int main()
{
    bool ok = fuck1();
    if (!ok) {
        return 0;
    }
    fuck2();

    return 0;
}