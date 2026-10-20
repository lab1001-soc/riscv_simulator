#include "rtthread.h"
// #include "register.h"
#include "riscv-ops.h"
#include "hw.h"
#include "software.h"
extern volatile rt_ubase_t  rt_interrupt_from_thread;
extern volatile rt_ubase_t  rt_interrupt_to_thread;
extern volatile rt_uint32_t rt_thread_switch_interrupt_flag;

void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to)
{
    if (rt_thread_switch_interrupt_flag == 0)
        rt_interrupt_from_thread = from;

    rt_interrupt_to_thread = to;
    rt_thread_switch_interrupt_flag = 1;

    /*trig soft interrupt*/
    sw_irq_set();
    set_csr(mie,1<<3);
    // ext_irq_init(INTR_SOFT,1,ENABLE);
    // rt_kprintf("enter rt_hw_context_switch_interrupt\n");
    return ;
}

void SOFT_IRQHandler(void)
{
    sw_irq_clear();
    // CORE_CHIP_SYS->rtc_systick_sync_cmd.bit.software_irq =0;
    // rt_kprintf("SOFT_IRQ\n");
}