# 架构
本项目主要有四个数据结构: threadpool, future, list, worker. 

```cpp
typedef void *(*fork_join_task_t)(threadpool *pool, void *data); // 任务函数

class threadpool {
private:
    worker *workers; // 工作线程列表
public:
    list global_queue; // 任务队列
    std::unique_ptr<future> submit(fork_join_task_t task, void *data);
}
```

threadpool代表线程池, threadpool::threadpool创建任务队列和 n 个工作线程, threadpool::sumbit 提交任务到队列, 返回 future.

`fork_join_task_t`传入线程池和以 void 指针表示的任意类型的数据, 只要调用threadpool::sumbit的时候把数据准备好, 传入数据的地址即可, 返回以 void 指针表示的任意类型的返回值, void 指针可以指向全局变量的地址, 可以包含 long 或者 double 的值(两者都是八个字节, 可以放入指针), 可以在任务函数内调用一次 malloc, 放入任务结果, 返回地址, 使用任务返回值的一方要记得调用 free, 以免内存泄漏. 


worker 是工作线程, 从队列当中取出任务执行. 

```cpp
typedef enum {
    NOT_STARTED = 0,
    IN_PROGRESS = 1,
    COMPLETED = 2
} status_t; // 任务状态

class future {
    status_t status = NOT_STARTED; 
    void *data; // 任务函数的数据
    fork_join_task_t task; // 任务函数
    void *result; // 任务的结果
    threadpool *pool;
    list_element elem;
};
```

future 是对任务和任务结果的封装, future::get 返回任务的结果。

```cpp
class list_element {
public:
    list_element *prev;
    list_element *next;
}
class list {
    list_element head;
    list_element tail;
}
```

list是链表, 这里用作任务的队列. list的节点是list_element, 在threadpool::submit, 我们实际上创建了 future 对象, 把 future.elem 插入队列.
工作线程从队列取出任务时, 取出的是 list_element, 根据下面的宏:
```cpp
list_entry(list_element_address, struct, member) ((struct *) ((list_element_address) - offset(struct, member)))
```
可以得到future 对象的地址, 也就是说我们可以通过嵌入 future 对象的list_element的地址, 和 list_element在 future 中的相对偏移量, 获得 future 对象的地址, 调用 future.task, 传入future.pool和future.data, 把结果存入 future.result.

整个架构还是挺简单的, 难点在于泛型和加锁以及内存管理.

# 泛型
我们的任务实现了泛型, 因为数据和任务返回值都可以是任意类型, 其缺点是可能有一次 malloc 和一次 free, 影响性能, 要把任务的结果类型从 void 指针强制转换成真正的类型, 跳过了类型检查, 所以程序的正确性完全靠程序员自己小心, 其优点是为所有的任务只生成一份机器码. 下面我们看一下用模板实现的任务泛型:
```cpp
auto submit(f, args...) -> std::future<decltype(f(args...))> {
    T = decltype(f(args...));

    // Create a function with bounded parameters ready to execute
    std::function<T()> func = std::bind(f, args...);

    auto task_ptr = std::packaged_task<T()>(func);

    // Wrap packaged task into void function
    std::function<void()> wrapper_func = [task_ptr]() {
      (*task_ptr)(); 
    };

    // Enqueue generic wrapper function
    queue.enqueue(wrapper_func);

    // Return future from promise
    return task_ptr->get_future();
  }
};
```
没有 malloc 和 free, 代价是每一类型的任务, 编译器都会生成一份submit 的机器码, 代码膨胀非常严重.

前面的队列也是一种泛型, 只要把list_element嵌入一个类, 这个类就添加到队列.

我们可以做一个变形, 把list_element嵌入到 future 的起始位置, 衍生类继承基类的时候, 其内存布局是基类的成员在衍生类之前, 所以经过这么一变形, 
我们实际上让 future 继承了 list_element , 则 future 的定义变成了:
```cpp
class future : public list_element {
    status_t status = NOT_STARTED; 
    void *data; // 任务函数的数据
    fork_join_task_t task; // 任务函数
    void *result; // 任务的结果
    threadpool *pool;
};
```
这样做的好处是以前future 和 list 之间的互动都要通过 future.elem这个媒介, 现在可以直接进行, 代码简化了.

# 加锁

# 内存管理ls 