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