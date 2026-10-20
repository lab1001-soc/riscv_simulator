
#define REGBYTES (4)
#define MSTATUS_PRV1  0x00001880
#define MSTATUS_MIE   0x00000008
#define MSTATUS_FS    0x00006000

    .text
    .option norvc
IRQ_TABLE:
    j trap                     //0
    j Default_Handler          //1
    j Default_Handler          //2
    j VECTOR_SOFT_IRQ          //3 software
    j Default_Handler          //4
    j Default_Handler          //5
    j Default_Handler          //6
    j SYSTICK_IRQHandler       //7 timerirq
    j Default_Handler          //8
    j Default_Handler          //9
    j Default_Handler          //10
    j Default_IRQHandler       //11 extirq
    j Default_Handler          //12
    j Default_Handler          //13
    j Default_Handler          //14
    j Default_Handler          //15
    j VECTOR_RTC_IRQ           //16
    j VECTOR_QSPI2_IRQ         //17
    j VECTOR_QSPI1_IRQ         //18
    j VECTOR_QSPI_IRQ          //19
    j VECTOR_UARTA_IRQ         //20
    j VECTOR_SADC_IRQ          //21
    j VECTOR_GDAC_IRQ          //22
    j VECTOR_TIMERGR0_IRQ      //23
    j VECTOR_WDT_IRQ           //24
    j VECTOR_SDMA_IRQ          //25
    j VECTOR_GPIO_IRQ          //26
    j VECTOR_AADC_IRQ          //27
    j VECTOR_AES_IRQ           //28
    j VECTOR_USB_IRQ           //29
    j VECTOR_BTTIMER_IRQ       //30
    j VECTOR_BT_IRQ            //31

chip_init:
    j flash_start
    nop

    .text
	.globl flash_start
	.globl delay
	.section .text.startup
flash_start:
	la sp, _stack

	/* set exception and irq mtvec */
	la a0,IRQ_TABLE
	ori a0,a0,1     //1: vector mode 0: direct mode
	csrw mtvec,a0

	/* enable fs */
	li t0,MSTATUS_FS
	csrs mstatus,t0
	csrw fcsr,x0

    /*load iram code*/
    la a0,_siitcm
    la a1,_sitcm
    la a2,_eitcm
    bgeu a1,a2,0f
3:
    lw t0, (a0)
    sw t0, (a1)
    addi a0,a0,4
    addi a1,a1,4
    bltu a1,a2,3b
0:
	/* Load data section */
	la a0, _sidata
	la a1, _sdata
	la a2, _edata
	bgeu a1,a2,2f
1:
	lw t0, (a0)
	sw t0, (a1)
	addi a0,a0,4
	addi a1,a1,4
	bltu a1,a2,1b
2:
	/* clear bss section */
	la a0,_sbss
	la a1,_ebss
	bgeu a0,a1,2f
1:
	sw zero,(a0)
	addi a0,a0,4
	bltu a0,a1,1b
2:

#ifndef __NO_SYSTEM_INIT
//        jal systeminit
#endif

#ifndef __NO_BOARD_INIT
//        jal board_init
#endif
	li t0, 0x00000800
	csrs 0x304,t0

	li t0, MSTATUS_MIE
	csrs mstatus, t0

	jal main

    /* never retch here*/
__exit:
	j __exit

.macro DISABLE_MIE
    csrc mstatus, MSTATUS_MIE
.endm

.macro ENABLE_MIE
    csrs mstatus, MSTATUS_MIE
.endm

.global __disable_irq
.type __disable_irq, %function
__disable_irq:
    csrrci a0, mstatus, 8
    andi a0,a0,8
    ret

.global __enable_irq
.type __enable_irq, %function
__enable_irq:
    csrs mstatus, a0
    ret

.align 3
.globl delay1
delay1:
    add a0,a0,-1
    bnez a0,delay1
    ret

