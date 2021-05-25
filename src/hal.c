
/**
 ******************************************************************************
 * @file    wain_wrapper.c
 * @author  SolarEdge Infrastructure Team
 * @brief   HAL module driver.
 *          This is the common part of the HAL package.
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 SolarEdge.
 * All rights reserved.</center></h2>
 *
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "hal.h"
#include "ansi.h"

/* Global system start tick value */
uint32_t gStartTick = 0;

/**
 * @brief
 *   Initializes the ticks counter. 
 * @return
 *   nothing.
 */

void HAL_InitTicks(void)
{
    gStartTick = GetTickCount();
}
/**
 * @brief  Provides a tick value in millisecond.
 * @note   This function is declared as __weak  to be overwritten  in case of
 * other implementations in user file.
 * @retval tick value
 */

uint32_t HAL_GetTick(void)
{

    return GetTickCount() - gStartTick;
}

/**
 * @brief This function provides accurate delay (in milliseconds) based
 *        on TIMER0 counter read.
 * @param Delay: specifies the delay time length, in ticks.
 * @retval None
 */

void HAL_Delay(uint16_t ticks)
{
    Sleep(ticks);
}

/**
 * @brief
 *   Construct a HAL day and time type out milliseconds integer value.
 * @param
 *   HAL date type.
 *   Ticks to covert, if 0 we will use the current system tick.
 * @return none.
 */

void HAL_TicksToTime(HAL_TimeTypeDef *time, uint32_t ms)
{

    if ( ms == 0 )
        ms = HAL_GetTick();

    if ( time )
    {
        time->msecs   = ms % 1000;
        time->seconds = (ms / 1000) % 60;
        time->minutes = (ms / (1000 * 60)) % 60;
        time->hours   = (ms / (1000 * 60 * 60)) % 24;
        time->days    = time->hours / 24;
    }
}

/**
 * @brief
 *   Gets the stack pointer 
 * @return stack pointer.
 */

__declspec(naked) void *HAL_GetStackPointer(void)
{
    __asm
    {
        mov eax, esp
        ret
    }
}

/**
 * @brief
 *   Printf coupled with time
 * @param
 *   printf style input
 * @return count of bytes printed.
 */

int printf_c(HAL_TermColor color, const char *format, ...)
{
    static lines = 0;
    va_list         list;
    char            localBuff[1024] = {0};
    int             size            = -1;
    HAL_TimeTypeDef timestamp;

    do
    {

        if ( ! format ) // Module was not initialized
            break;

        if ( lines == 25 )
        {
            system("cls");
            lines = 0;
        }

        HAL_TicksToTime(&timestamp, 0);

        // Construct the message
        va_start(list, format);
        size = vsnprintf(localBuff, sizeof(localBuff) - 1, format, list);
        va_end(list);

        if ( size < 0 )
            break;

        printf("[%02d.%02d:%02d:%02d.%03d] ", timestamp.days, timestamp.hours, timestamp.minutes, timestamp.seconds, (int) timestamp.msecs);

        switch ( color )
        {
            case Color_White:
                printf(ANSI_MODE);
                break;
            case Color_Red:
                printf(ANSI_RED);
                break;
            case Color_Green:
                printf(ANSI_GREEN);
                break;
            case Color_Blue:
                printf(ANSI_BLUE);
                break;
            case Color_Yellow:
                printf(ANSI_YELLOW);
                break;
        }

        printf("%s\r\n" ANSI_MODE, localBuff);
    } while ( 0 );

    lines++;

    return size;
}

/**
 * @brief
 *   Print a message to the console, pause ans waiy for key.
 * @return
 *   none.
 */

void HAL_Pause(char *str, char expected)
{
    int ch;
    if ( str )
        printf(str);

    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

    do
    {
        ch = _getch();
        ch = toupper(ch);
    } while ( ch != expected );
}

/**
 * @brief
 *   Non blocking getch() that checks for input only only once in x rounds.
 *   Specially made so the scheduler will not be slowed down due to this call.
 * @return
 *   Byte read, or -1 when there is nothing.
 *   none.
 */

int HAL_getch(void)
{
    int             c = -1;
    INPUT_RECORD    rc_input, rc;
    HANDLE          h_stdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD           dwRead, dwEvents = 0;
    static uint32_t cnt = 1;

    if ( cnt % 10000 == 0 )
    {
        cnt = 1;
        if ( PeekConsoleInput(h_stdin, &rc_input, 1, &dwEvents) == TRUE )
        {
            if ( dwEvents > 0 )
            {

                if ( ReadConsoleInput(h_stdin, &rc, 1, &dwRead) == FALSE )
                    return c;

                if ( rc.EventType == KEY_EVENT && rc.Event.KeyEvent.bKeyDown == TRUE )
                {
                    if ( rc.Event.KeyEvent.wRepeatCount > 1 )
                        c = 0;
                    if ( rc.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE )
                        c = 0;
                    else
                        c = rc.Event.KeyEvent.uChar.AsciiChar;
                }
            }
        }
    }
    cnt++;

    return c;
}

/**
 * @brief
 *   Turn colors on in Win10 CMD window.
 * @return
 *   none.
 */

void HAL_Enable_Colors(void)
{

#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif

}

/************************ (C) COPYRIGHT SolarEdge *****END OF FILE****/
