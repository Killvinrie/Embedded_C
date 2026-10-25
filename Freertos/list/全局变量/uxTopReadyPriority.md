`uxTopReadyPriority `holds the priority of the highest priority ready

`
`#define taskRECORD_READY_PRIORITY( uxPriority ) `

```c
do { \

if( ( uxPriority ) > uxTopReadyPriority ) \

{ \

uxTopReadyPriority = ( uxPriority ); \

} \

} while( 0 ) /* taskRECORD_READY_PRIORITY */

```
