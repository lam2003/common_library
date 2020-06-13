#include "thread/pool.h"
#include "utils/logger.h"
#include <atomic>
#include <iostream>
#include <poller/event_poller.h>
#include <signal.h>
#include <thread/group.h>
#include <thread/pool.h>
#include <thread/semaphore.h>
#include <thread/task.h>
#include <thread/task_executor.h>
#include <thread/task_queue.h>
#include <unistd.h>
#include <utils/function_traits.h>
#include <utils/list.h>
#include <utils/once_token.h>
#include <utils/utils.h>

using namespace std;
using namespace common_library;

void test()
{
    throw std::runtime_error("fuck you");
}
int main()
{
    test();

    return 0;
}
