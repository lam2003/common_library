#ifndef COMMON_LIBRARY_THREAD_GROUP_H
#define COMMON_LIBRARY_THREAD_GROUP_H

#include <utils/list.h>

#include <memory>
#include <thread>
#include <unordered_map>

namespace common_library {

class ThreadGroup {
  public:
    ThreadGroup() = default;
    ~ThreadGroup()
    {
        threads_.clear();
    }

    template <typename Func> std::thread* CreateThread(Func&& f)
    {
        std::shared_ptr<std::thread> thread_new =
            std::make_shared<std::thread>(f);
        thread_id_           = thread_new->get_id();
        threads_[thread_id_] = thread_new;

        return thread_new.get();
    }

    void RemoveThread(std::thread* t)
    {
        std::unordered_map<std::thread::id,
                           std::shared_ptr<std::thread>>::iterator it =
            threads_.find(t->get_id());

        if (it != threads_.end()) {
            threads_.erase(it);
        }
    }

    bool IsSelfThreadIn()
    {
        std::thread::id tid = std::this_thread::get_id();
        if (tid == thread_id_) {
            return true;
        }

        return threads_.find(tid) != threads_.end();
    }

    bool IsThreadIn(std::thread* t)
    {
        std::thread::id tid = t->get_id();
        if (tid == thread_id_) {
            return true;
        }

        return threads_.find(tid) != threads_.end();
    }

    size_t Size()
    {
        return threads_.size();
    }

    void JoinAll()
    {
        if (IsSelfThreadIn()) {
            throw std::runtime_error("thread_group: trying joining itself");
        }

        for (std::pair<const std::thread::id, std::shared_ptr<std::thread>>&
                 it : threads_) {
            if (it.second->joinable()) {
                it.second->join();
            }
        }

        threads_.clear();
    }

  private:
    ThreadGroup(const ThreadGroup& that) = delete;
    ThreadGroup(ThreadGroup&& that)      = delete;
    ThreadGroup& operator=(const ThreadGroup& that) = delete;
    ThreadGroup& operator=(ThreadGroup&& that) = delete;

  private:
    std::unordered_map<std::thread::id, std::shared_ptr<std::thread>> threads_;
    std::thread::id thread_id_;
};

}  // namespace common_library

#endif