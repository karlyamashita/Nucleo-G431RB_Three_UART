/*
 * PollingRoutine.h
 *
 *  Created on: Oct 7, 2023
 *      Author: karl.yamashita
 */

#ifndef INC_POLLINGROUTINE_H_
#define INC_POLLINGROUTINE_H_


void PollingInit(void);
void PollingRoutine(void);

void UART_Parse_1(UART_DMA_QueueStruct * msg);
void UART_Parse_2(UART_DMA_QueueStruct * msg);
void UART_Parse_3(UART_DMA_QueueStruct * msg);

void BlinkGreenLED(void);


#endif /* INC_POLLINGROUTINE_H_ */
