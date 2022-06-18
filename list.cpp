#include "list.h"




inline bool list_element::is_interior() {
    return prev != nullptr && next != nullptr;
}

inline bool list_element::is_tail() {
    return prev != nullptr && next == nullptr;
}

void list_element::remove() {
    assert(is_interior());
    prev->next = next;
    next->prev = prev;
}

void list_element::insert(list_element *before) {
    assert(before->is_interior() || before->is_tail());
    prev = before->prev;
    next = before;
    before->prev->next = this;
    before->prev = this;
}
