#include "driver/spi_asm330.h"

#include "core/project.h"
#include "driver/gpio.h"
#include "driver/spi.h"
#include "driver/spi_gyro.h"
#include "driver/time.h"
#include "util/util.h"

#ifdef USE_GYRO

// 10 MHz max SPI frequency
#define ASM330LHH_MAX_SPI_CLK_HZ MHZ_TO_HZ(10)
#define ASM330LHH_INIT_SPI_CLK_HZ MHZ_TO_HZ(0.5)

//WHOAMI
#define ASM330LHH_CHIP_ID 0x6B



extern spi_bus_device_t gyro_bus;

uint8_t asm330_detect() {
  const uint8_t id = asm330_read(ASM330LHH_REG_WHO_AM_I);
  if (ASM330LHH_CHIP_ID==id) {
    return GYRO_TYPE_ASM330;
  }else{
    return GYRO_TYPE_INVALID;
  }
}



uint8_t asm330_read(uint8_t reg) {
  spi_bus_device_reconfigure(&gyro_bus, SPI_MODE_TRAILING_EDGE, ASM330LHH_INIT_SPI_CLK_HZ);

  uint8_t buffer[2] = {reg | 0x80, 0x00};

  const spi_txn_segment_t segs[] = {
      spi_make_seg_buffer(buffer, buffer, 2),
  };
  spi_seg_submit_wait(&gyro_bus, segs);

  return buffer[1];
}

void asm330_write(uint8_t reg, uint8_t data) {
  spi_bus_device_reconfigure(&gyro_bus, SPI_MODE_TRAILING_EDGE, ASM330LHH_INIT_SPI_CLK_HZ);

  const spi_txn_segment_t segs[] = {
      spi_make_seg_const(reg),
      spi_make_seg_const(data),
  };
  spi_seg_submit_wait(&gyro_bus, segs);
}

void asm330_setRegwithMask(asm330lhhRegister_e registerID, asm330lhhConfigMasks_e mask, uint8_t value, uint32_t delayMs){
    uint8_t newValue=0;
    newValue=asm330_read(registerID);
    time_delay_ms(2);
    newValue = (newValue & ~mask) | value;
    asm330_write(registerID, newValue);
    time_delay_ms(delayMs);
}

void asm330_configure() {
  
    // Reset the device (wait 100ms before continuing config)
    asm330_setRegwithMask(ASM330LHH_REG_CTRL3_C, ASM330LHH_MASK_CTRL3_C_RESET, BIT(0), 100);

    // Configure data ready pulsed mode
    asm330_setRegwithMask(ASM330LHH_REG_COUNTER_BDR1, ASM330LHH_MASK_COUNTER_BDR1, ASM330LHH_VAL_COUNTER_BDR1_DDRY_PM, 0);

    // Configure interrupt pin 1 for gyro data ready only
    asm330_write(ASM330LHH_REG_INT1_CTRL, ASM330LHH_VAL_INT1_CTRL);
    time_delay_ms(1);

    // Disable interrupt pin 2
    asm330_write(ASM330LHH_REG_INT2_CTRL, ASM330LHH_VAL_INT2_CTRL);
    time_delay_ms(1);

    // Configure the accelerometer
    // 833hz ODR, 16G scale, use LPF2 output (default with ODR/4 cutoff)
    asm330_write( ASM330LHH_REG_CTRL1_XL, (ASM330LHH_VAL_CTRL1_XL_ODR833 << 4) | (ASM330LHH_VAL_CTRL1_XL_16G << 2) | (ASM330LHH_VAL_CTRL1_XL_LPF2 << 1));
    time_delay_ms(1);

    // Configure the gyro
    // 6664hz ODR, 2000dps scale
    asm330_write( ASM330LHH_REG_CTRL2_G, (ASM330LHH_VAL_CTRL2_G_ODR6664 << 4) | (ASM330LHH_VAL_CTRL2_G_2000DPS << 2));
    time_delay_ms(1);
    
    // Configure control register 3
    // latch LSB/MSB during reads; set interrupt pins active high; set interrupt pins push/pull; set 4-wire SPI; enable auto-increment burst reads
    asm330_setRegwithMask( ASM330LHH_REG_CTRL3_C, ASM330LHH_MASK_CTRL3_C, (ASM330LHH_VAL_CTRL3_C_BDU | ASM330LHH_VAL_CTRL3_C_H_LACTIVE | ASM330LHH_VAL_CTRL3_C_PP_OD | ASM330LHH_VAL_CTRL3_C_SIM | ASM330LHH_VAL_CTRL3_C_IF_INC), 1);

    // Configure control register 4
    // enable accelerometer high performane mode; enable gyro LPF1
    asm330_setRegwithMask( ASM330LHH_REG_CTRL4_C, ASM330LHH_MASK_CTRL4_C, (ASM330LHH_VAL_CTRL4_C_DRDY_MASK | ASM330LHH_VAL_CTRL4_C_I2C_DISABLE | ASM330LHH_VAL_CTRL4_C_LPF1_SEL_G), 1);

    // Configure control register 6
    // disable I2C interface; set gyro LPF1 cutoff according to gyro_hardware_lpf setting
    asm330_setRegwithMask(ASM330LHH_REG_CTRL6_C, ASM330LHH_MASK_CTRL6_C, (ASM330LHH_VAL_CTRL6_C_XL_HM_MODE |ASM330LHH_VAL_CTRL6_C_FTYPE_223HZ), 1);

    // // Configure control register 7
    // asm330lhhWriteRegisterBits(dev, ASM330LHH_REG_CTRL7_G, ASM330LHH_MASK_CTRL7_G, (ASM330LHH_VAL_CTRL7_G_HP_EN_G | ASM330LHH_VAL_CTRL7_G_HPM_G_16), 1);

    // Configure control register 9
    // Enables the proper device configuration
    asm330_setRegwithMask( ASM330LHH_REG_CTRL9_XL, ASM330LHH_MASK_CTRL9_XL, ASM330LHH_VAL_CTRL9_XL_DEVICE_CONF, 1);

}

void asm330_read_data(uint8_t reg, uint8_t *data, uint32_t size) {
  spi_bus_device_reconfigure(&gyro_bus, SPI_MODE_TRAILING_EDGE, ASM330LHH_MAX_SPI_CLK_HZ);

  const spi_txn_segment_t segs[] = {
      spi_make_seg_const(reg | 0x80),
      spi_make_seg_buffer(data, NULL, size),
  };
  spi_seg_submit_wait(&gyro_bus, segs);
}
#endif