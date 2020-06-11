#ifndef COMMON_LIBRARY_LIST_H
#define COMMON_LIBRARY_LIST_H

namespace common_library {

template <typename T> class List;

template <typename T> class ListNode {
  public:
    friend class List<T>;

    template <typename... Args>
    ListNode(Args&&... args) : data_(std::forward<Args>(args)...)
    {
    }

  private:
    T         data_;
    ListNode* next_ = nullptr;
};

template <typename T> class List {
  public:
    typedef ListNode<T> NodeType;

    List() {}

    List(List&& other)
    {
        swap(other);
    }

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
        else {
            NodeType* node = front_;
            front_         = node->next_;
            delete node;
            if (!front_) {
                tail_ = nullptr;
            }
            --size_;
        }
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

    T& front()
    {
        return front_->data_;
    }

    T& back()
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

    void swap(List& other)
    {
        NodeType* tmp_node;

        tmp_node     = front_;
        front_       = other.front_;
        other.front_ = tmp_node;

        tmp_node    = tail_;
        tail_       = other.tail_;
        other.tail_ = tmp_node;

        uint64_t tmp_size = size_;
        size_             = other.size_;
        other.size_       = tmp_size;
    }

    bool empty()
    {
        return size_ == 0;
    }

    void append(List& other)
    {
        if (other.empty()) {
            return;
        }
        if (tail_) {
            tail_->next_ = other.front_;
        }
        else {
            front_ = other.front_;
        }

        tail_ = other.tail_;
        size_ += other.size_;
        other.front_ = other.tail_ = nullptr;
        other.size_                = 0;
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