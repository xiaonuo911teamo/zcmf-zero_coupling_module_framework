## messager 模块说明

此模块中定义了一种消息交互方式，publish负责推送，subscribe负责订阅。其原理是通过subscribe在静态存储区定义了函数，通过publish指定其函数名称，即可调用此静态函数。

此模块中还定义了一种服务的实现方式。add_server_func用于注册服务，get_server_func取得服务，另外还有has_server和remove_server_func可以用于判断服务是否存在和删除服务。

> 消息交互和服务两种方式看起来确有相似之处，但是其含义有明显不同。消息交互目的是推送参数，然后由订阅的函数进行相应的处理，不同的地方，也可以对相同的消息做出不同的处理。而服务由一个模块提供，其实现也只能有一个，在其他模块中也只能是补充参数进行调用。

### 使用方式

1. 消息交互
    在一处或多处使用subscribe订阅消息，并设置对应的处理方式。在另一处使用publish推送消息，此时会自动调用多处subscribe的处理方式。
2. 服务
    在一处使用add_server_func注册服务，在另一处使用get_server_func，获取服务并给予适当的参数，获取函数的反馈或为服务中更新一些东西。

> 注：无论哪一种，单独调用一方面都是安全的。只使用subscribe没有publish，则不会触发调用。只有publish则不会做任何处理，也不会有问题。服务也是类似。

### 服务机制使用示例

设置场景: 模块A中提供生产<font color=red>香飘飘奶茶</font>(赞助商)的服务, 模块B中,需要使用生产奶茶的服务.

```
// 奶茶的定义
class Meco {
public:
    // 提供大中小杯子, 和甜度要求. 生成相应的奶茶
    Meco(int _size, float _sugar) : size(_size), sugar(_sugar) {}

private: 
    int size;
    float sugar;
};
```

```
// 模块A
Messager::add_server_func<Meco(int, float)>("get_a_meco", [](int size, float sugar) -> Meco {
    return Meco(size, sugar);
});
```

```
// 模块B, 
if (Messager::has_server("get_a_meco")) {
    auto get_a_meco = get_server_func("get_a_meco");
    // 这个if是必须要加的, 上面的has_server是可以不加的. 在多线程中, 存在在其他线程调用remove_server_func函数的可能, 导致这里获得空函数, 直接调用出错.
    // get_server_func调用是安全的, 如果服务不存在, 只是返回空指针而已, 所以has_server是可以省略的
    if (get_a_meco) {   
        auto my_meco = get_a_meco(2, 0.8); // 中杯, 8分甜
        // .....
    }
}
```

### 消息机制使用示例

设置场景: 在马戏团中的一只海豚, 听驯兽师的指令, 做出规范动作. 模块A模拟驯兽师发出指令, 模块B模拟海豚响应指令.

```
// 驯兽师
// 设定一次 "play_ball" 指令持续ns, 听到"stop"指令或超过ns, 则停止.

Messager::publish("play_ball", 10);
sleep(7);
Messager::publish("stop");l

```

```
// 海豚
Messager::subcribe<int>("play_ball", [&](int sec) {
    _is_stop = true;
    int64_t start = AppUtil::get_current_ms();
    while (AppUtil::get_current_ms() - start < sec * 1000 && (!_is_stop)) {
        play_ball();    // 用时很短, ms级别
    }
});

Messager::subcribe("stop", [&](){
    _is_stop = true;
});
```