.macro SAVE_CONTEXT
    addi sp,sp,-40*REGBYTES
    sw x1 , 0 *REGBYTES(sp) /* ra */
    sw x4 , 1 *REGBYTES(sp) /* tp */
    sw x5 , 2 *REGBYTES(sp) /* t0 */
    sw x6 , 3 *REGBYTES(sp) /* t1 */
    sw x7 , 4 *REGBYTES(sp) /* t2 */
    sw x10, 5 *REGBYTES(sp) /* a0 */
    sw x11, 6 *REGBYTES(sp) /* a1 */
    sw x12, 7 *REGBYTES(sp) /* a2 */
    sw x13, 8 *REGBYTES(sp) /* a3 */
    sw x14, 9 *REGBYTES(sp) /* a4 */
    sw x15, 10*REGBYTES(sp) /* a5 */
    sw x16, 11*REGBYTES(sp) /* a6 */
    sw x17, 12*REGBYTES(sp) /* a7 */
    sw x28, 13*REGBYTES(sp) /* t3 */
    sw x29, 14*REGBYTES(sp) /* t4 */
    sw x30, 15*REGBYTES(sp) /* t5 */
    sw x31, 16*REGBYTES(sp) /* t6 */

    fsw f0, 17*REGBYTES(sp) /* ft0 */
    fsw f1, 18*REGBYTES(sp) /* ft1 */
    fsw f2, 19*REGBYTES(sp) /* ft2 */
    fsw f3, 20*REGBYTES(sp) /* ft3 */
    fsw f4, 21*REGBYTES(sp) /* ft4 */
    fsw f5, 22*REGBYTES(sp) /* ft5 */
    fsw f6, 23*REGBYTES(sp) /* ft6 */
    fsw f7, 24*REGBYTES(sp) /* ft7 */
    fsw f10,25*REGBYTES(sp) /* fa0 */
    fsw f11,26*REGBYTES(sp) /* fa1 */
    fsw f12,27*REGBYTES(sp) /* fa2 */
    fsw f13,28*REGBYTES(sp) /* fa3 */
    fsw f14,29*REGBYTES(sp) /* fa4 */
    fsw f15,30*REGBYTES(sp) /* fa5 */
    fsw f16,31*REGBYTES(sp) /* fa6 */
    fsw f17,32*REGBYTES(sp) /* fa7 */
    fsw f28,33*REGBYTES(sp) /* ft8 */
    fsw f29,34*REGBYTES(sp) /* ft9 */
    fsw f30,35*REGBYTES(sp) /* ft10*/
    fsw f31,36*REGBYTES(sp) /* ft11*/
.endm

.macro RESTORE_CONTEXT
    lw x1 , 0 *REGBYTES(sp) /* ra */
    lw x4 , 1 *REGBYTES(sp) /* tp */
    lw x5 , 2 *REGBYTES(sp) /* t0 */
    lw x6 , 3 *REGBYTES(sp) /* t1 */
    lw x7 , 4 *REGBYTES(sp) /* t2 */
    lw x10, 5 *REGBYTES(sp) /* a0 */
    lw x11, 6 *REGBYTES(sp) /* a1 */
    lw x12, 7 *REGBYTES(sp) /* a2 */
    lw x13, 8 *REGBYTES(sp) /* a3 */
    lw x14, 9 *REGBYTES(sp) /* a4 */
    lw x15, 10*REGBYTES(sp) /* a5 */
    lw x16, 11*REGBYTES(sp) /* a6 */
    lw x17, 12*REGBYTES(sp) /* a7 */
    lw x28, 13*REGBYTES(sp) /* t3 */
    lw x29, 14*REGBYTES(sp) /* t4 */
    lw x30, 15*REGBYTES(sp) /* t5 */
    lw x31, 16*REGBYTES(sp) /* t6 */

    flw f0, 17*REGBYTES(sp) /* ft0 */
    flw f1, 18*REGBYTES(sp) /* ft1 */
    flw f2, 19*REGBYTES(sp) /* ft2 */
    flw f3, 20*REGBYTES(sp) /* ft3 */
    flw f4, 21*REGBYTES(sp) /* ft4 */
    flw f5, 22*REGBYTES(sp) /* ft5 */
    flw f6, 23*REGBYTES(sp) /* ft6 */
    flw f7, 24*REGBYTES(sp) /* ft7 */
    flw f10,25*REGBYTES(sp) /* fa0 */
    flw f11,26*REGBYTES(sp) /* fa1 */
    flw f12,27*REGBYTES(sp) /* fa2 */
    flw f13,28*REGBYTES(sp) /* fa3 */
    flw f14,29*REGBYTES(sp) /* fa4 */
    flw f15,30*REGBYTES(sp) /* fa5 */
    flw f16,31*REGBYTES(sp) /* fa6 */
    flw f17,32*REGBYTES(sp) /* fa7 */
    flw f28,33*REGBYTES(sp) /* ft8 */
    flw f29,34*REGBYTES(sp) /* ft9 */
    flw f30,35*REGBYTES(sp) /* ft10*/
    flw f31,36*REGBYTES(sp) /* ft11*/
    addi sp, sp, 40*REGBYTES
