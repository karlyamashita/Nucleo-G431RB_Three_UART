# Nucleo-G431RB_Three_UART

## This project will show how to create a UART data structure with a ring buffer. It also shows how to check HAL status and to recover if HAL does return busy. This project takes advantage of ST's HAL_UARTEx_ReceiveToIdle_DMA to receive a variable length packet.

In UART_DMA_Handler.h is the UART data struture(s).

``` C
#define UART_DMA_CHAR_SIZE 192
#define UART_DMA_QUEUE_SIZE 16

typedef struct
{
	uint8_t data[UART_DMA_CHAR_SIZE];
	uint32_t size;
}UART_DMA_Data; // this is used in queue structure

typedef struct
{
	UART_HandleTypeDef *huart;
	struct
	{
		UART_DMA_Data queue[UART_DMA_QUEUE_SIZE];
		UART_DMA_Data *msgToParse;
		RING_BUFF_STRUCT ptr;
		uint32_t queueSize;
		HAL_StatusTypeDef HAL_Status;
	}rx;
	struct
	{
		UART_DMA_Data queue[UART_DMA_QUEUE_SIZE];
		RING_BUFF_STRUCT ptr;
		uint32_t queueSize;
	}tx;
}UART_DMA_QueueStruct;
```

--*huart-- is the pointer to the uart instance.
In --rx-- structure
+ --queue-- is an array of the structure --UART_DMA_Data.--
+ --*msgToParse-- is a pointer to the --queue-- which is assigned when there is a new message when you call --UART_DMA_MsgRdy().--
+ --ptr-- is the ring buffer pointer to --queue--
+ --queueSize-- is optional but is asssigned the queue size so you don't have to remember the #define name of --UART_DMA_QUEUE_SIZE--, or what ever define name you may use.
+ --HAL_Status-- is the status when --HAL_UARTEx_ReceiveToIdle_DMA-- is called. This is check in main while loop. If not HAL_OK, will attempt to call --HAL_UARTEx_ReceiveToIdle_DMA-- again.

In --tx-- structure
+ pretty much the same as rx structure with same variable names. 


This is an example to declare a data structure instance 
```
UART_DMA_QueueStruct uart2 =
{
	.huart = &huart2,
	.rx.queueSize = UART_DMA_QUEUE_SIZE,
	.tx.queueSize = UART_DMA_QUEUE_SIZE
};
```

document still being written....
