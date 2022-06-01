#ifndef COMMON_LIBRARY_LIST_H
#define COMMON_LIBRARY_LIST_H

#include <utility>

#include <stdint.h>

namespace common_library {

template <typename T> class List;

template <typename T> class ListNode final {
  public:
    friend class List<T>;

    template <typename... Args>
    ListNode(Args&&... args) : data_(std::forward<Args>(args)...)
    {
    }

    ~ListNode() = default;

  private:
    T         data_;
    ListNode* next_ = nullptr;
};

template <typename T> class List final {
  public:
    typedef ListNode<T> NodeType;

    List() {}

    List(List&& that)
    {
        swap(that);
    }

    ~List()
    {
        clear();
    }

  public:
    template <typename... Args> void emplace_front(Args&&... args)
    {
        NodeType* node = new NodeType(std::forward<Args>(args)...);
        if (!front_) {
            front_ = tail_ = node;
        }
        else {
            node->next_ = front_;
            front_      = node;
        }
        ++size_;
    }

    template <typename... Args> void emplace_back(Args&&... args)
    {
        NodeType* node = new NodeType(std::forward<Args>(args)...);
        if (!tail_) {
            front_ = tail_ = node;
        }
        else {
            tail_->next_ = node;
            tail_        = node;
        }
        ++size_;
    }

    void pop_front()
    {
        if (!front_) {
            return;
        }

        NodeType* node = front_;
        front_         = front_->next_;
        delete node;
        if (!front_) {
            tail_ = nullptr;
        }
        --size_;
    }

    void clear()
    {
        NodeType* ptr  = front_;
        NodeType* last = ptr;
        while (ptr) {
            last = ptr;
            ptr  = ptr->next_;
            delete last;
        }

        front_ = tail_ = nullptr;
        size_          = 0;
    }

    T& front() const
    {
        return front_->data_;
    }

    T& back() const
    {
        return tail_->data_;
    }

    template <typename Func> void for_each(Func&& f)
    {
        NodeType* ptr = front_;
        while (ptr) {
            f(ptr->data_);
            ptr = ptr->next_;
        }
    }

    void swap(List& that)
    {
        NodeType* tmp_node;

        tmp_node    = front_;
        front_      = that.front_;
        that.front_ = tmp_node;

        tmp_node   = tail_;
        tail_      = that.tail_;
        that.tail_ = tmp_node;

        uint64_t tmp_size = size_;
        size_             = that.size_;
        that.size_        = tmp_size;
    }

    bool empty()
    {
        return size_ == 0;
    }

    void append(List& that)
    {
        if (that.empty()) {
            return;
        }
        if (tail_) {
            tail_->next_ = that.front_;
        }
        else {
            front_ = that.front_;
        }

        tail_ = that.tail_;
        size_ += that.size_;
        that.front_ = that.tail_ = nullptr;
        that.size_               = 0;
    }

    T& operator[](uint64_t pos)
    {
        NodeType* ptr = front_;
        while (pos--) {
            ptr = ptr->next_;
        }
        return ptr->data_;
    }

    uint64_t size()
    {
        return size_;
    }

  private:
    NodeType* front_ = nullptr;
    NodeType* tail_  = nullptr;
    uint64_t  size_  = 0;
};

}  // namespace common_library

#endif