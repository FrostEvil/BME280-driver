/*
 * parse_command.c
 *
 *  Created on: May 11, 2026
 *      Author: asiac
 */

#include "parse_command.h"
#include "bme280.h"
#include <string.h>
#include <stdio.h>

static BME280_Oversampling ConvertOversamplingFactor(int value) {
	BME280_Oversampling osrs = BME280_OSRS_SKIPPED;
	switch (value) {
	case 1:
		osrs = BME280_OSRS_X1;
		break;

	case 2:
		osrs = BME280_OSRS_X2;
		break;

	case 4:
		osrs = BME280_OSRS_X4;
		break;

	case 8:
		osrs = BME280_OSRS_X8;
		break;

	case 16:
		osrs = BME280_OSRS_X16;
		break;

	default:
		osrs = BME280_OSRS_SKIPPED;
	}

	return osrs;
}

static int ConvertOversamplingAnswer(int value) {
	int osrs_answer = 0;
	switch (value) {
	case 0:
		osrs_answer = 0;
		break;

	case 1:
		osrs_answer = 1;
		break;

	case 2:
		osrs_answer = 2;
		break;

	case 3:
		osrs_answer = 4;
		break;

	case 4:
		osrs_answer = 8;
		break;

	default:
		osrs_answer = 16;
	}

	return osrs_answer;

}

const char* ConvertModeValue(int value) {

	switch (value) {
	case 0:
		return "Sleep mode";

	case 1:
	case 2:
		return "Forced mode";

	case 3:
		return "Normal mode";

	default:
		return "Unknown mode";
	}
}

const char* ConvertStatusMeasuring(int value) {

	switch (value) {
	case 0:
		return "IDLE";

	case 1:
		return "Measuring";

	default:
		return "Unknown measuring status";

	}

}

const char* ConvertStatusUpdate(int value) {

	switch (value) {
	case 0:

		return "update ready";

	case 1:

		return "update busy";

	default:

		return "unknown update status";

	}

}

/* Parses UART commands and updates BME280 configuration */
Command ParseCommand(char *rx_data_buffer) {

	int value = 0;
	rx_data_buffer[strcspn(rx_data_buffer, "\r\n")] = 0;
	BME280_Oversampling osrs;
	Command parse_command = { .type = CMD_INVALID, .value = 0 };

	if (strcmp(rx_data_buffer, "status") == 0) {
		parse_command.type = CMD_GET_STATUS;
		parse_command.value = 1;
	} else {
		if (sscanf(rx_data_buffer, "osrs %d", &value) == 1) {
			osrs = ConvertOversamplingFactor(value);

			if (osrs != BME280_OSRS_SKIPPED) {
				parse_command.type = CMD_SET_OVERSAMPLING;
				parse_command.value = osrs;
			}
		} else {
			parse_command.type = CMD_INVALID;
			parse_command.value = 0;
		}
	}

	return parse_command;

}

void ExecuteCommand(Command *command, BME280_HandleTypeDef *bme,
		char *uart_response) {


	/* Buffer for UART response messages */
	if (command->type == CMD_GET_STATUS) {
		BME280_ReadConfig(bme);

		snprintf(uart_response, 256, "Temperature oversampling x%d\r\n"
				"Pressure oversampling x%d\r\n"
				"Humidity oversampling x%d\r\n"
				"Mode: %s\r\n"
				"Status_measuring: %s\r\n"
				"Status_update: %s\r\n",
				ConvertOversamplingAnswer(bme->curr_params.osrs_t),
				ConvertOversamplingAnswer(bme->curr_params.osrs_p),
				ConvertOversamplingAnswer(bme->curr_params.osrs_h),
				ConvertModeValue(bme->curr_params.mode),
				ConvertStatusMeasuring(bme->curr_params.measuring_status),
				ConvertStatusUpdate(bme->curr_params.update_status));

	} else if (command->type == CMD_SET_OVERSAMPLING && command->value != 0) {

		HAL_StatusTypeDef BME280_set_oversampling_status =
				BME280_SetOversampling(bme, command->value);

		if (BME280_set_oversampling_status == HAL_OK) {
			strcpy(uart_response, "Oversampling set correctly!\r\n");
		} else {
			strcpy(uart_response, "Something went wrong...\r\n");
		}

	} else {
		strcpy(uart_response, "Wrong command!\r\n");
	}

}
