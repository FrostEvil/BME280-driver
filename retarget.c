/*
 * retarget.c
 *
 *  Created on: Apr 23, 2026
 *      Author: tomas
 */

#include <stdio.h>
#include "main.h"

//Zadeklarowany uchwyt URAT zdefiniowany w usart.c
extern UART_HandleTypeDef huart2;

int __io_putchar(uint8_t ch) {
	//Wysyłanie jednego znaku przez UART
	//Timeout ustawiony na 10ms
	HAL_UART_Transmit(&huart2, &ch, 1, 10);
	return ch;
}
