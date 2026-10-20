#include "hw.h"
#include "MyPrintf.h"
#include "rtthread.h"

static void led1_thread_entry(void* parameter)
{
    while (1) {
        //rt_kprintf("thread_entry1!\n");
        rt_thread_mdelay(500);
    }
}

extern volatile rt_uint32_t rt_thread_switch_interrupt_flag;
void main(void)
{
    rt_kprintf("main_entry\n");
    rt_thread_t tid1;
    tid1 = rt_thread_create("led1", led1_thread_entry, RT_NULL,
                            2048, 10, 10);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);


    while(1)
    {
        //rt_kprintf("main_entry123\n");
        rt_thread_mdelay(2000);
    }
}