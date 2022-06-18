//
// Created by wq on 2022/6/16.
//

#include "thread_pool_list.h"




inline bool list_element::is_interior() {
    return prev != nullptr && next != nullptr;
}

inline bool list_element::is_tail() {
    return prev != nullptr && next == nullptr;
}

list_element *list_element::remove() {
    assert(is_interior());
    prev->next = next;
    next->prev = prev;
    return next;
}

void list_element::insert(list_element *before) {
    assert(before->is_interior() || before->is_tail());
    prev = before->prev;
    next = before;
    before->prev->next = this;
    before->prev = this;
}
