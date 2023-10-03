#pragma once

#include <stdint.h>


#define BIT(N) (1<<(N))

// ASM330LHH register configuration values
typedef enum {
    ASM330LHH_VAL_COUNTER_BDR1_DDRY_PM = BIT(7),// (bit 7) enable data ready pulsed mode
    ASM330LHH_VAL_INT1_CTRL = 0x02,             // enable gyro data ready interrupt pin 1
    ASM330LHH_VAL_INT2_CTRL = 0x00,             // disable gyro data ready interrupt pin 2
    ASM330LHH_VAL_CTRL1_XL_ODR833 = 0x07,       // accelerometer 833hz output data rate (gyro/8)
    ASM330LHH_VAL_CTRL1_XL_ODR1667 = 0x08,      // accelerometer 1666hz output data rate (gyro/4)
    ASM330LHH_VAL_CTRL1_XL_ODR3332 = 0x09,      // accelerometer 3332hz output data rate (gyro/2)
    ASM330LHH_VAL_CTRL1_XL_ODR6664 = 0x0A,      // accelerometer 6664hz output data rate (gyro/1)
    ASM330LHH_VAL_CTRL1_XL_8G = 0x03,           // accelerometer 8G scale
    ASM330LHH_VAL_CTRL1_XL_16G = 0x01,          // accelerometer 16G scale
    ASM330LHH_VAL_CTRL1_XL_LPF1 = 0x00,         // accelerometer output from LPF1
    ASM330LHH_VAL_CTRL1_XL_LPF2 = 0x01,         // accelerometer output from LPF2
    ASM330LHH_VAL_CTRL2_G_ODR6664 = 0x0A,       // gyro 6664hz output data rate
    ASM330LHH_VAL_CTRL2_G_2000DPS = 0x03,       // gyro 2000dps scale
    ASM330LHH_VAL_CTRL3_C_BDU = BIT(6),         // (bit 6) output registers are not updated until MSB and LSB have been read (prevents MSB from being updated while burst reading LSB/MSB)
    ASM330LHH_VAL_CTRL3_C_H_LACTIVE = 0,        // (bit 5) interrupt pins active high
    ASM330LHH_VAL_CTRL3_C_PP_OD = 0,            // (bit 4) interrupt pins push/pull
    ASM330LHH_VAL_CTRL3_C_SIM = 0,              // (bit 3) SPI 4-wire interface mode
    ASM330LHH_VAL_CTRL3_C_IF_INC = BIT(2),      // (bit 2) auto-increment address for burst reads
    ASM330LHH_VAL_CTRL4_C_DRDY_MASK = BIT(3),   // (bit 3) data ready interrupt mask
    ASM330LHH_VAL_CTRL4_C_I2C_DISABLE = BIT(2), // (bit 2) disable I2C interface
    ASM330LHH_VAL_CTRL4_C_LPF1_SEL_G = BIT(1),  // (bit 1) enable gyro LPF1
    ASM330LHH_VAL_CTRL6_C_XL_HM_MODE = 0,       // (bit 4) enable accelerometer high performance mode
    ASM330LHH_VAL_CTRL6_C_FTYPE_297HZ = 0x00,   // (bits 2:0) gyro LPF1 cutoff 297Hz
    ASM330LHH_VAL_CTRL6_C_FTYPE_223HZ = 0x01,   // (bits 2:0) gyro LPF1 cutoff 223Hz
    ASM330LHH_VAL_CTRL6_C_FTYPE_154HZ = 0x02,   // (bits 2:0) gyro LPF1 cutoff 154Hz
    ASM330LHH_VAL_CTRL6_C_FTYPE_470HZ = 0x03,   // (bits 2:0) gyro LPF1 cutoff 470Hz
    ASM330LHH_VAL_CTRL7_G_HP_EN_G = BIT(6),     // (bit 6) enable gyro high-pass filter
    ASM330LHH_VAL_CTRL7_G_HPM_G_16 = 0x00,      // (bits 5:4) gyro HPF cutoff 16mHz
    ASM330LHH_VAL_CTRL7_G_HPM_G_65 = 0x01,      // (bits 5:4) gyro HPF cutoff 65mHz
    ASM330LHH_VAL_CTRL7_G_HPM_G_260 = 0x02,     // (bits 5:4) gyro HPF cutoff 260mHz
    ASM330LHH_VAL_CTRL7_G_HPM_G_1040 = 0x03,    // (bits 5:4) gyro HPF cutoff 1.04Hz
    ASM330LHH_VAL_CTRL9_XL_DEVICE_CONF = BIT(1),// (bit 1) Enables the proper device configuration
} asm330lhhConfigValues_e;

