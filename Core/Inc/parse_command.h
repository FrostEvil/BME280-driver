/*
 * parse_command.h
 *
 *  Created on: May 11, 2026
 *      Author: asiac
 */

#ifndef INC_PARSE_COMMAND_H_
#define INC_PARSE_COMMAND_H_

#include <stdint.h>
#include "bme280.h"

typedef enum {
	CMD_SET_OVERSAMPLING, CMD_GET_STATUS, CMD_INVALID
} CommandType;

typedef struct {
	CommandType type;
	uint8_t value;
} Command;


Command ParseCommand( char *rx_data_buffer);
void ExecuteCommand(Command *command, BME280_HandleTypeDef *bme,
		char *oversampling_statement);

#endif /* INC_PARSE_COMMAND_H_ */
