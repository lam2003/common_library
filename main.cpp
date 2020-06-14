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
#include <typeinfo>
#include <unistd.h>
#include <utils/function_traits.h>
#include <utils/list.h>
#include <utils/once_token.h>
#include <utils/utils.h>

#include <iostream>

using namespace std;
using namespace common_library;

int main()
{
    Logger::Instance().SetWriter(
        std::make_shared<AsyncLogWriter>(Logger::Instance()));
    Logger::Instance().AddChannel(std::make_shared<ConsoleChannel>("test",LERROR));

    std::string temp = get_exe_path();

    LOG_E << temp;
    return 0;
}
