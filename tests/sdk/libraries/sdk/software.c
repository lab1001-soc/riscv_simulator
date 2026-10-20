/*
 * software.c - 软件中断操作实现
 */

#include "software.h"

void sw_irq_set(void)
{
    SW_IRQ_CTRL = 1;
}

void sw_irq_clear(void)
{
    SW_IRQ_CTRL = 0;
}

int sw_irq_state(void)
{
    return (int)SW_IRQ_CTRL;
}
