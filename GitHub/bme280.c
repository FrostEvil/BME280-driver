/*
 * bme280.c
 *
 *  Created on: Apr 28, 2026
 *      Author: tomas
 */

#include "bme280.h"

typedef uint32_t BME280_U32_t;
typedef int64_t BME280_S64_t;

static HAL_StatusTypeDef BME280_ReadReg(BME280_HandleTypeDef *bme, uint8_t reg,
		uint8_t *data) {
	HAL_StatusTypeDef transmit_status;
	HAL_StatusTypeDef receive_status;

	transmit_status = HAL_I2C_Master_Transmit(bme->hi2c, bme->address, &reg, 1,
			100);

	if (transmit_status != HAL_OK) {
		return transmit_status;
	}
	receive_status = HAL_I2C_Master_Receive(bme->hi2c, bme->address, data, 1,
			100);

	return receive_status;
}

static HAL_StatusTypeDef BME280_ReadMulti(BME280_HandleTypeDef *bme,
		uint8_t reg, uint8_t *buffer, uint8_t size) {
	HAL_StatusTypeDef transmit_status;
	HAL_StatusTypeDef receive_status;

	transmit_status = HAL_I2C_Master_Transmit(bme->hi2c, bme->address, &reg, 1,
			100);

	if (transmit_status != HAL_OK) {
		return transmit_status;
	}
	receive_status = HAL_I2C_Master_Receive(bme->hi2c, bme->address, buffer,
			size, 100);

	return receive_status;

}

static HAL_StatusTypeDef BME280_WriteReg(BME280_HandleTypeDef *bme, uint8_t reg,
		uint8_t value) {
	HAL_StatusTypeDef transmit_status;
	uint8_t buffer[2] = { reg, value };

	transmit_status = HAL_I2C_Master_Transmit(bme->hi2c, bme->address, buffer,
			2, 100);

	return transmit_status;
}

// Initializes BME280 sensor:
// - verifies chip ID
// - configures oversampling and mode
// - reads calibration data from sensor
HAL_StatusTypeDef BME280_Init(BME280_HandleTypeDef *bme) {
	// Validate input pointer
	if (bme == NULL) {
		return HAL_ERROR;
	}

	uint8_t BME280_id = 0; // Stores chip ID read from register 0xD0
	HAL_StatusTypeDef BME280_id_status = BME280_ReadReg(bme, 0xD0, &BME280_id);

	if (BME280_id_status != HAL_OK) {
		return BME280_id_status;
	}

	if (BME280_id != 0x60) {
		return HAL_ERROR;
	}

// Set mode and oversampling parameters
	uint8_t ctrl_hum = bme->osrs_h & 0x07;
	uint8_t ctrl_meas = ((bme->osrs_t & 0x07) << 5)
			| ((bme->osrs_p & 0x07) << 2) | (bme->mode & 0x03);

	HAL_StatusTypeDef ctrl_hum_status = BME280_WriteReg(bme, 0xF2, ctrl_hum);
	HAL_Delay(10);

	if (ctrl_hum_status != HAL_OK) {
		return ctrl_hum_status;
	}

	HAL_StatusTypeDef ctrl_means_status = BME280_WriteReg(bme, 0xF4, ctrl_meas);
	HAL_Delay(10);

	if (ctrl_means_status != HAL_OK) {
		return ctrl_means_status;
	}

	// Calibration data registers (from datasheet):
	// 0x88–0xA1 -> temperature & pressure + dig_H1
	// 0xE1–0xE7 -> humidity calibration
	uint8_t size_calib1 = 26;
	uint8_t size_calib2 = 7;
	uint8_t buffer_calib1[26];
	uint8_t buffer_calib2[7];
	uint8_t reg_start_calib1 = 0x88;
	uint8_t reg_start_calib2 = 0xE1;

	HAL_StatusTypeDef readout_status_calib1;
	HAL_StatusTypeDef readout_status_calib2;

	// Read calibration data from sensor memory
	readout_status_calib1 = BME280_ReadMulti(bme, reg_start_calib1,
			buffer_calib1, size_calib1);
	readout_status_calib2 = BME280_ReadMulti(bme, reg_start_calib2,
			buffer_calib2, size_calib2);

	if (readout_status_calib1 == HAL_OK && readout_status_calib2 == HAL_OK) {
		// temp calibration data
		bme->calib_data.dig_T1 = (buffer_calib1[1] << 8) | buffer_calib1[0];
		bme->calib_data.dig_T2 = (buffer_calib1[3] << 8) | buffer_calib1[2];
		bme->calib_data.dig_T3 = (buffer_calib1[5] << 8) | buffer_calib1[4];

		// pressure calibration data
		bme->calib_data.dig_P1 = (buffer_calib1[7] << 8) | buffer_calib1[6];
		bme->calib_data.dig_P2 = (buffer_calib1[9] << 8) | buffer_calib1[8];
		bme->calib_data.dig_P3 = (buffer_calib1[11] << 8) | buffer_calib1[10];
		bme->calib_data.dig_P4 = (buffer_calib1[13] << 8) | buffer_calib1[12];
		bme->calib_data.dig_P5 = (buffer_calib1[15] << 8) | buffer_calib1[14];
		bme->calib_data.dig_P6 = (buffer_calib1[17] << 8) | buffer_calib1[16];
		bme->calib_data.dig_P7 = (buffer_calib1[19] << 8) | buffer_calib1[18];
		bme->calib_data.dig_P8 = (buffer_calib1[21] << 8) | buffer_calib1[20];
		bme->calib_data.dig_P9 = (buffer_calib1[23] << 8) | buffer_calib1[22];

		// humidity calibration
		bme->calib_data.dig_H1 = buffer_calib1[25];
		bme->calib_data.dig_H2 = (buffer_calib2[1] << 8) | buffer_calib2[0];
		bme->calib_data.dig_H3 = buffer_calib2[2];
		bme->calib_data.dig_H4 = (buffer_calib2[3] << 4)
				| (buffer_calib2[4] & 0x0F);
		bme->calib_data.dig_H5 = (buffer_calib2[5] << 4)
				| ((buffer_calib2[4] & 0xF0) >> 4);
		bme->calib_data.dig_H6 = buffer_calib2[6];

		return HAL_OK;
	} else {
		return HAL_ERROR;
	}

}

