```c
prvCopyDataToQueue(
    pxQueue,
    pvItemToQueue,
    xCopyPosition
);
```


```c
memcpy(
    (void *)pxQueue->pcWriteTo,
    pvItemToQueue,
    (size_t)pxQueue->uxItemSize
);

pxQueue->pcWriteTo += pxQueue->uxItemSize;

if (pxQueue->pcWriteTo >= pxQueue->u.xQueue.pcTail)
{
    pxQueue->pcWriteTo = pxQueue->pcHead;
}

pxQueue->uxMessagesWaiting++;
```

- 通过[[memcpy]]将消息复制到队列中
- 修改队列写指针（注意回绕）
- 队列消息数量++
