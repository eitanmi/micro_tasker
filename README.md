# Micro Tasker

This code brings non-preemptive scheduling capability to any Win32 C file.
By “non preemptive” we mean that it is up to the executing task to release the CPU to other pending tasks,
the scheduler does not and cannot interrupt a task in the middle of its execution. 
Moreover, this scheduler is not using any prioritizing method but rather simply jumps from one task to another
in the order of their creation.
Even though it was designed with simplicity in mind, it does offer tasks stack separation, and 
several standard methods of controlling code execution, and ultimately brings us closer to
making the most of the MCU and managing large code segments in far
smaller and easier to maintain modules.

## API

```c
#include "scheduler.h"

int main(int argc, char *argv[])
{
    HAL_Enable_Colors();
    SetConsoleTitle("Scheduler");

    HAL_InitTicks();

    /* Create few tasks */
    htsk_moshe = xTaskCreate("TSK_MOSHE", tsk_moshe, 0x3000, NULL);
    htsk_aviv  = xTaskCreate("TSK_AVIV", tsk_aviv, 0x3000, NULL);
    htsk_eli   = xTaskCreate("TSK_ELI", tsk_eli, 0x3000, NULL);

    /* Start the scheduler infinite loop */
    vTaskStartScheduler();

    return 0;
}
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License
[MIT](https://choosealicense.com/licenses/mit/)
