# 架构
本项目主要有四个数据结构: threadpool, future, list, worker. 

```cpp
typedef void *(*fork_join_task_t)(threadpool *pool, void *data); // 任务函数

class threadpool {
private:
    worker *workers; // 工作线程列表
public:
    pthread_mutex_t lock;
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
```cpp
void *worker_thread(void *args) { // 工作线程执行的代码
    auto *pool = (threadpool *) args;
    while (true) {
        pthread_mutex_lock(&pool->lock);
        auto *fut = (future *) pool->global_queue.pop_front();
        fut->status = IN_PROGRESS;
        pthread_mutex_unlock(&pool->lock);
        fut->result  = (fut->task)(pool, fut->data);
        fut->status = COMPLETED;
        pthread_cond_signal(&fut->done);
    }
    return nullptr;
}
void *future::get() {
    pthread_mutex_lock(&pool->lock);
    if (status == NOT_STARTED) {
        remove();
        status = IN_PROGRESS;
        pthread_mutex_unlock(&pool->lock);
        result = task(pool, data);
        status = COMPLETED;
        pthread_cond_signal(&done);
    } else {
        while (status != COMPLETED) {
            pthread_cond_wait(&done, &pool->lock);
        }
        pthread_mutex_unlock(&pool->lock);
    }
    return result;
}
```
future::get有一个更简单的写法:
```cpp
void *future::get() {
    if task result is not ready
        wait for signal
    return result;
}
```

这样的话, 只有工作线程一处地方需要加锁, future 和 task 可以分离, 添加到队列中的是 task, task 执行完毕发送信号给 future,
future只有两个状态:REDAY 和 NOT_READY, 甚至队列都不用自定义的 list, 可以用现成的std::queue.

为什么我们不采用更简单的写法呢, 因为本项目有一个核心需求: 在任务中能够提交子任务. 设想一个场景: n 个工作线程执行 n 个任务, 
n 个任务都提交子任务, 任务调用 future::get等待子任务的结果, 然而没有多余的工作线程去执行子任务, future::get 卡死在永远等不来的信号, 
工作线程也因此卡死. 破局的方法是future::get 执行的时候, 如果发现还在队列中, 就自己从队列中删除, 自己执行任务, 获得结果.

这导致了好几个结果: 
- future 需要三个状态: NOT_STARTED(还在队列中), IN_PROGRESS, COMPLETED.
- future::get需要加锁, 需要加锁的地方有两处, 代码更加复杂.
- future::get 中会把自身从队列中删除, 所以不能用现成的std::queue, 而要用自定义的链表.

很显然, worker_thread和 future::get 中从队列中删除 future 需要加锁, 为什么future 状态设为IN_PROGRESS也要加锁, 
这是因为会有这种状况发生: worker_thread取出任务执行, future 状态没有设为IN_PROGRESS, future::get同时执行, 发现 future 的状态是 NOT_STARTED,
以为任务还没开始, 也试图把自身从队列中删除, 但是这时 future 已经不在队列中了.
执行任务不用加锁, 因为子任务会等待任务占有的锁,任务等待子任务的结果, 会死锁, 这就是加锁复杂的地方, 多一点少一点都不行, 必须刚刚好.

future::get使用条件锁阻塞等待 future 的状态变为 COMPLETED, worker_thread在任务完成后发送信号唤醒条件锁.

条件锁的使用有很多个坑, 这里仅举两个:
wait 必须有配套的 signal, 且 signal 必须在 wait 之后调用, 否则 wait 没有 signal 来唤醒, 将陷入永远的阻塞.
条件锁只能这样写:
```cpp
// 生产者
conditonal.lock()
prepare resource
conditional.signal()
conditional.unlock() 