// Compensates raw temperature value using calibration data
// Returns temperature in °C * 100 (e.g. 5123 = 51.23°C)
static BME280_S32_t BME280_compensate_T_int32(BME280_HandleTypeDef *bme,
		BME280_S32_t adc_T) {
	BME280_S32_t var1, var2, T;
	var1 = (((adc_T >> 3) - ((BME280_S32_t) bme->calib_data.dig_T1 << 1)))
			* ((BME280_S32_t) bme->calib_data.dig_T2) >> 11;
	var2 = (((((adc_T >> 4) - ((BME280_S32_t) bme->calib_data.dig_T1))
			* ((adc_T >> 4) - ((BME280_S32_t) bme->calib_data.dig_T1))) >> 12)
			* ((BME280_S32_t) bme->calib_data.dig_T3)) >> 14;
	bme->t_fine = var1 + var2;
	T = (bme->t_fine * 5 + 128) >> 8;
	return T;
}

// Compensates raw pressure value using calibration data
// Returns pressure in Pa * 256 (e.g. 24674867 = 96386.2Pa)
static BME280_U32_t BME280_compensate_P_int64(BME280_HandleTypeDef *bme,
		BME280_S32_t adc_P) {
	BME280_S64_t var1, var2, p;
	var1 = ((BME280_S64_t) bme->t_fine) - 12800;
	var2 = var1 * var1 * (BME280_S64_t) bme->calib_data.dig_P6;
	var2 = var2 + ((var1 * (BME280_S64_t) bme->calib_data.dig_P5) << 17);
	var2 = var2 + (((BME280_S64_t) bme->calib_data.dig_P4) << 35);
	var1 = ((var1 * var1 * (BME280_S64_t) bme->calib_data.dig_P3) >> 8)
			+ ((var1 * (BME280_S64_t) bme->calib_data.dig_P2) << 12);
	var1 = ((((BME280_S64_t) 1) << 47) + var1)
			* ((BME280_S64_t) bme->calib_data.dig_P1) >> 33;

	if (var1 == 0) {
		return 0; // Avoid exception caused by division by zero
	}

	p = 1048576 - adc_P;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (((BME280_S64_t) bme->calib_data.dig_P9) * (p >> 13) * (p >> 13))
			>> 25;
	var2 = (((BME280_S64_t) bme->calib_data.dig_P8) * p) >> 19;
	p = ((p + var1 + var2) >> 8)
			+ (((BME280_S64_t) bme->calib_data.dig_P7) << 4);

	return (BME280_U32_t) p;
}

