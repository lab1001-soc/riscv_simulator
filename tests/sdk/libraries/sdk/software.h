/*
 * software.h - 软件中断操作接口
 * SW_IRQ_CTRL: 0x8400 (写1置MSIP, 写0清MSIP, 读返回MSIP状态)
 */

#ifndef SW_IRQ_H__
#define SW_IRQ_H__

#include <stdint.h>

#define SW_IRQ_BASE      0x8400
#define SW_IRQ_CTRL_ADDR (SW_IRQ_BASE + 0x00)

#define SW_IRQ_CTRL  (*(volatile uint32_t *)SW_IRQ_CTRL_ADDR)

void sw_irq_set(void);
void sw_irq_clear(void);
int  sw_irq_state(void);

#endif /* SW_IRQ_H__ */
