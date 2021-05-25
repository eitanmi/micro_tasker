/**
  ******************************************************************************
  * @file    scheduler.c
  * @author  SolarEdge Infrastructure Team
  * @brief   Scheduler HAL module driver.
  *
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

#include "scheduler.h"
#include "hal.h"
#include "llist.h"

/* Memory protection value */
#define HAL_XTASK_MEM_MARKER 0xcca55acc

/**
  * @brief Context descriptor associated with each running task.
  * @note  This context was carefully aligned, all pointers are
  *        located in an ALIGNED address.
  */

typedef struct __XTask_CtxTypeDef
{
    TaskFunction_t             cb;                              /* Task entry point (call back) */
    void *                     args;                            /* Task arguments */
    char *                     sp_bottom;                       /* Base stack pointer */
    char *                     sp_top;                          /* Base stack pointer */
    uint32_t                   events;                          /* Events */
    uint32_t                   stak_size;                       /* Max stack allocated for the task in bytes */
    jmp_buf                    ctx_task;                        /* Long jump context */
    jmp_buf                    ctx_sched;                       /* Long jump context */
    uint32_t                   delay_end;                       /* Delay end time in ticks */
    uint32_t                   event_expire_end;                /* Event pending expiration tick value */
    uint32_t                   ticks_accumulated;               /* Total ticks spent by the task */
    uint32_t                   ticks_peek;                      /* Peek ticks spent by the task */
    uint32_t                   ticks_avg;                       /* Average ticks spent by the task */
    uint32_t                   ticks_start;                     /* Task start tick value */
    uint8_t                    name[HAL_XTASK_MAX_STRING_SIZE]; /* Task name */
    uint8_t                    yielding;                        /* Yielding state */
    uint8_t                    running;                         /* Are we running? */
    uint8_t                    pendingEvent;                    /* Waiting for event */
    uint8_t                    stk_color;                       /* The initial state stack memory 'color' */
    uint32_t                   mem_marker;                      /* Memory protection  marker */
    struct __XTask_CtxTypeDef *next;                            /* Link next pointer */

} XTask_CtxTypeDef;

/**
  * @brief Module locals, note that all pointers are aligned.
  */

typedef struct __XTask_ConfigTypeDef
{
    XTask_CtxTypeDef *cur;     /* Pointer to the current context being executed */
    XTask_CtxTypeDef *head;    /* Pointer to the context list head */
    uint8_t           running; /* Scheduler global running state ? */

} XTask_ConfigTypeDef;

/* Container for this module globals */
XTask_ConfigTypeDef gXTsk = {.head = NULL, .running = false, .cur = NULL};

/**
  * @brief
  *   Macros for jumping to and from the scheduler
  *   using setjmp / longjmp.
  */

#define vTaskJump(tsk)                  \
    {                                   \
        if ( ! setjmp(tsk->ctx_task) )  \
            longjmp(tsk->ctx_sched, 1); \
    }

#define vSchedJump(tsk)                 \
    {                                   \
        if ( ! setjmp(tsk->ctx_sched) ) \
            longjmp(tsk->ctx_task, 1);  \
    }

/**
  * @brie Gets the current task context.
  * @retval task context pointer if found, else NULL.
  */

/**
  * @brie Gets the current task context.
  * @retval task context pointer if found, else NULL.
  */

static XTask_CtxTypeDef *xTaskGetContext(void)
{
#if ( HAL_XTASK_ENABLED > 0 )

    /* Read current stack pointer address */
    uint32_t sp = (uint32_t) HAL_GetStackPointer();

    /* No active context ? */
    if ( gXTsk.cur == NULL )
        return NULL;

    /* We can return the local task handle only if current SP is within the global tasks memory address space */
    if ( HAL_VAL_IN_RANGE(sp, (uint32_t) gXTsk.cur->sp_bottom, (uint32_t) gXTsk.cur->sp_top) )
    {
        if ( gXTsk.cur->mem_marker == HAL_XTASK_MEM_MARKER )
            return gXTsk.cur; /* Return current task as a handle */
    }

    /* If we're here, we couldn't map current stack to any of the running tasks */
    return NULL;

#else
    return NULL;
#endif
}

/**
  * @brief Validate task stack prior to entering
  * @param handle: handle (pointer) to a task structure.
  * @retval Boolean.
  */

#if ( HAL_XTASK_STACK_CHECK_LEN > 0 )

