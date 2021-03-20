#include "config.h"
#include "config_helper.h"

#define CrazyBee_F4

#define F4
#define F411

//PORTS
#define SPI_PORTS   \
  SPI1_PA5PA6PA7    \
  SPI2_PB13PB14PB15 \
  SPI3_PB3PB4PB5

#define USART_PORTS \
  USART1_PA10PA9    \
  USART2_PA3PA2

//LEDS
#define LED_NUMBER 1
#define LED1PIN PIN_C13
#define LED1_INVERT
#define BUZZER_PIN GPIO_Pin_15
#define BUZZER_PIN_PORT GPIOC

//#define FPV_PIN GPIO_Pin_13
//#define FPV_PORT GPIOA

//GYRO
#define MPU6XXX_SPI_PORT SPI_PORT1
#define MPU6XXX_NSS PIN_A4
#define MPU6XXX_INT PIN_A1
#define USE_DUMMY_I2C
#define GYRO_ID_1 0x68
#define GYRO_ID_2 0x73
#define GYRO_ID_3 0x78
#define GYRO_ID_4 0x71

//RADIO
#ifdef RX_FRSKY
#define USE_CC2500
#define CC2500_SPI_PORT SPI_PORT3
#define CC2500_NSS PIN_A15
#define CC2500_GDO0_PIN PIN_C14

#define SOFTSPI_NONE
#endif

#ifdef SERIAL_RX
#define RX_USART USART_PORT2
#define SOFTSPI_NONE
#endif
#ifndef SOFTSPI_NONE
#define RADIO_CHECK
#define SPI_MISO_PIN GPIO_Pin_10
#define SPI_MISO_PORT GPIOA
#define SPI_MOSI_PIN GPIO_Pin_9
#define SPI_MOSI_PORT GPIOA
#define SPI_CLK_PIN GPIO_Pin_3
#define SPI_CLK_PORT GPIOA
#define SPI_SS_PIN GPIO_Pin_2
#define SPI_SS_PORT GPIOA
#endif

// OSD
#define ENABLE_OSD
#define MAX7456_SPI_PORT SPI_PORT2
#define MAX7456_NSS PIN_B12

//VOLTAGE DIVIDER
#define BATTERYPIN GPIO_Pin_0
#define BATTERYPORT GPIOB
#define BATTERY_ADC_CHANNEL ADC_Channel_8

#ifndef VOLTAGE_DIVIDER_R1
#define VOLTAGE_DIVIDER_R1 10000
#endif

#ifndef VOLTAGE_DIVIDER_R2
#define VOLTAGE_DIVIDER_R2 1000
#endif

#ifndef ADC_REF_VOLTAGE
#define ADC_REF_VOLTAGE 3.3
#endif

// MOTOR PINS
#define MOTOR_PIN0 MOTOR_PIN_PB7
#define MOTOR_PIN1 MOTOR_PIN_PB8
#define MOTOR_PIN2 MOTOR_PIN_PB10
#define MOTOR_PIN3 MOTOR_PIN_PB6
