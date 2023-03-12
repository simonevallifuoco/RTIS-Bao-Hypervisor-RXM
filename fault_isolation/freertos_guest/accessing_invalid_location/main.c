/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* FreeRTOS kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <uart.h>
#include <irq.h>
#include <plat.h>
#include <stdlib.h>
#include <string.h>
#include <timers.h>
#include <FreeRTOSConfig.h>


/* Prototypes for the standard FreeRTOS callback/hook functions */
void vApplicationMallocFailedHook(void);
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
void vApplicationTickHook(void);

/*-----------------------------------------------------------*/

#define SHMEM_IRQ_ID (52)

char* const freertos_message = (char*)0x70000000;
char* const linux_message    = (char*)0x70002000;
const size_t shmem_channel_size = 0x2000; 

static int start = 0;  
static int incr = 0;

void uart_rx_handler(){
    static int irq_count = 0;
    printf("%s %d\n", __func__, ++irq_count);
    uart_clear_rxirq();
}


void shmem_update_msg(char* str) {
    sprintf(freertos_message, "%s", str);
}

void shmem_handler() {
    linux_message[shmem_channel_size-1] = '\0';
    char* end = strchr(linux_message, '\n');
    *end = '\0';
    printf("%s\n", linux_message);
    sprintf(freertos_message, "FreeRTOS: OK, number %d\n", incr++);
    if(incr==5){
        printf("Trying to access a not valid location\n");
        volatile int *p = (volatile int *)0x00000000; 
        *p = 0xDEADBEEF;
    }
    start = 1;
}

void shmem_init() {
    memset(freertos_message, 0, shmem_channel_size);
    memset(linux_message, 0, shmem_channel_size);
    shmem_update_msg(0);
    irq_set_handler(SHMEM_IRQ_ID, shmem_handler);
    irq_set_prio(SHMEM_IRQ_ID, IRQ_MAX_PRIO);
    irq_enable(SHMEM_IRQ_ID);
}

//Matrici
#define MSIZE 100
#define N1 100
#define N2 100

void generate_matrix(uint32_t **matrix, const size_t size) {
    for(size_t i = 0; i < size; i++) {
        for(size_t j = 0; j < size; j++) {
            matrix[i][j] = rand()%100;
        }
    }
}

static void prod_matrix(uint32_t **m1, uint32_t **m2, const size_t size) 
{
    static uint32_t **result = NULL;
    
    if(result == NULL) {
        result = (uint32_t**) pvPortMalloc(size * sizeof(uint32_t*));
        
        for(int i = 0; i < size; i++) {
            result[i] = (uint32_t*) pvPortMalloc(size * sizeof(uint32_t));
        }   
    }

    for(size_t i = 0; i < size; i++) {
        for(size_t j = 0; j < size; j++) {
            result[i][j] = 0;
        }
    }
    
    for (int i = 0; i < size; i++) {
    //printf("%d", i);
        for (int j = 0; j < size; j++) {
            for (int k = 0; k < size; k++) {
                result[i][j] += m1[i][k] * m2[k][j];
            }
        }
    }
    
}

/*-----------------------------------------------------------*/

void vTask(void *pvParameters)
{
    printf("Start vTask\n");
    unsigned long counter = 0;
    unsigned long id = (unsigned long)pvParameters;

    //Allocazione delle matrici
    uint32_t **m1, **m2;
    m1 = (uint32_t**) pvPortMalloc(MSIZE * sizeof(uint32_t*));  
    m2 = (uint32_t**) pvPortMalloc(MSIZE * sizeof(uint32_t*));

    for(int i = 0; i < MSIZE; i++) 
    {
        m1[i] = (uint32_t*) pvPortMalloc(MSIZE * sizeof(uint32_t));
        m2[i] = (uint32_t*) pvPortMalloc(MSIZE * sizeof(uint32_t));
    }
    
    generate_matrix(m1, MSIZE);
    generate_matrix(m2, MSIZE);

    while (1)
    {
        printf("Task%d: %d waiting for start message\n", id, counter++);

        while(start == 0) vTaskDelay(10); 
        start = 0;

        printf("Task%d: %d start=1\n", id, counter++);

        unsigned long start_time = 0;
        unsigned long end_time = 0;
        unsigned long sum = 0;
        unsigned long mean = 0;


        for(int p = 0; p < N1; p++) 
        {
            sum = 0;
            mean = 0;

            for(int q = 0; q < N2; q++) 
            {

                start_time = xTaskGetTickCount();
                //printf("start_time: %d\n", start_time);

                //Prodotto tra matrici
                prod_matrix(m1, m2, MSIZE);

                end_time = xTaskGetTickCount();
                //printf("end_time: %d\n", end_time);
                sum += end_time - start_time;
                //printf("sum: %d\n", sum);
            }

            //media dei tempi calcolata in microsecondi
            mean = (sum*1000000)/ (N2*configTICK_RATE_HZ);
            printf("Average calculation time: %d us\n", mean);
        }

    }
}