static bool xTaskValidate(XTask_CtxTypeDef *ctx)
{
    char *ptr = NULL;
    int   i   = 0;

    if ( ctx && ctx->mem_marker == HAL_XTASK_MEM_MARKER )
    {
        /* Stack overflow protection , make sure that the last x byte of the stack base address
         * has the correct ' color'  and that the jump stack pointer is located at the correct boundaries
         */

        ptr = (char *) ctx->sp_bottom;
        while ( i++ < HAL_XTASK_STACK_CHECK_LEN )
        {

            if ( *ptr != ctx->stk_color )
                return false;

            ptr++;
        }
    }

    return true;
}

#endif

/**
  * @brief Return the stack usage in percentages.
  * @param handle: handle (pointer) to a task structure.
  * @retval usage in percentages or -1 on error;
  */

int xTaskGetStackUsage(TaskHandle_t handle)
{

    XTask_CtxTypeDef *ctx = (XTask_CtxTypeDef *) handle;

    int          usage   = -1;
    char *       ptr     = NULL;
    unsigned int freeMem = 0;

    if ( ctx && handle != HAL_XTASK_INVALID_HANDLE && ctx->mem_marker == HAL_XTASK_MEM_MARKER )
    {

        ptr = (char *) ctx->sp_bottom;
        while ( freeMem < ctx->stak_size )
        {
            if ( *ptr != ctx->stk_color )
                break;

            freeMem++;
            ptr++;
        }

        /* At this point 'freeMem' holds the count of bytes with the initial stack ' color' value */
        usage = 100 - (freeMem * 100 / ctx->stak_size);
    }

    return usage;
}

/**
 * @brief Dumps XTasks statistics
 * @param print: 'printf' implementation
 * @retval None.
 */

void xTaskDumpStats(PrintfFn print)
{
#if ( HAL_XTASK_ENABLED > 0 )

#if ( HAL_XTASK_COLLECT_STATS > 0 )

    int             tskCnt       = 0;
    char            tskState[16] = {0};
    char            tskUsage[16] = {0};
    char            timeBuf[32]  = {0};
    HAL_TimeTypeDef timestamp;
    int             ctxSize = sizeof(XTask_CtxTypeDef);

    XTask_CtxTypeDef *ctx = NULL;

    print("\r\n");
    print("%-10s%-14s%-16s%-12s%-20s%-12s", "Name", "State", "Stack total", "Stack peek", "Time spent (H:m:s)", "Time peek (ms)");
    print("\r\n--------------------------------------------------------------------------------------\r\n\r\n");

    LL_FOREACH(gXTsk.head, ctx)
    {
        tskState[0] = 0;
        tskUsage[0] = 0;
        timeBuf[0]  = 0;

        if ( ctx->running == false )
            strncpy(tskState, "Stopped", sizeof(tskState) - 1);
        else
        {
            if ( ctx->pendingEvent == true )
                strncpy(tskState, "Pending", sizeof(tskState) - 1);
            else if ( ctx->delay_end > 0 )
                strncpy(tskState, "Delaying", sizeof(tskState) - 1);
            else
                strncpy(tskState, "Executing", sizeof(tskState) - 1);
        }

        /* Build a time stamp string */
        HAL_TicksToTime(&timestamp, (uint32_t) ctx->ticks_accumulated);
        snprintf(timeBuf, sizeof(timeBuf), "%02d.%02d:%02d", timestamp.hours, timestamp.minutes, timestamp.seconds);

        snprintf(tskUsage, sizeof(tskUsage) - 1, "%d%%", xTaskGetStackUsage((TaskHandle_t) ctx));
        print("%-10s%-14s%-16d%-12s%-20s%-12lu\r\n", ctx->name, tskState, ctx->stak_size, tskUsage, timeBuf, ctx->ticks_peek);
        tskCnt++;
    }

    print("\r\nTotal running tasks: %d, context size: %d bytes.\r\n", tskCnt, ctxSize);
    print("'Time spent' can only accumulate full milliseconds rather than fraction of a millisecond.\r\n", ctxSize);

#else
    print("\r\nThe scheduler is set NOT to collect statistics.\r\n");
#endif

#else
    print("\r\nNot available when a scheduler is not used.\r\n");
#endif
}

/**
  * @brief Signals a task.
  * @param handle: handle (pointer) to a task structure.
  * @param event: event to signal.
  * @retval none.
  */

