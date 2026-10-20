#ifndef __CONFIG_H__
#define __CONFIG_H__

/* 关闭调试输出 */
// #define DEBUG_FETCH
// #define MEM_TRACE
// #define DEBUG_TRAP


/* 键盘输入检查间隔（指令条数），避免 _kbhit() 系统调用拖慢仿真 */
#define INPUT_CHECK_INTERVAL  100000

#endif
