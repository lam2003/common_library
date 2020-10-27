#include "thread/worker.h"
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

template <typename T,
          template <typename E, typename = std::allocator<E>> class Obj =
              std::vector>
class Stack {
    Stack()
    {
        queue_.push_back(1);
        queue_.push_back(2);
        queue_.push_back(3);
    }

  private:
    Obj<T> queue_;
};

int main()
{
    Logger::Instance().SetWriter(
        std::make_shared<AsyncLogWriter>(Logger::Instance()));
    Logger::Instance().AddChannel(
        std::make_shared<ConsoleChannel>("test", LTRACE));

    std::string temp = get_exe_path();

    LOG_E << temp;
    return 0;
}
