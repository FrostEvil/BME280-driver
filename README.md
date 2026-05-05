# BME280 STM32 HAL Driver

Lightweight driver for the BME280 environmental sensor (temperature, pressure, humidity) using STM32 HAL and I2C.

## Features

* BME280 initialization (chip ID check + configuration)
* Temperature measurement [°C]
* Pressure measurement [hPa]
* Humidity measurement [%RH]
* Calibration data handling based on datasheet
* Simple blocking I2C implementation (HAL)

## Requirements

* STM32 microcontroller
* STM32 HAL (CubeMX / CubeIDE)
* I2C peripheral configured
* BME280 sensor (I2C)

## File Structure

```
.
├── bme280.c
├── bme280.h
├── main.c        # example usage
└── README.md
```

## Getting Started

### 1. Add files to your project

Copy:

* `bme280.c`
* `bme280.h`

into your STM32 project.

---

### 2. Configure the driver

```c
BME280_HandleTypeDef bme;

bme.hi2c = &hi2c1;
bme.address = 0x76 << 1; // or 0x77 << 1

bme.osrs_t = BME280_OSRS_X1;
bme.osrs_p = BME280_OSRS_X1;
bme.osrs_h = BME280_OSRS_X1;
bme.mode   = BME280_NORMAL_MODE;
```

---

### 3. Initialize the sensor

```c
if (BME280_Init(&bme) != HAL_OK) {
    // handle error
}
```

---

### 4. Read measurements

```c
float temperature, pressure, humidity;

if (BME280_ReadMeasurements(&bme, &temperature, &pressure, &humidity) == HAL_OK) {
    // temperature in °C
    // pressure in hPa
    // humidity in %
}
```

---

## Output Units

| Parameter   | Unit |
| ----------- | ---- |
| Temperature | °C   |
| Pressure    | hPa  |
| Humidity    | %RH  |

---

## Notes

* I2C address depends on wiring:

  * `0x76` (SDO = GND)
  * `0x77` (SDO = VCC)
* HAL expects shifted address (`<< 1`)
* Driver uses blocking I2C (no DMA / interrupt)

---

## UART `printf` support (retarget)

This project uses a simple `retarget.c` file to redirect `printf()` output to UART.

It works by implementing the `__io_putchar()` function, which is called internally by `printf`. Each character is then sent using `HAL_UART_Transmit()`.

Example implementation:

```c
extern UART_HandleTypeDef huart2;

int __io_putchar(uint8_t ch) {
    HAL_UART_Transmit(&huart2, &ch, 1, 10);
    return ch;
}
```

### Notes

* The UART handle (`huart2`) must match your CubeMX configuration
* Make sure UART is initialized before using `printf()`
* This is a blocking implementation (simple but sufficient for debugging)
* Requires proper linker settings (usually already configured in STM32CubeIDE)

This file is optional but recommended for debugging and data logging over UART.

## Possible Improvements

* Forced mode support (low power applications)
* Non-blocking I2C (DMA / Interrupt)
* Timeout handling improvements
* Data filtering / averaging
* Support for multiple sensors
* Configuration API (separate from init)

---

## Example

```c
while (1)
{
    if (BME280_ReadMeasurements(&bme, &temp, &pressure, &humidity) == HAL_OK)
    {
        printf("T: %.2f C\r\n", temp);
        printf("P: %.2f hPa\r\n", pressure);
        printf("H: %.2f %%\r\n", humidity);
    }

    HAL_Delay(1000);
}
```

---

## License

MIT License (you can change it if needed)

---

## Author

TS embedded practice project
