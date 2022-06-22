#ifndef LIST_H
#define LIST_H

#include <cstddef>
#include <cassert>

class list_element {
    inline bool is_interior() {
        return prev != nullptr && next != nullptr;
    }

    inline bool is_tail() {
        return prev != nullptr && next == nullptr;
    }

public:
    list_element *prev;
    list_element *next;
//    T *object;
//
//    list_element(T *object) : object(object) {}

    void remove() {
        assert(is_interior());
        prev->next = next;
        next->prev = prev;
    }

    void insert(list_element *before) {
        assert(before->is_interior() || before->is_tail());
        prev = before->prev;
        next = before;
        before->prev->next = this;
        before->prev = this;
    }


};

class list {
    list_element head;
    list_element tail;

    list_element *begin() {
        return head.next;
    }

    list_element *end() {
        return &tail;
    }

    list_element *front() {
        assert(!empty());
        return head.next;
    }

    list_element *back() {
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

    void push_back(list_element *element) {
        element->insert(end());
    }

//    void push_front(list_element<T> *element) {
//        element->insert(begin());
//
//    }

    list_element *pop_front() {
        auto front_element = front();
        front_element->remove();
        return front_element;
    }

//    T *pop_back() {
//        auto *back_element = back();
//        back_element->remove();
//        return back_element->entry();
//    }
};

#endif