/*-----------------------------------------------------------*/

int main(void)
{

    printf("Bao FreeRTOS guest\n");

    uart_enable_rxirq();
    irq_set_handler(UART_IRQ_ID, uart_rx_handler);
    irq_set_prio(UART_IRQ_ID, IRQ_MAX_PRIO);
    irq_enable(UART_IRQ_ID); 
     
    //seme numeri casuali
    srand(xTaskGetTickCount());

    printf("Shared memory initialization\n");
    shmem_init();   

    printf("Create task\n");
    xTaskCreate(
        vTask,
        "Task1",
        configMINIMAL_STACK_SIZE,
        (void *)1,
        tskIDLE_PRIORITY + 1,
        NULL);

    /*
    xTaskCreate(
        vTask,
        "Task2",
        configMINIMAL_STACK_SIZE,
        (void *)2,
        tskIDLE_PRIORITY + 1,
        NULL);
    */

    vTaskStartScheduler();

}   

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{
    /* vApplicationMallocFailedHook() will only be called if
    configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
    function that will get called if a call to pvPortMalloc() fails.
    pvPortMalloc() is called internally by the kernel whenever a task, queue,
    timer or semaphore is created.  It is also called by various parts of the
    demo application.  If heap_1.c or heap_2.c are used, then the size of the
    heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
    FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
    to query the size of free heap space that remains (although it does not
    provide information on how the remaining heap might be fragmented). */
    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void)
{
    /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
    to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
    task.  It is essential that code added to this hook function never attempts
    to block in any way (for example, call xQueueReceive() with a block time
    specified, or call vTaskDelay()).  If the application makes use of the
    vTaskDelete() API function (as this demo application does) then it is also
    important that vApplicationIdleHook() is permitted to return to its calling
    function, because it is the responsibility of the idle task to clean up
    memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pcTaskName;
    (void)pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}
/*-----------------------------------------------------------*/

void vApplicationTickHook(void)
{
}
/*-----------------------------------------------------------*/

void vAssertCalled(void)
{
    volatile uint32_t ulSetTo1ToExitFunction = 0;

    taskDISABLE_INTERRUPTS();
    while (ulSetTo1ToExitFunction != 1)
    {
        __asm volatile("NOP");
    }
}
/*-----------------------------------------------------------*/

/* This version of vApplicationAssert() is declared as a weak symbol to allow it
to be overridden by a version implemented within the application that is using
this BSP. */
void vApplicationAssert( const char *pcFileName, uint32_t ulLine )
{
volatile uint32_t ul = 0;
volatile const char *pcLocalFileName = pcFileName; /* To prevent pcFileName being optimized away. */
volatile uint32_t ulLocalLine = ulLine; /* To prevent ulLine being optimized away. */

    /* Prevent compile warnings about the following two variables being set but
    not referenced.  They are intended for viewing in the debugger. */
    ( void ) pcLocalFileName;
    ( void ) ulLocalLine;

    printf( "Assert failed in file %s, line %lu\r\n", pcLocalFileName, ulLocalLine );

    /* If this function is entered then a call to configASSERT() failed in the
    FreeRTOS code because of a fatal error.  The pcFileName and ulLine
    parameters hold the file name and line number in that file of the assert
    that failed.  Additionally, if using the debugger, the function call stack
    can be viewed to find which line failed its configASSERT() test.  Finally,
    the debugger can be used to set ul to a non-zero value, then step out of
    this function to find where the assert function was entered. */
    taskENTER_CRITICAL();
    {
        while( ul == 0 )
        {
            __asm volatile( "NOP" );
        }
    }
    taskEXIT_CRITICAL();
}
