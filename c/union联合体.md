
参考<https://www.bilibili.com/video/BV1754y1Y7Ut/?spm_id_from=333.1007.top_right_bar_window_default_collection.content.click&vd_source=5b63935761d030cb6c3e302e15b67dfa>

```c

union node_u
{
	struct 
	{
		int *left;
		int *right;
	}internal;
	double data[2];
}
```

二叉树中可以用union来同时表示节点和叶子




```c
union Data {
    uint32_t value;
    uint8_t bytes[4];
};

union Data data;

data.value = 0x12345678;

data.bytes[0] = 0xFF;
```

# 联合体的用法
联合体是 C 语言中的一种复合数据类型，与结构体不同，联合体的所有成员共享同一块内存，其大小需要满足最大成员的存储和对齐要求。

联合体主要有两个用途：
- 一是**节省内存**，当多种类型的数据不会同时使用时，可以共享存储空间；比如[[Queue_t]]用作信号量和队列时的互斥性
- 二是**访问数据的不同字节表示**，例如嵌入式通信中查看一个 32 位整数的四个字节。