// Compensates raw humidity value using calibration data
// Returns humidity in %RH * 1024 (e.g. 47445 = 46.33 %RH)
static BME280_U32_t BME280_compensate_H_int32(BME280_HandleTypeDef *bme,
		BME280_S32_t adc_H) {
	BME280_S32_t v_x1_u32r;

	v_x1_u32r = (bme->t_fine - ((BME280_S32_t) 76800));
	v_x1_u32r = (((((adc_H << 14)
			- (((BME280_S32_t) bme->calib_data.dig_H4) << 20)
			- (((BME280_S32_t) bme->calib_data.dig_H5) * v_x1_u32r))
			+ ((BME280_S32_t) 16384)) >> 15)
			* (((((((v_x1_u32r * ((BME280_S32_t) bme->calib_data.dig_H6)) >> 10)
					* (((v_x1_u32r * ((BME280_S32_t) bme->calib_data.dig_H3))
							>> 11) + ((BME280_S32_t) 32768))) >> 10)
					+ ((BME280_S32_t) 2097152))
					* ((BME280_S32_t) bme->calib_data.dig_H2) + 8192) >> 14));
	v_x1_u32r = (v_x1_u32r
			- (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7)
					* ((BME280_S32_t) bme->calib_data.dig_H1)) >> 4));
	v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
	v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

	return (BME280_U32_t) (v_x1_u32r >> 12);
}

// Reads raw data from sensor and converts it to physical values:
// temperature [°C], pressure [hPa], humidity [%RH]
HAL_StatusTypeDef BME280_ReadMeasurements(BME280_HandleTypeDef *bme,
		float *temp, float *pressure, float *humidity) {

	// Validate input pointers
	if (bme == NULL || temp == NULL || pressure == NULL || humidity == NULL) {
		return HAL_ERROR;
	}

	uint8_t size_raw_data = 8;
	uint8_t buffer_raw_data[8];
	uint8_t reg_start_raw_data = 0xF7;
	HAL_StatusTypeDef readout_status_raw_data;

	uint32_t raw_temp = 0;
	uint32_t raw_pressure = 0;
	uint16_t raw_humidity = 0;

	readout_status_raw_data = BME280_ReadMulti(bme, reg_start_raw_data,
			buffer_raw_data, size_raw_data);

	if (readout_status_raw_data == HAL_OK) {
		raw_temp = (buffer_raw_data[3] << 12) | (buffer_raw_data[4] << 4)
				| (buffer_raw_data[5] >> 4);

		raw_pressure = (buffer_raw_data[0] << 12) | (buffer_raw_data[1] << 4)
				| (buffer_raw_data[2] >> 4);

		raw_humidity = (buffer_raw_data[6] << 8) | (buffer_raw_data[7]);
	} else {
		return readout_status_raw_data;
	}

	*temp = BME280_compensate_T_int32(bme, raw_temp) / 100.0;
	*pressure = BME280_compensate_P_int64(bme, raw_pressure) / 25600.0;
	*humidity = BME280_compensate_H_int32(bme, raw_humidity) / 1024.0;

	return HAL_OK;
}

HAL_StatusTypeDef BME280_TriggerForcedMeasurement(BME280_HandleTypeDef *bme) {

	bme->mode = BME280_FORCED_MODE;

	uint8_t ctrl_meas = ((bme->osrs_t & 0x07) << 5)
			| ((bme->osrs_p & 0x07) << 2) | (bme->mode & 0x03);

	return BME280_WriteReg(bme, 0xF4, ctrl_meas);
}

HAL_StatusTypeDef BME280_SetOversampling(BME280_HandleTypeDef *bme,
		BME280_Oversampling osrs) {

	HAL_StatusTypeDef ctrl_status;

	bme->osrs_t = osrs;
	bme->osrs_p = osrs;
	bme->osrs_h = osrs;

	uint8_t ctrl_hum = osrs & 0x07;
	uint8_t ctrl_meas = ((osrs & 0x07) << 5) | ((osrs & 0x07) << 2)
			| (bme->mode & 0x03);

	ctrl_status = BME280_WriteReg(bme, 0xF2, ctrl_hum);

	if (ctrl_status == HAL_OK) {
		ctrl_status = BME280_WriteReg(bme, 0xF4, ctrl_meas);

		if (ctrl_status == HAL_OK) {
			bme->osrs_t = osrs;
			bme->osrs_p = osrs;
			bme->osrs_h = osrs;
		}
	}
	return ctrl_status;
}

HAL_StatusTypeDef BME280_ReadConfig(BME280_HandleTypeDef *bme) {
	uint8_t size_params = 3;
	uint8_t buffer_params[3];
	uint8_t reg_start_params = 0xF2;
	HAL_StatusTypeDef readout_params_status;

	readout_params_status = BME280_ReadMulti(bme, reg_start_params,
			buffer_params, size_params);

	if (readout_params_status == HAL_OK) {
		bme->curr_params.update_status = (buffer_params[1] & 0x01);
		bme->curr_params.measuring_status = ((buffer_params[1] & 0x08) >> 2);
		bme->curr_params.mode = ((buffer_params[2]) & 0x03);
		bme->curr_params.osrs_p = ((buffer_params[2] & 0x1C) >> 2);
		bme->curr_params.osrs_t = ((buffer_params[2] & 0xE0) >> 5);
		bme->curr_params.osrs_h = (buffer_params[0] & 0x07);
	}

	return readout_params_status;
}