void xTaskNotify(TaskHandle_t handle, uint32_t event)
{

#if ( HAL_XTASK_ENABLED > 0 )

    XTask_CtxTypeDef *ctx = (XTask_CtxTypeDef *) handle;

    if ( ctx && handle != HAL_XTASK_INVALID_HANDLE && ctx->mem_marker == HAL_XTASK_MEM_MARKER )
    {
        HAL_SET_BIT(ctx->events, event);
    }

#endif
}

/**
  * @brief  Checks if there are any pending events, otherwise jump back to the scheduler.
  *         The events will be auto cleared.
  * @retval Event bits.
  */

uint32_t xTaskNotifyWait(uint32_t ticksToWait)
{
    uint32_t stored = 0;

#if ( HAL_XTASK_ENABLED > 0 )

    XTask_CtxTypeDef *ctx = xTaskGetContext(); /* Find current context */

    /* Make sure the context is valid and jump */
    if ( ctx && ctx->running == true )
    {
        if ( ctx->events == 0 )
        {
            ctx->pendingEvent = true;

            /* Set event pending expiration tick */
            if ( ticksToWait > 0 && ticksToWait != HAL_XTASK_MAX_TIME )
                ctx->event_expire_end = (HAL_GetTick() + ticksToWait);

            vTaskJump(ctx);
        }

        /* We're back from the context execution, we can return the pending event bits to the caller */
        stored            = ctx->events;
        ctx->pendingEvent = false;
        ctx->events       = 0;
    }

#endif
    return stored;
}

/**
  * @brief Jumps back to the scheduler and let other task do some work.
  * @retval None.
  */

void taskYIELD(void)
{
#if ( HAL_XTASK_ENABLED > 0 )

    XTask_CtxTypeDef *ctx = xTaskGetContext(); /* Find current context */

    /* Make sure the context is valid and jump */
    if ( ctx && ctx->running == true )
    {
        ctx->yielding = true; /* Mark this context as yielding */
        vTaskJump(ctx);
    }

#endif
}

/**
  * @brief Mark the task as delayed for the required duration and jump back to the schedule.
  * @param handle: handle (pointer) to a task structure.
  * @retval None.
  */

void vTaskDelay(uint32_t delay)
{
#if ( HAL_XTASK_ENABLED > 0 )

    XTask_CtxTypeDef *ctx = xTaskGetContext(); /* Find current context */

    /* Make sure the context is valid and jump */
    if ( ctx && ctx->running == true )
    {
        ctx->yielding = true;

        /* Set delay expiration tick */
        if ( delay > 0 )
            ctx->delay_end = (HAL_GetTick() + delay);

        vTaskJump(ctx);
    }

#endif
}

/**
  * @brief Create a new task in memory in suspended state.
  * @param name: NULL terminated string describing the task.
  * @param cb: Task handler function`.
  * @param stackSize: Stack to allocate for the stack.
  * @retval valid handle to the newly created task.
  */

TaskHandle_t xTaskCreate(char *name, TaskFunction_t cb, uint32_t stackSize, void *ptr)
{
#if ( HAL_XTASK_ENABLED > 0 )

    XTask_CtxTypeDef *ctx       = NULL;
    void *            stk_ptr   = NULL;
    static char       stk_color = 'A';

    /* Not adding new tasks after the scheduler has been started */
    if ( gXTsk.running == true )
        return HAL_XTASK_INVALID_HANDLE;

    ctx = malloc(sizeof(XTask_CtxTypeDef));
    if ( ! ctx )
        return HAL_XTASK_INVALID_HANDLE; /* No memory for the task node */

    /* Clears task list member */
    memset(ctx, 0, sizeof(XTask_CtxTypeDef));

    /* Initializes all of the task properties */
    strncpy((char *) ctx->name, name, HAL_XTASK_MAX_STRING_SIZE);
    ctx->mem_marker       = HAL_XTASK_MEM_MARKER;
    ctx->cb               = cb;
    ctx->args             = ptr;
    ctx->events           = 0;
    ctx->event_expire_end = HAL_XTASK_MAX_TIME;
    ctx->sp_bottom        = (char *) malloc(stackSize + 1024);
    ctx->sp_top           = ctx->sp_bottom + stackSize;
    ctx->stak_size        = stackSize;
    ctx->stk_color        = stk_color++;
    ctx->next             = NULL;

    if ( ctx->sp_top == NULL )
        return HAL_XTASK_INVALID_HANDLE;

    memset(ctx->sp_bottom, ctx->stk_color, ctx->stak_size);

    /* Attach it to the tasks list */
    LL_APPEND(gXTsk.head, ctx);

    // printf_c(Color_White, "'%s' created, stack bottpm: %p, top : %p", ctx->name, ctx->sp_bottom, ctx->sp_top);

    return (TaskHandle_t) ctx;

#endif
    return HAL_XTASK_INVALID_HANDLE;
}

