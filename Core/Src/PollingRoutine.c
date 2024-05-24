/*
 * PollingRoutine.c
 *
 *  Created on: Oct 7, 2023
 *      Author: karl.yamashita
 */

#include "main.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2; // VCP
extern UART_HandleTypeDef huart3;

extern TimerCallbackStruct timerCallback;


UART_DMA_QueueStruct uart1 =
{
	.huart = &huart1,
	.rx.queueSize = UART_DMA_QUEUE_SIZE,
	.tx.queueSize = UART_DMA_QUEUE_SIZE,
};

UART_DMA_QueueStruct uart2 =
{
	.huart = &huart2,
	.rx.queueSize = UART_DMA_QUEUE_SIZE,
	.tx.queueSize = UART_DMA_QUEUE_SIZE
};

UART_DMA_QueueStruct uart3 =
{
	.huart = &huart3,
	.rx.queueSize = UART_DMA_QUEUE_SIZE,
	.tx.queueSize = UART_DMA_QUEUE_SIZE
};


void PollingInit(void)
{
	TimerCallbackRegisterOnly(&timerCallback, BlinkGreenLED);
	TimerCallbackTimerStart(&timerCallback, BlinkGreenLED, 500, TIMER_REPEAT);

	uart1.rx.msgToParse->size = 0;


	UART_DMA_EnableRxInterrupt(&uart1);
	UART_DMA_EnableRxInterrupt(&uart2);
	UART_DMA_EnableRxInterrupt(&uart3);

	UART_DMA_NotifyUser(&uart2, "STM32 ready", strlen("STM32 ready"), true);
}

void PollingRoutine(void)
{
	TimerCallbackCheck(&timerCallback);

	UART_DMA_CheckRxInterruptErrorFlag(&uart1);
	UART_DMA_CheckRxInterruptErrorFlag(&uart2);
	UART_DMA_CheckRxInterruptErrorFlag(&uart3);
	UART_Parse_1(&uart1);
	UART_Parse_2(&uart2);
	UART_Parse_3(&uart3);
}

void UART_Parse_1(UART_DMA_QueueStruct * msg)
{
	char str[] = "UART1_RX Received from UART3_TX > PARSE > Out to UART2_TX > Docklight";

	if(UART_DMA_MsgRdy(msg))
	{
		UART_DMA_NotifyUser(&uart2, str, strlen(str), true);
		UART_DMA_NotifyUser(&uart2, (char*)msg->rx.msgToParse->data, msg->rx.msgToParse->size, false);
	}
}

void UART_Parse_2(UART_DMA_QueueStruct * msg) // VCP
{
	char str[] = "UART2_RX Received from Docklight > PARSE > Out to UART1_TX >  Wired to UART3_RX";

	if(UART_DMA_MsgRdy(msg))
	{
		UART_DMA_NotifyUser(&uart2, str, strlen(str), true);
		UART_DMA_NotifyUser(&uart1, (char*)msg->rx.msgToParse->data, msg->rx.msgToParse->size, false);
	}
}

void UART_Parse_3(UART_DMA_QueueStruct * msg)
{
	char str[] = "UART3_RX Received from UART1_TX > PARSE > Out UART3_TX > Wired to UART1_RX";

	if(UART_DMA_MsgRdy(msg))
	{
		UART_DMA_NotifyUser(&uart2, str, strlen(str), true);
		UART_DMA_NotifyUser(&uart3, (char*)msg->rx.msgToParse->data, msg->rx.msgToParse->size, false);
	}
}

/*
 * Description: Increment pointer and enable interrupt again.
 *
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(huart == uart1.huart)
	{
		uart1.rx.queue[uart1.rx.ptr.index_IN].size = Size;
		RingBuff_Ptr_Input(&uart1.rx.ptr, UART_DMA_QUEUE_SIZE);
		UART_DMA_EnableRxInterrupt(&uart1);
	}
	else if(huart == uart2.huart)
	{
		uart2.rx.queue[uart2.rx.ptr.index_IN].size = Size;
		RingBuff_Ptr_Input(&uart2.rx.ptr, UART_DMA_QUEUE_SIZE);
		UART_DMA_EnableRxInterrupt(&uart2);
	}
	else if(huart == uart3.huart)
	{
		uart3.rx.queue[uart3.rx.ptr.index_IN].size = Size;
		RingBuff_Ptr_Input(&uart3.rx.ptr, UART_DMA_QUEUE_SIZE);
		UART_DMA_EnableRxInterrupt(&uart3);
	}
}

/*
 * Description: The HAL driver calls this callback when it finishes transmitting.
 * 				We clear the txPending flag and call UART_DMA_SendMessage again.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == uart1.huart)
	{
		uart1.tx.txPending = false;
		UART_DMA_SendMessage(&uart1);
	}
	else if(huart == uart2.huart)
	{
		uart2.tx.txPending = false;
		UART_DMA_SendMessage(&uart2);
	}
	else if(huart == uart3.huart)
	{
		uart3.tx.txPending = false;
		UART_DMA_SendMessage(&uart3);
	}
}

void BlinkGreenLED(void)
{
	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}
