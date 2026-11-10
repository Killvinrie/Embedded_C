假设一个简单的内存分配器使用链表管理空闲内存。

```c
typedef struct Block
{
    size_t size;
    struct Block *next;
} Block;

Block *freeList;
```

假设当前空闲链表为：

```
freeList
   │
   ▼
┌───────────┐
│ Block A   │
│ 100 Bytes │
└─────┬─────┘
      │
      ▼
┌───────────┐
│ Block B   │
│ 200 Bytes │
└─────┬─────┘
      │
      ▼
     NULL
```

现在 Task1 和 Task2 同时申请内存：

```c
// Task1
void *p1 = malloc(80);

// Task2
void *p2 = malloc(80);
```

假设分配器采用最简单的策略：找到合适的块，然后从空闲链表中将它移除。

它内部的伪代码可能是：

```c
void *my_malloc(size_t size)
{
    Block *p = freeList;

    if (p != NULL && p->size >= size)
    {
        freeList = p->next;

        return (void *)(p + 1);
    }

    return NULL;
}
```

这只是一个说明并发问题的简化分配器，没有实现分块、对齐和完整的内存管理逻辑。


# malloc对于嵌入式系统存在的问题:
- 在小型嵌入式系统上并不总是可用。
- 实现体积可能较大，会占用宝贵的代码空间。
- ==通常不是线程安全的。
- ==不具确定性；每次调用执行所需时间可能不同。
- ==可能发生碎片问题（堆中空闲内存足够，但没有可用连续块）。
- 可能使链接器配置更复杂。
- 若允许堆空间增长到与其他变量使用的内存重叠，可能导致难以调试的错误。