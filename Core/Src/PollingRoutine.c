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
	.tx.queueSize = UART_DMA_QUEUE_SIZE
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

	UART_DMA_EnableRxInterrupt(&uart1);
	UART_DMA_EnableRxInterrupt(&uart2);
	UART_DMA_EnableRxInterrupt(&uart3);

	NotifyUserDMA(&uart2, "STM32 ready", true);

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

	UART_DMA_SendMessage(&uart1);
	UART_DMA_SendMessage(&uart2);
	UART_DMA_SendMessage(&uart3);
}

void UART_Parse_1(UART_DMA_QueueStruct * msg)
{
	if(UART_DMA_MsgRdy(msg))
	{
		NotifyUserDMA(&uart2, "UART1_RX Received from UART3_TX > PARSE > Out to UART2_TX > Docklight", true);
		NotifyUserDMA(&uart2, (char*)msg->rx.msgToParse->data, false);
	}
}

void UART_Parse_2(UART_DMA_QueueStruct * msg) // VCP
{
	if(UART_DMA_MsgRdy(msg))
	{
		NotifyUserDMA(&uart2, "UART2_RX Received from Docklight > PARSE > Out to UART1_TX >  Wired to UART3_RX", true);
		NotifyUserDMA(&uart1, (char*)msg->rx.msgToParse->data, false);
	}
}

void UART_Parse_3(UART_DMA_QueueStruct * msg)
{
	if(UART_DMA_MsgRdy(msg))
	{
		NotifyUserDMA(&uart2, "UART3_RX Received from UART1_TX > PARSE > Out UART3_TX > Wired to UART1_RX", true);
		NotifyUserDMA(&uart3, (char*)msg->rx.msgToParse->data, false);
	}
}

/*
 * Description: Increment pointer
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


void BlinkGreenLED(void)
{
	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}
