#include "hw.h"
#include "uart.h"
#include "ackermann.c"
#include "isa_test/test_isa.c"

int main(void)
{
    char c;
    uart_init();
    MyPrintf("uart test demo !!\r\n");
    MyPrintf("UART RX test: press a key...\r\n");
    while (1)
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
                MyPrintf("you pressed %c key!\r\n", c);
        }
    }
    return 0;
}