#include <thread/worker.h>
#include <utils/utils.h>

namespace common_library {

INSTANCE_IMPL(WorkerPool);

int WorkerPool::s_pool_size_ = 0;

void WorkerPool::SetPoolSize(int size)
{
    s_pool_size_ = size;
}

WorkerPool::WorkerPool()
{
    int size =
        s_pool_size_ > 0 ? s_pool_size_ : std::thread::hardware_concurrency();

    create_executor(
        []() {
            Worker::Ptr ret(new Worker);
            return ret;
        },
        size);

    LOG_I << "worker pool size: " << size;
}

Worker::Ptr WorkerPool::GetWorker()
{
    return std::dynamic_pointer_cast<Worker>(GetExecutor());
}

}  // namespace common_library