// ASM330LHH register configuration bit masks
typedef enum {
    ASM330LHH_MASK_COUNTER_BDR1 = 0x80,    // 0b10000000
    ASM330LHH_MASK_CTRL3_C = 0x7C,         // 0b01111100
    ASM330LHH_MASK_CTRL3_C_RESET = BIT(0), // 0b00000001
    ASM330LHH_MASK_CTRL4_C = 0x0E,         // 0b00001110
    ASM330LHH_MASK_CTRL6_C = 0x17,         // 0b00010111
    ASM330LHH_MASK_CTRL7_G = 0x70,         // 0b01110000
    ASM330LHH_MASK_CTRL9_XL = 0x02,        // 0b00000010
} asm330lhhConfigMasks_e;

// ASM330LHH registers (not the complete list)
typedef enum {
    ASM330LHH_REG_COUNTER_BDR1 = 0x0B,// Counter batch data rate register
    ASM330LHH_REG_INT1_CTRL = 0x0D,  // int pin 1 control
    ASM330LHH_REG_INT2_CTRL = 0x0E,  // int pin 2 control
    ASM330LHH_REG_WHO_AM_I = 0x0F,   // chip ID
    ASM330LHH_REG_CTRL1_XL = 0x10,   // accelerometer control
    ASM330LHH_REG_CTRL2_G = 0x11,    // gyro control
    ASM330LHH_REG_CTRL3_C = 0x12,    // control register 3
    ASM330LHH_REG_CTRL4_C = 0x13,    // control register 4
    ASM330LHH_REG_CTRL5_C = 0x14,    // control register 5
    ASM330LHH_REG_CTRL6_C = 0x15,    // control register 6
    ASM330LHH_REG_CTRL7_G = 0x16,    // control register 7
    ASM330LHH_REG_CTRL8_XL = 0x17,   // control register 8
    ASM330LHH_REG_CTRL9_XL = 0x18,   // control register 9
    ASM330LHH_REG_CTRL10_C = 0x19,   // control register 10
    ASM330LHH_REG_STATUS = 0x1E,     // status register
    ASM330LHH_REG_OUT_TEMP_L = 0x20, // temperature LSB
    ASM330LHH_REG_OUT_TEMP_H = 0x21, // temperature MSB
    ASM330LHH_REG_OUTX_L_G = 0x22,   // gyro X axis LSB
    ASM330LHH_REG_OUTX_H_G = 0x23,   // gyro X axis MSB
    ASM330LHH_REG_OUTY_L_G = 0x24,   // gyro Y axis LSB
    ASM330LHH_REG_OUTY_H_G = 0x25,   // gyro Y axis MSB
    ASM330LHH_REG_OUTZ_L_G = 0x26,   // gyro Z axis LSB
    ASM330LHH_REG_OUTZ_H_G = 0x27,   // gyro Z axis MSB
    ASM330LHH_REG_OUTX_L_A = 0x28,   // acc X axis LSB
    ASM330LHH_REG_OUTX_H_A = 0x29,   // acc X axis MSB
    ASM330LHH_REG_OUTY_L_A = 0x2A,   // acc Y axis LSB
    ASM330LHH_REG_OUTY_H_A = 0x2B,   // acc Y axis MSB
    ASM330LHH_REG_OUTZ_L_A = 0x2C,   // acc Z axis LSB
    ASM330LHH_REG_OUTZ_H_A = 0x2D,   // acc Z axis MSB
} asm330lhhRegister_e;

uint8_t asm330_detect();
void asm330_configure();

void asm330_write(uint8_t reg, uint8_t data);

uint8_t asm330_read(uint8_t reg);
void asm330_read_data(uint8_t reg, uint8_t *data, uint32_t size);