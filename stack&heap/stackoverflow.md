# 出现的场景
- 递归调用无法终止
```c
#include <stdio.h>
 void try_overflow(void) 
 {  
	 int a = 10; 
	 try_overflow(); 
 } 
 void main(void)
 {  
	 try_overflow(); 
 }
```
- 调用函数占用空间过大
```c
#include <stdio.h> 
#include <string.h>
#define N 8192*1024 
 void main(void) 
 {  
	 char arr[N]; 
	 memset(arr, 'c', sizeof(char)*N); 
	 for (int i = 0; i < N; i++) 
	 {  
		 printf("%c", arr[i]); 
	 } 
	 puts(""); 
 }

```
都会造成segmentation fault！
# 可能造成的影响

- 