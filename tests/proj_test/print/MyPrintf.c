
#include <stdarg.h>
#include "../types.h"
#define NULL 0
//*****************************************************************************
//
//! A simple  MyPrintf function supporting \%c, \%d, \%p, \%s, \%u,\%x, and \%X.
//!
//! \param format is the format string.
//! \param ... are the optional arguments, which depend on the contents of the
//! \return None.
//
//*****************************************************************************

//#define SIM_PLATFORM

void print_char(int data)
{
    *(volatile byte *)0x8200 = data;
}


void printfsend(uint8_t *buf, int len)
{
    // uint8_t printbuf[256];
    for (int i = 0; i < len; i++)
    {
        print_char(buf[i]);
    }

}

void MyPrintf(const char *format, ...)
{
    static const int8_t *const g_pcHex1 = "0123456789abcdef";
    static const int8_t *const g_pcHex2 = "0123456789ABCDEF";

    uint32_t ulIdx = 0, ulValue = 0, ulPos = 0, ulCount = 0, ulFICount=0, ulBase = 0;
    float floatVal=0;
    uint8_t isfloat = 0, ulNeg = 0;
    int8_t *pcStr = NULL, pcBuf[16] = {0}, cFill = 0;
    char HexFormat = 0;
    va_list vaArgP;

    va_start(vaArgP, format);

    while (*format)
    {
        /* Find the first non-% character, or the end of the string. */
        for (ulIdx = 0; (format[ulIdx] != '%') && (format[ulIdx] != '\0'); ulIdx++)
        {}

        /* Write this portion of the string. */
        if (ulIdx > 0)
        {
            printfsend((uint8_t *)format, ulIdx);
        }

        format += ulIdx;

        if (*format == '%')
        {
            format++;

            /* Set the digit count to zero, and the fill character to space */
            /* (i.e. to the defaults) */
            ulCount = 0;
            cFill = ' ';
            isfloat = 0;
            ulFICount = 0;

again:
            switch (*format++)
            {
            case '.': isfloat = 1; ulFICount = ulCount; ulCount =0; goto again;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                if ((format[-1] == '0') && (ulCount == 0) && (isfloat == 0))
                {
                    cFill = '0';
                }

                ulCount *= 10;
                ulCount += format[-1] - '0';

                goto again;
            }

            case 'c':
            {
                ulValue = va_arg(vaArgP, unsigned long);
                printfsend((uint8_t *)&ulValue, 1);
                break;
            }

            case 'd':
            {
                ulValue = va_arg(vaArgP, unsigned long);
                ulPos = 0;

                if ((long)ulValue < 0)
                {
                    ulValue = -(long)ulValue;
                    ulNeg = 1;
                }
                else
                {
                    ulNeg = 0;
                }

                ulBase = 10;
                goto convert;
            }

            case 's':
            {
                pcStr = (int8_t *)va_arg(vaArgP, char *);

                for (ulIdx = 0; pcStr[ulIdx] != '\0'; ulIdx++)
                {}

                printfsend((uint8_t *)pcStr, ulIdx);

                if (ulCount > ulIdx)
                {
                    ulCount -= ulIdx;
                    while (ulCount--)
                    {
                        printfsend((uint8_t *)" ", 1);
                    }
                }
                break;
            }

            case 'u':
            {
                ulValue = va_arg(vaArgP, unsigned long);
                ulPos = 0;
                ulBase = 10;
                ulNeg = 0;
                goto convert;
            }

            case 'X':
            {
                ulValue = va_arg(vaArgP, unsigned long);
                ulPos = 0;
                ulBase = 16;
                ulNeg = 0;
                HexFormat = 'X';
                goto convert;
            }

            case 'x':

            case 'p':
            {
                ulValue = va_arg(vaArgP, unsigned long);
                ulPos = 0;
                ulBase = 16;
                ulNeg = 0;
                HexFormat = 'x';

convert:
                for (ulIdx = 1;
                        (((ulIdx * ulBase) <= ulValue) &&
                         (((ulIdx * ulBase) / ulBase) == ulIdx));
                        ulIdx *= ulBase, ulCount--)
                {
                }

                if (ulNeg)
                {
                    ulCount--;
                }

                if (ulNeg && (cFill == '0'))
                {
                    pcBuf[ulPos++] = '-';
                    ulNeg = 0;
                }

                if ((ulCount > 1) && (ulCount < 16))
                {
                    for (ulCount--; ulCount; ulCount--)
                    {
                        pcBuf[ulPos++] = cFill;
                    }
                }

                if (ulNeg)
                {
                    pcBuf[ulPos++] = '-';
                }

                for (; ulIdx; ulIdx /= ulBase)
                {
                    if (HexFormat == 'x')
                        pcBuf[ulPos++] = g_pcHex1[(ulValue / ulIdx) % ulBase];//x
                    else
                        pcBuf[ulPos++] = g_pcHex2[(ulValue / ulIdx) % ulBase];//X
                    if(ulPos>=16)
                    {
                    	printfsend((uint8_t *)pcBuf, ulPos);
                    	ulPos = 0;
                    }
                }

                printfsend((uint8_t *)pcBuf, ulPos);
                if(isfloat)
                	goto printfloatfrac;
                break;
            }
            case 'f':
            	HexFormat = 'x';
            	goto printfloatinte;
            case 'F':
            	HexFormat = 'X';
printfloatinte:
            	floatVal = va_arg(vaArgP, double);
				ulPos = ulCount;
				ulCount = ulFICount;
				ulFICount = ulPos;
				ulPos = 0;
				ulBase = 10;
				isfloat = 1;
				if ((long)floatVal < 0)
				{
					ulValue = -(long)floatVal;
					ulNeg = 1;
					floatVal = -floatVal;
				}
				else
				{
					ulNeg = 0;
					ulValue = (long)floatVal;
				}
				goto convert;
printfloatfrac:
				isfloat = 0;
				ulPos = 0;
				ulFICount = ulFICount==0 ? 2 : ulFICount;
				pcBuf[ulPos++] = '.';
				for (ulIdx = 0; ulIdx < ulFICount; ulIdx++)
				{
					floatVal -= ulValue;
					floatVal *= 10;
					ulValue = (long)floatVal;
					if(HexFormat == 'x')
						pcBuf[ulPos++] = g_pcHex1[ulValue % ulBase];
					else
						pcBuf[ulPos++] = g_pcHex2[ulValue % ulBase];
					if(ulPos>=16)
					{
						printfsend((uint8_t *)pcBuf, ulPos);
						ulPos = 0;
					}
				}
				printfsend((uint8_t *)pcBuf, ulPos);
				break;

            case '%':
            {
                printfsend((uint8_t *)format - 1, 1);
                break;
            }

            default:
            {
                printfsend((uint8_t *)"ERROR", 5);
                break;
            }
            }/* switch */
        }/* if */
    }/* while */
    va_end(vaArgP);
}

void printv(uint8_t *buf, uint32_t len, char *s)
{
    uint32_t i = 0;
    uint32_t n = 0;
    MyPrintf("%s:", s);
    for (i = 0; i < len; i++)
    {
        if (i % 16 == 0)
        {
            MyPrintf("\r\n%08x:", n);
            n += 16;
        }
        MyPrintf("%02x ", buf[i]);
    }
    MyPrintf("\r\n");
}

void printv1(uint8_t *buf, uint32_t len, char *s)
{
    uint32_t i = 0;
    uint32_t n = 0;
    MyPrintf("%s:", s);
    for (i = 0; i < len; i++)
    {
        MyPrintf("%02x ", buf[i]);
    }
    MyPrintf("\r\n");
}