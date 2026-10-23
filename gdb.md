---
date: 2026-10-20T22:41:00
tags:
---
```terminal
gdb  ./bin -q
```

q 的作用是关闭提醒信息

# 常用指令
run |  r
break +  line  在line行设置断点
continue | c  继续运行到end
next | n    单步运行
info  break | info b查看断点
delete  xx  删除第几个断点
print + var | p  var 查看变量
x + var 查看变量地址
set var  xx =  xx 修改变量
x  + address 查看对应内存地址的值