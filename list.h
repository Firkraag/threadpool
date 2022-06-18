#ifndef LIST_H
#define LIST_H

#include <cstddef>
#include <cassert>

class list_element {
    inline bool is_interior();

    inline bool is_tail();

public:
    list_element *prev;
    list_element *next;

    void remove();

    void insert(list_element *before);

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

    void push_front(list_element *element) {
        element->insert(begin());

    }

    list_element *pop_front() {
        list_element *front_element = front();
        front_element->remove();
        return front_element;
    }

    list_element *pop_back() {
        auto *back_element = back();
        back_element->remove();
        return back_element;
    }

//    static T *entry(list_element *element) {
//        return (T *) ((char *) element - offset);
//    }
//    ~list() {
//
//    }
};

#define list_entry(LIST_ELEM, STRUCT, MEMBER) \
((STRUCT *) ((char *) LIST_ELEM - offsetof(STRUCT, MEMBER)))
#endif