/**
  * @brie Gets the task handle by iterating the tasks list.
  * @retval task pointer if found, else NULL.
  */

TaskHandle_t xTaskGetHandle(void)
{
#if ( HAL_XTASK_ENABLED > 0 )

    XTask_CtxTypeDef *ctx = xTaskGetContext(); /* Find current context */

    /* If the context is valid */
    if ( ctx )
        return (TaskHandle_t) ctx;

#endif
    return HAL_XTASK_INVALID_HANDLE;
}

/**
  * @brief Enters the task for the first time.
  * @retval Nothing
  */

void vTaskStart(XTask_CtxTypeDef *ctx)
{

    /*
    * This task has not been started yet. Assign a new stack
    * pointer, run the task, and exit it at the end.
	*/
    register void *top = (void *) ctx->sp_top;
    __asm
    {
			mov esp, top;
    }

    ctx->cb(ctx->args);
}

/**
  * @brief Start an endless task scheduler loop.
  * @retval HAL Status type.
  */

bool vTaskStartScheduler(void)
{

#if ( HAL_XTASK_COLLECT_STATS > 0 )
    uint32_t ticks_spent;
#endif

    /* Already running ? */
    if ( gXTsk.running == true )
        return false;

    /* No tasks to execute! */
    if ( gXTsk.head == NULL )
        return false;

    /* Useful, allow some time for the system to stabilize before starting the show */
    HAL_Delay(100);

    /* Start all tasks, the next task will be started as soon as the previous one yields */
    /* This is an intermediate state, where have to invoke the task by calling it startup routine.
     */
    LL_FOREACH(gXTsk.head, gXTsk.cur)
    {
        if ( gXTsk.cur->mem_marker == HAL_XTASK_MEM_MARKER )
        {

            gXTsk.cur->running = true;
            if ( ! setjmp(gXTsk.cur->ctx_sched) )
            {
                vTaskStart(gXTsk.cur);
            }
        }
    }

    /* Sets scheduler state to running */
    gXTsk.running = true;

    /* Infinite loop serving tasks as needed */
    while ( true )
    {

        LL_FOREACH(gXTsk.head, gXTsk.cur)
        {

#if ( HAL_XTASK_STACK_CHECK_LEN > 0 )

            if ( xTaskValidate(gXTsk.cur) == false )
            {
                printf("\r\nStack over flow detected!\r\n");
                exit(0); /* Invalid handle or stack memory, not really a heap is */
            }
#endif

            /* Jump to the next task in the list */
            /* Note: Future implementation can drop this context switch and simply
             * jump from one context to the next without passing through here.
             */

            gXTsk.cur->ticks_start = 0;

            /* Check if event expiration tick was set and reached */
            if ( gXTsk.cur->running == true && (gXTsk.cur->yielding == true || gXTsk.cur->events || (HAL_GetTick() >= gXTsk.cur->event_expire_end)) )
            {

                /* Check if delay interval was set and expired */
                if ( gXTsk.cur->delay_end == 0 || gXTsk.cur->delay_end <= HAL_GetTick() )
                {
                    gXTsk.cur->delay_end   = 0;
                    gXTsk.cur->yielding    = false;
                    gXTsk.cur->ticks_start = HAL_GetTick();

                    vSchedJump(gXTsk.cur);
                }
            }

#if ( HAL_XTASK_COLLECT_STATS > 0 )

            /* Collect statistics */
            if ( gXTsk.cur->ticks_start > 0 )
            {
                ticks_spent           = (HAL_GetTick() - gXTsk.cur->ticks_start);
                gXTsk.cur->ticks_peek = HAL_MAX(gXTsk.cur->ticks_peek, ticks_spent);
                gXTsk.cur->ticks_accumulated += ticks_spent; /* Store total accumulated ticks spent by the task */
            }
#endif
            /* Dump statitics when the user presses any key */
            if ( HAL_getch() > -1 )
            {
                xTaskDumpStats(printf);
                HAL_Pause("\r\nPress space to continue..\r\n", 0x20);
            }
        }
    }
}
