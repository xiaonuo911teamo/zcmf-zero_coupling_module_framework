## 自定义数据类型介绍

多线程并行是本框架的特点之一, 所以通常都会对变量的线程安全性有要求, 所以这里就实现了常用的几种线程安全的数据结构模板.使用时,直接传入模板类型即可.目前已经实现的结构有, 针对普通变量的DoubleBufferData, 针对队列的DoubleBufferedQueue, 针对数组的DoubleBufferedVector. 

### 快速上手

所有的结构中为了方便使用都引用了下面的定义.

1. 线程锁 _data_mutex: 保证线程安全
2. 原子变量 _data_buffer_updated: 表示数据是否进行的更新, 相对于上一次使用get_data来说.
3. 原子变量 _has_data: 表示在构造出来之后,是否有进行过赋值, 即调用set_data
4. 赋值函数 set_data: 注入一个新的值, 更新_data_buffer_updated和_has_data
5. 取值函数 get_data: 取出当前值, 更新_data_buffer_updated为false
6. 窥视函数 peek_data: 区别于get_data, peek_data不会去更新内部状态, 只是偷看一眼数据.
7. 查看数据更新状态函数 is_updated
8. 查看是否已载入数据函数 has_data

### DoubleBufferData 使用示例

> 使用经典的消费-生产的例子作为演示用法.
> 1. 生产线程每次产生一个蛋糕, 当蛋糕被取走后, 进行生产下一个. 
> 2. 生产一个蛋糕用时2s.
> 3. 消费消费线程, 按计划来取蛋糕, 如此时没有蛋糕, 则等待其生产.

```
struct Cake {
    enum Flavors {
        Sponge,
        Chiffon,
        AngelFool,
        Pound,
        Cheese,
        Mousse
    };
    Flavors flavor;
    int size;
    Cake() : flavor(Flavors::Sponge), size(6) {}
};

DoubleBufferData<Cake> cake; // 表示蛋糕

std::thread producer([&]() {
    while(1) {
        if (!cake.is_update()) {        // get_data 和 set_data会自动更新内部的is_updated状态
            AppUtil::sleep_ms(2000);
            cake.set_data(Cake());       // 内部带有线程锁, 无需额外设置
        }
    }
});

std::thread customer([&]() {
    int plan[6] = {1, 2, 5, 7, 8, 10}; // 计划第几秒去取蛋糕
    for (int i = 0; i < 6;) {
        static int64_t init_time = AppUtil::get_current_ms();
        while (AppUtil::get_current_ms() - init_time >= a[i]*1000) {
            if (cake.is_update()) {
                Cake tmp = cake.get_data();     // 内部带有线程锁, 无需额外设置
                INFO() << "get a cake. " << (int)tmp.flavor;
                i++;                            // 切换到下一个人
            } else {
                INFO() << "waiting for cake ...";
            }
        }
    }
});
```

### 仍需优化部分

1. DoubleBufferedQueue和DoubleBufferedVector, 对swap的实现部分, 存在争议, 具体参考定义部分, [double_buffered_queue](./double_buffered_queue.hpp), [double_buffered_vector](./double_buffered_vector.hpp)

2. vector queue还有很多操作没有支持, 像迭代器, 随机访问等.
