#ifndef LIST_H
#define LIST_H

#include <cstddef>
#include <cassert>

template<typename T>
class list_element {
    inline bool is_interior() {
        return prev != nullptr && next != nullptr;
    }

    inline bool is_tail() {
        return prev != nullptr && next == nullptr;
    }

public:
    list_element<T> *prev;
    list_element<T> *next;

    void remove() {
        assert(is_interior());
        prev->next = next;
        next->prev = prev;
    }

    void insert(list_element<T> *before) {
        assert(before->is_interior() || before->is_tail());
        prev = before->prev;
        next = before;
        before->prev->next = this;
        before->prev = this;
    }

    T *entry() {
        return (T *) ((char *) this - offsetof(T, element));
    }

};

template<typename T>
class list {
    list_element<T> head;
    list_element<T> tail;

    list_element<T> *begin() {
        return head.next;
    }

    list_element<T> *end() {
        return &tail;
    }

    list_element<T> *front() {
        assert(!empty());
        return head.next;
    }

    list_element<T> *back() {
        assert(!empty());
        return tail.prev;
    }

public:
    list() {
        head.prev = nullptr;
        head.next = &tail;
        tail.prev = &head;
        tail.next = nullptr;
    }

    bool empty() {
        return begin() == end();
    }

    void push_back(list_element<T> *element) {
        element->insert(end());
    }

    void push_front(list_element<T> *element) {
        element->insert(begin());

    }

    list_element<T> *pop_front() {
        auto *front_element = front();
        front_element->remove();
        return front_element;
    }

    list_element<T> *pop_back() {
        auto *back_element = back();
        back_element->remove();
        return back_element;
    }
};

#endif