.endm

.macro SAVE_CSR_CONTEXT
    csrr t0,mepc
    csrr t1,mcause
    sw   t0,37*REGBYTES(sp) /* mepc */
    sw   t1,38*REGBYTES(sp) /* mcause */
.endm

.macro RESTORE_CSR_CONTEXT
    lw   t0,37*REGBYTES(sp) /* mepc */
    lw   t1,38*REGBYTES(sp) /* mcause */
    csrw mcause, t1
    csrw mepc,   t0
.endm

    .align 2
    .global Default_IRQHandler
    .weak   Default_IRQHandler
    .type   Default_IRQHandler, %function
Default_IRQHandler:

    SAVE_CONTEXT
    SAVE_CSR_CONTEXT

_IRQ:
    /* get irq */
    la t0,isr_table
    li t1,0x00008E64        /* t1: irq num */
    lw t1,0(t1)
    slli t2, t1, 2
    add t0, t0, t2
    lw t2, (t0)
    sw t1,39*REGBYTES(sp)
    ENABLE_MIE

    jalr t2         /* jump to irq */

    DISABLE_MIE
    /* clear pending mask*/
    lw t1,39*REGBYTES(sp)
    li t0,0x00008E64
    sw t1,(t0)
    nop
    nop
    /*enter new irq or recover */
    lw t1,4(t0)
    bgt t1,x0,_IRQ

    /*enable pri mie */
    li t0,MSTATUS_PRV1
    csrs mstatus,t0

    RESTORE_CSR_CONTEXT
    RESTORE_CONTEXT
    mret




/*trap staart */
.section .text.trap
.align 6
.globl trap
.weak trap
.type trap,%function
trap:
    /*check trap */
    csrr tp,mcause
    bgez tp,__trap //mcause>=0
    /*check plic irq */
    csrr tp,mcause
    blt tp,x0,.Interrupt
    /*never reatch here */
    j .

__trap:
    li t0,3
    beq tp,t0,ebreak_handle
    li t0,11
    beq tp,t0,ecall_handle

    SAVE_CONTEXT
    SAVE_CSR_CONTEXT
    /*save regs */
    jal trap_c
    /*never reatch here */
trap_end:
    j .

.Interrupt:
    addi sp,sp,-4
    sw t0,0x0(sp)

    la t0,IRQ_TABLE
    slli tp,tp,2
    add tp,t0,tp
    /*recover t0 */
    lw t0,0(sp)
    addi sp,sp,4

    jr tp


    .global trap_c
    .weak trap_c
    .type trap_c,%function
trap_c:
    j trap_c




