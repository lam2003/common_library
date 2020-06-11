#ifndef COMMON_LIBRARY_LIST_H
#define COMMON_LIBRARY_LIST_H

namespace common_library {

template <typename T> class List;

template <typename T> class ListNode {
  public:
    friend class List<T>;

    ListNode(const T& data)
    {
        data_ = data;
        next_ = nullptr;
    }

    ListNode(T&& data)
    {
        data_ = std::forward<T>(data);
    }

  private:
    T         data_;
    ListNode* next_;
};

template <typename T> class List {
  public:
    typedef ListNode<T> NodeType;

    List()
    {
        front_ = tail_ = nullptr;
        size_          = 0;
    }

    void emplace_front(T&& data)
    {
        NodeType* node = new NodeType(std::forward<T>(data));
        if (!front_) {
            front_ = tail_ = node;
        }
        else {
            node->next_ = front_;
            front_      = node;
        }
        ++size_;
    }

    void emplace_front(const T& data)
    {
        NodeType* node = new NodeType(data);
        if (!front_) {
            front_ = tail_ = node;
        }
        else {
            node->next_ = front_;
            front_      = node;
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

    void emplace_back(T&& data)
    {
        NodeType* node = new NodeType(std::forward<T>(data));
        if (!tail_) {
            front_ = tail_ = node;
        }
        else {
            tail_->next_ = node;
            tail_        = node;
        }
        ++size_;
    }

    void emplace_back(const T& data)
    {
        NodeType* node = new NodeType(data);
        if (!tail_) {
            front_ = tail_ = node;
        }
        else {
            tail_->next_ = node;
            tail_        = node;
        }
        ++size_;
    }

  private:
    NodeType* front_;
    NodeType* tail_;
    uint64_t  size_;
};

}  // namespace common_library

#endif