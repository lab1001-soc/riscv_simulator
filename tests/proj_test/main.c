#include "MyPrintf.h"
#include "testcase/ackermann.c"
#include "uart/uart.h"
extern int test_isa(void);

void main()
{
    char c;
    uart_init();
    MyPrintf("Hello World!\r\n");
    MyPrintf("UART RX test: press a key...\r\n");
    while(1)
    {
        if(!uart_rx_valid())
            continue;

        c = uart_getchar();
        switch(c)
        {
            case '1':
                MyPrintf("ackermann_test\r\n");
                ackermann_test();
                break;
            case '2':
                MyPrintf("test_isa\r\n");
                test_isa();
                break;
            default:
                MyPrintf("unknown command '%c' (0x%02x)\r\n", c, c);
        }
    }
}