.align  2
.weak   Default_Handler
.global Default_Handler
.type   Default_Handler, %function
Default_Handler:
    j       Default_Handler
    .size   Default_Handler, . - Default_Handler

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
    .macro  def_irq_handler handler_name
    .weak   \handler_name
    .globl  \handler_name
    .set    \handler_name, Default_Handler
    .endm

    def_irq_handler    ebreak_handle
    def_irq_handler    ecall_handle
    def_irq_handler    SYSTICK_IRQHandler
    def_irq_handler    RSVD_IRQHandler
    def_irq_handler    BT_IRQHandler
    def_irq_handler    SOFT_IRQHandler
    def_irq_handler    BTTIMER_IRQHandler
    def_irq_handler    UARTA_IRQHandler
    def_irq_handler    USB_IRQHandler
    def_irq_handler    AES_IRQHandler
    def_irq_handler    IIC_IRQHandler
    def_irq_handler    IIC1_IRQHandler
    def_irq_handler    HSPI_IRQHandler
    def_irq_handler    AADC_IRQHandler
    def_irq_handler    IIS_IRQHandler
    def_irq_handler    GPIO_IRQHandler
    def_irq_handler    SDMA_IRQHandler
    def_irq_handler    HWAKE_IRQHandler
    def_irq_handler    WDT_IRQHandler
    def_irq_handler    TIMERGR0_IRQHandler
    def_irq_handler    TIMERGR1_IRQHandler
    def_irq_handler    EPWM_IRQHandler
    def_irq_handler    GDAC_IRQHandler
    def_irq_handler    SADC_IRQHandler
    def_irq_handler    CRC_IRQHandler
    def_irq_handler    UARTC_IRQHandler
    def_irq_handler    UARTD_IRQHandler
    def_irq_handler    UARTB_IRQHandler
    def_irq_handler    FFT_IRQHandler
    def_irq_handler    QSPI_IRQHandler
    def_irq_handler    QSPI1_IRQHandler
    def_irq_handler    QSPI2_IRQHandler
    def_irq_handler    RTC_IRQHandler
    def_irq_handler    CAN_IRQHandler
    def_irq_handler    VECTOR_SOFT_IRQ
    def_irq_handler    VECTOR_RTC_IRQ
    def_irq_handler    VECTOR_QSPI2_IRQ
    def_irq_handler    VECTOR_QSPI1_IRQ
    def_irq_handler    VECTOR_QSPI_IRQ
    def_irq_handler    VECTOR_UARTA_IRQ
    def_irq_handler    VECTOR_SADC_IRQ
    def_irq_handler    VECTOR_GDAC_IRQ
    def_irq_handler    VECTOR_TIMERGR0_IRQ
    def_irq_handler    VECTOR_WDT_IRQ
    def_irq_handler    VECTOR_SDMA_IRQ
    def_irq_handler    VECTOR_GPIO_IRQ
    def_irq_handler    VECTOR_AADC_IRQ
    def_irq_handler    VECTOR_AES_IRQ
    def_irq_handler    VECTOR_USB_IRQ
    def_irq_handler    VECTOR_BTTIMER_IRQ
    def_irq_handler    VECTOR_BT_IRQ

    .align 2
    .globl  isr_table
isr_table:
        .long       RSVD_IRQHandler
        .long       BT_IRQHandler
        .long       SOFT_IRQHandler
        .long       BTTIMER_IRQHandler
        .long       UARTA_IRQHandler
        .long       USB_IRQHandler
        .long       AES_IRQHandler
        .long       IIC_IRQHandler
        .long       IIC1_IRQHandler
        .long       HSPI_IRQHandler
        .long       AADC_IRQHandler
        .long       IIS_IRQHandler
        .long       GPIO_IRQHandler
        .long       SDMA_IRQHandler
        .long       HWAKE_IRQHandler
        .long       WDT_IRQHandler
        .long       TIMERGR0_IRQHandler
        .long       TIMERGR1_IRQHandler
        .long       EPWM_IRQHandler
        .long       GDAC_IRQHandler
        .long       SADC_IRQHandler
        .long       CRC_IRQHandler
        .long       UARTC_IRQHandler
        .long       UARTD_IRQHandler
        .long       UARTB_IRQHandler
        .long       FFT_IRQHandler
        .long       QSPI_IRQHandler
        .long       QSPI1_IRQHandler
        .long       QSPI2_IRQHandler
        .long       RTC_IRQHandler
        .long       CAN_IRQHandler
        .long       RSVD_IRQHandler
