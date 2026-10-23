sizeof关键字不是函数
在编译的时候直接给出结果
比如
```c
int func()
{
	printf("hello world~");
	return 10;
}

int main()
{
	printf("sizeof = %ld \n", sizeof(func()));
}
```
上面的程序并不会打印出hello world~