// 消费者
condtional.lock()
while (resource is not ready) {
    conditional.wait()
}
condtional.unlock()
```
在多消费者的场景中, 生产者可能用 signal唤醒多个消费者, 先唤醒的消费者消费掉资源, 后面的消费者发现资源没准备好, 
继续阻塞, 等待下一次唤醒, 如此循环, 直到资源准备好了才往下执行.

上面的 worker_thread 和 future::get有一个隐蔽的 bug: 如果 future::get 执行到`while (status != COMPLETED) {`切换到工作线程执行
```cpp
fut->result  = (fut->task)(pool, fut->data);
fut->status = COMPLETED;
pthread_cond_signal(&fut->done);
```
再切换回来执行`pthread_cond_wait(&done, &pool->lock);`, 就会造成 signal 先于 wait 执行,
wait 永远阻塞, 也就是我说的第一个坑.

上面的写法等于生产者没有锁:
```cpp
prepare resource
conditional.signal()
```
造成 signal 可以插入`while (resource is not ready) {`和wait 之间, 所以解决的办法是再加一个锁:
```cpp
void *worker_thread(void *args) { // 工作线程执行的代码
    auto *pool = (threadpool *) args;
    while (true) {
        pthread_mutex_lock(&pool->lock);
        auto *fut = (future *) pool->global_queue.pop_front();
        fut->status = IN_PROGRESS;
        pthread_mutex_unlock(&pool->lock);
        fut->result  = (fut->task)(pool, fut->data);
        pthread_mutex_lock(&pool->lock);
        fut->status = COMPLETED;
        pthread_cond_signal(&fut->done);
        pthread_mutex_unlock(&pool->lock);
    }
    return nullptr;
}
void *future::get() {
    pthread_mutex_lock(&pool->lock);
    if (status == NOT_STARTED) {
        remove();
        status = IN_PROGRESS;
        pthread_mutex_unlock(&pool->lock);
        result = task(pool, data);
        pthread_mutex_lock(&pool->lock);
        status = COMPLETED;
        pthread_cond_signal(&done);
        pthread_mutex_unlock(&pool->lock);
    } else {
        while (status != COMPLETED) {
            pthread_cond_wait(&done, &pool->lock);
        }
        pthread_mutex_unlock(&pool->lock);
    }
    return result;
}
```

# 内存管理
future对象在三处地方被使用: 工作线程中从队列中取出执行并把任务结果存入 future, threadpool 析构时可能操作队列上剩余的future,
比如取出 future 一一执行, threadpool.submit的调用方获得 future 并调用 future.get 获得任务结果.

很明显, 不能在工作线程中刚把任务结果存入 future就析构. threadpool 析构时可以析构队列上剩余的 future, 但无法析构全部的 future.
所以我们只有一个选择: 析构threadpool::submit返回的 future.

这带来一个全新的问题: 在一个工作线程中把结果存入 future 和另一个线程析构 future 是同时进行的, 或者析构开始时 future 还在队列中, 
这两者都迫使我们在析构函数中使用threadpool.lock, 因此 future 必须先于 threadpool 析构, threadpool开始析构时, 所有 future 都已经被
析构, 队列为空, 这就是为什么我们在 threadpool 的析构函数开头加了一句断言`assert(global_queue.emp
ty());`.

```cpp
future::~future() {
    pthread_mutex_lock(&pool->lock);
    if (status == NOT_STARTED) {
        remove();
        pthread_mutex_unlock(&pool->lock);
    } else {
        while (status != COMPLETED) {
            pthread_cond_wait(&done, &pool->lock);
        }
        pthread_mutex_unlock(&pool->lock);
    }
    pthread_cond_destroy(&done);
}
```

下面我们来看一下 future 析构函数的具体实现. 首先我们要获得 threadpool.lock, 然后检查 future 的状态, 
如果是 NOT_STARTED, 只需要从队列删除 future, 没必要执行任务, 因为如果任务的返回值指向堆上的内存,
需要释放内存, 如果不指向, 则不需要释放, 我们在 future 析构函数中无法就这点做出确切的判断. 如果状态不是NOT_STARTED,
我们等待任务完成, 免得工作线程将结果存入 future, future 却已经被析构了, 造成 segment fault, 我们无法处理任务的返回值,
即无法确定是不是要释放内存, 所以这里可能有内存泄露.

要想没有内存泄露, 必须严格遵循如下的编程范式:
```cpp
pool = threadpool(nthread);
fut1 = pool.submit(task1);
fut2 = pool.submit(task2);
result1 = fut1.get()
result2 = fut2.get()
destruct fut1
destruct fut2
destruct pool
free result1 if it is allocated on heap
free result2 if it is allocated on heap
```
先创建线程池, 然后提交任务, 对于每一个 future, 都调用 future.get 获取返回值, 保证任务执行完成,
然后析构 future, 最后析构线程池. 这样的话, 到执行 future 析构函数时, future 的状态都是 COMPLETED,
future 的析构函数可以简化成:

```cpp
future::~future() {
    pthread_cond_destroy(&done);
}
```
之所以不这么写是因为我们用unique_ptr包裹threadpool.submit返回的 future 来保证 future 先于 threadpool 被析构,
但是无法保证线程池的使用者不会因为因为疏忽等原因忘记调用 future.get. 假如我们偷懒使用了上面简化的析构函数, 看似没有锁
效率提高了, 但是只要遗漏了一个 future.get, 就有可能工作线程把结果存入已经被析构的 future, 造成 segment fault 程序崩溃.
而采用上面更复杂的析构函数, 同样面对遗漏了 future.get, 后果也就是内存泄露. 内存泄露的后果可大可小,
长期运行的程序当然不能有内存泄露, 然而对于计算斐波那契数列这种得到了结果就退出的程序, 内存泄漏其实无关紧要.

如果任务的返回值类型不再是泛型, 而是一个固定类型, 则上面简化的析构函数依然有 segment fault 的风险,
而更复杂的方案依然正确, 且不再有内存泄露, 说明这个方案具有通用性.
