#include "drv_spi_sdcard.h"

#include <string.h>

#include "drv_spi.h"
#include "drv_time.h"
#include "project.h"

#if defined(STM32F4) && defined(USE_SDCARD)

#define SPI_PORT spi_port_defs[SDCARD_SPI_PORT]
#define NSS_PIN gpio_pin_defs[SDCARD_NSS_PIN]

typedef enum {
  SDCARD_POWER_UP,
  SDCARD_RESET,

  SDCARD_DETECT_INTERFACE,
  SDCARD_DETECT_INIT,
  SDCARD_DETECT_READ_INFO,
  SDCARD_DETECT_FINISH,

  SDCARD_DETECT_FAILED,

  SDCARD_READY,

  SDCARD_READ_MULTIPLE_START,
  SDCARD_READ_MULTIPLE_CONTINUE,
  SDCARD_READ_MULTIPLE_FINISH,
  SDCARD_READ_MULTIPLE_DONE,

  SDCARD_WRITE_MULTIPLE_START,
  SDCARD_WRITE_MULTIPLE_CONTINUE,
  SDCARD_WRITE_MULTIPLE_VERIFY,
  SDCARD_WRITE_MULTIPLE_SECTOR_SUCCESS,
  SDCARD_WRITE_MULTIPLE_FINISH,
  SDCARD_WRITE_MULTIPLE_FINISH_WAIT,
  SDCARD_WRITE_MULTIPLE_DONE

} sdcard_state_t;

typedef struct {
  uint8_t done;

  uint8_t *buf;
  uint32_t sector;
  uint32_t count;
  uint32_t count_done;

} sdcard_operation_t;

static volatile sdcard_state_t state = SDCARD_POWER_UP;
static sdcard_operation_t operation;

// one block plus 1 command byte, 2 crc bytes and 1 response byte

#define TOKEN_SIZE 1
#define CRC_SIZE 2
#define RESPONSE_SIZE 1

static uint8_t dma_block_buffer[SDCARD_BLOCK_SIZE + TOKEN_SIZE + CRC_SIZE + RESPONSE_SIZE];

// how many cycles to delay for write confirm
#define IDLE_BYTES 16

sdcard_info_t sdcard_info;

void sdcard_init() {
  spi_init_pins(SDCARD_SPI_PORT, SDCARD_NSS_PIN);

  spi_enable_rcc(SDCARD_SPI_PORT);

  SPI_I2S_DeInit(SPI_PORT.channel);
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI_PORT.channel, &SPI_InitStructure);
  SPI_Cmd(SPI_PORT.channel, ENABLE);

  // Dummy read to clear receive buffer
  while (SPI_I2S_GetFlagStatus(SPI_PORT.channel, SPI_I2S_FLAG_TXE) == RESET)
    ;

  SPI_I2S_ReceiveData(SPI_PORT.channel);

  spi_dma_init(SDCARD_SPI_PORT);
}

static void sdcard_reinit_fast() {
  SPI_Cmd(SPI_PORT.channel, DISABLE);
  SPI_I2S_DeInit(SPI_PORT.channel);
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI_PORT.channel, &SPI_InitStructure);
  SPI_Cmd(SPI_PORT.channel, ENABLE);
}

static uint8_t sdcard_wait_non_idle() {
  for (uint16_t timeout = 8;; timeout--) {
    if (timeout == 0) {
      return 0xFF;
    }
    const uint8_t ret = spi_transfer_byte(SDCARD_SPI_PORT, 0xff);
    if (ret != 0xFF) {
      return ret;
    }
  }
  return 0xFF;
}

static uint8_t sdcard_wait_for_idle() {
  for (uint16_t timeout = 8;; timeout--) {
    if (timeout == 0) {
      return 0;
    }
    const uint8_t ret = spi_transfer_byte(SDCARD_SPI_PORT, 0xff);
    if (ret == 0xFF) {
      return 1;
    }
  }
  return 0;
}

static void sdcard_select() {
  spi_csn_enable(SDCARD_NSS_PIN);
}

static void sdcard_deselect() {
  timer_delay_us(10);
  spi_csn_disable(SDCARD_NSS_PIN);
}

static uint8_t sdcard_command(const uint8_t cmd, const uint32_t args) {
  if (cmd != SDCARD_GO_IDLE && !sdcard_wait_for_idle()) {
    return 0xFF;
  }

  spi_transfer_byte(SDCARD_SPI_PORT, 0x40 | cmd);
  spi_transfer_byte(SDCARD_SPI_PORT, args >> 24);
  spi_transfer_byte(SDCARD_SPI_PORT, args >> 16);
  spi_transfer_byte(SDCARD_SPI_PORT, args >> 8);
  spi_transfer_byte(SDCARD_SPI_PORT, args >> 0);

  // we have to send CRC while we are still in SD Bus mode
  switch (cmd) {
  case SDCARD_GO_IDLE:
    spi_transfer_byte(SDCARD_SPI_PORT, 0x95);
    break;
  case SDCARD_IF_COND:
    spi_transfer_byte(SDCARD_SPI_PORT, 0x87);
    break;
  default:
    spi_transfer_byte(SDCARD_SPI_PORT, 0xff);
    break;
  }

  return sdcard_wait_non_idle();
}

static uint8_t sdcard_app_command(const uint8_t cmd, const uint32_t args) {
  sdcard_command(SDCARD_APP_CMD, 0);
  return sdcard_command(cmd, args);
}

uint32_t sdcard_read_response() {
  return (spi_transfer_byte(SDCARD_SPI_PORT, 0xff) << 24) |
         (spi_transfer_byte(SDCARD_SPI_PORT, 0xff) << 16) |
         (spi_transfer_byte(SDCARD_SPI_PORT, 0xff) << 8) |
         (spi_transfer_byte(SDCARD_SPI_PORT, 0xff) << 0);
}

void sdcard_read_data(uint8_t *buf, const uint32_t size) {
  // wait for data token
  uint8_t token = sdcard_wait_non_idle();
  if (token != 0xFE) {
    return;
  }

  //spi_dma_transfer_bytes(SDCARD_SPI_PORT, buf, size);
  for (uint32_t i = 0; i < size; i++) {
    buf[i] = spi_transfer_byte(SDCARD_SPI_PORT, 0xff);
  }

  // two bytes CRC
  spi_transfer_byte(SDCARD_SPI_PORT, 0xff);
  spi_transfer_byte(SDCARD_SPI_PORT, 0xff);
}

void sdcard_write_data(const uint8_t token, const uint8_t *buf, const uint32_t size) {
  // start block
  spi_transfer_byte(SDCARD_SPI_PORT, token);

  spi_dma_transfer_bytes(SDCARD_SPI_PORT, (uint8_t *)buf, size);

  // two bytes CRC
  spi_transfer_byte(SDCARD_SPI_PORT, 0xff);
  spi_transfer_byte(SDCARD_SPI_PORT, 0xff);

  // write response
  spi_transfer_byte(SDCARD_SPI_PORT, 0xff);
}

uint8_t sdcard_update() {
  static uint32_t delay_loops = 1000;
  if (delay_loops > 0) {
    delay_loops--;
    return 0;
  }

  switch (state) {
  case SDCARD_POWER_UP:
    spi_csn_disable(SDCARD_NSS_PIN);
    for (uint32_t i = 0; i < 20; i++) {
      spi_transfer_byte(SDCARD_SPI_PORT, 0xff);
    }
    state = SDCARD_RESET;
    delay_loops = 100;
    break;

  case SDCARD_RESET: {
    static uint32_t tries = 0;
    sdcard_select();
    uint8_t ret = sdcard_command(SDCARD_GO_IDLE, 0);
    if (ret == 0x01) {
      state = SDCARD_DETECT_INTERFACE;
    } else {
      tries++;
    }
    if (tries == 10) {
      tries = 0;
      delay_loops = 100;
      state = SDCARD_POWER_UP;
    }
    sdcard_deselect();
    break;
  }
  case SDCARD_DETECT_INTERFACE: {
    sdcard_select();
    uint8_t ret = sdcard_command(SDCARD_IF_COND, 0x1AA);
    if (ret == SDCARD_R1_IDLE) {
      uint32_t voltage_check = sdcard_read_response();
      if (voltage_check == 0x1AA) {
        // voltage check passed, got version 2
        sdcard_info.version = 2;
        state = SDCARD_DETECT_INIT;
      } else {
        state = SDCARD_DETECT_FAILED;
      }
    } else if (ret == (SDCARD_R1_ILLEGAL_COMMAND | SDCARD_R1_IDLE)) {
      // did respond with correct error, must be v1
      sdcard_info.version = 1;
      state = SDCARD_DETECT_INIT;
    } else {
      // ???
      state = SDCARD_DETECT_FAILED;
    }
    sdcard_deselect();
    break;
  }
  case SDCARD_DETECT_INIT: {
    sdcard_select();
    uint8_t ret = sdcard_app_command(SDCARD_ACMD_OD_COND, sdcard_info.version == 2 ? (1 << 30) : 0);
    if (ret == 0x0) {
      state = SDCARD_DETECT_READ_INFO;
    }
    sdcard_deselect();
    break;
  }

  case SDCARD_DETECT_READ_INFO: {
    sdcard_select();
    uint8_t ret = sdcard_command(SDCARD_OCR, 0);
    if (ret != 0x0) {
      sdcard_deselect();
      state = SDCARD_DETECT_FAILED;
      break;
    }
    sdcard_info.ocr = sdcard_read_response();
    sdcard_info.high_capacity = (sdcard_info.ocr & (1 << 30)) != 0;
    sdcard_deselect();

    sdcard_select();
    ret = sdcard_command(SDCARD_CID, 0);
    if (ret != 0x0) {
      sdcard_deselect();
      state = SDCARD_DETECT_FAILED;
      break;
    }
    sdcard_read_data((uint8_t *)&sdcard_info.cid, 16);
    sdcard_deselect();

    sdcard_select();
    ret = sdcard_command(SDCARD_CSD, 0);
    if (ret != 0x0) {
      sdcard_deselect();
      state = SDCARD_DETECT_FAILED;
      break;
    }
    sdcard_read_data((uint8_t *)&sdcard_info.csd, 16);
    sdcard_deselect();

    state = SDCARD_DETECT_FINISH;
    break;
  }

  case SDCARD_DETECT_FINISH: {
    if (!sdcard_info.high_capacity) {
      sdcard_select();
      uint8_t ret = sdcard_command(SDACARD_SET_BLOCK_LEN, SDCARD_BLOCK_SIZE);
      if (ret != 0x0) {
        state = SDCARD_DETECT_FAILED;
        break;
      }
      sdcard_deselect();
    }

    sdcard_reinit_fast();
    state = SDCARD_READY;
    break;
  }

  case SDCARD_READ_MULTIPLE_START: {
    uint8_t token = sdcard_wait_non_idle();
    if (token == 0xFE) {
      state = SDCARD_READ_MULTIPLE_CONTINUE;

      memset(dma_block_buffer, 0xFF, SDCARD_BLOCK_SIZE + CRC_SIZE);
      spi_dma_transfer_begin(SDCARD_SPI_PORT, (uint8_t *)&dma_block_buffer, SDCARD_BLOCK_SIZE + CRC_SIZE);
    }
    break;
  }

  case SDCARD_READ_MULTIPLE_CONTINUE: {
    if (!spi_dma_is_ready(SDCARD_SPI_PORT)) {
      break;
    }

    memcpy(operation.buf + operation.count_done * SDCARD_BLOCK_SIZE, dma_block_buffer, SDCARD_BLOCK_SIZE);
    operation.count_done++;

    if (operation.count_done == operation.count) {
      state = SDCARD_READ_MULTIPLE_FINISH;
    } else {
      uint8_t token = sdcard_wait_non_idle();
      if (token == 0xFE) {
        memset(dma_block_buffer, 0xFF, SDCARD_BLOCK_SIZE + CRC_SIZE);
        spi_dma_transfer_begin(SDCARD_SPI_PORT, (uint8_t *)&dma_block_buffer, SDCARD_BLOCK_SIZE + CRC_SIZE);
      }
    }

    break;
  }

  case SDCARD_READ_MULTIPLE_FINISH: {
    sdcard_command(SDCARD_STOP_TRANSMISSION, 0);
    sdcard_deselect();

    state = SDCARD_READ_MULTIPLE_DONE;
    break;
  }

  case SDCARD_WRITE_MULTIPLE_START: {
    // wait
    break;
  }

  case SDCARD_WRITE_MULTIPLE_CONTINUE: {
    if (!spi_dma_is_ready(SDCARD_SPI_PORT)) {
      break;
    }
    if (operation.count == operation.count_done) {
      break;
    }

    dma_block_buffer[0] = 0xFC;
    memcpy(dma_block_buffer + TOKEN_SIZE, operation.buf, SDCARD_BLOCK_SIZE);

    // two bytes CRC
    dma_block_buffer[SDCARD_BLOCK_SIZE + TOKEN_SIZE + 0] = 0xFF;
    dma_block_buffer[SDCARD_BLOCK_SIZE + TOKEN_SIZE + 1] = 0xFF;

    // write response
    dma_block_buffer[SDCARD_BLOCK_SIZE + TOKEN_SIZE + 2] = 0xFF;

    state = SDCARD_WRITE_MULTIPLE_VERIFY;
    spi_dma_transfer_begin(SDCARD_SPI_PORT, (uint8_t *)&dma_block_buffer, SDCARD_BLOCK_SIZE + TOKEN_SIZE + CRC_SIZE + RESPONSE_SIZE);
    break;
  }

  case SDCARD_WRITE_MULTIPLE_VERIFY: {
    if (!spi_dma_is_ready(SDCARD_SPI_PORT)) {
      break;
    }
    if (!sdcard_wait_for_idle()) {
      break;
    }

    operation.count_done++;
    state = SDCARD_WRITE_MULTIPLE_SECTOR_SUCCESS;
    break;
  }

  case SDCARD_WRITE_MULTIPLE_SECTOR_SUCCESS: {
    // wait
    break;
  }

  case SDCARD_WRITE_MULTIPLE_FINISH: {
    spi_transfer_byte(SDCARD_SPI_PORT, 0xff);
    spi_transfer_byte(SDCARD_SPI_PORT, 0xfd);

    memset(dma_block_buffer, 0x0, IDLE_BYTES);

    state = SDCARD_WRITE_MULTIPLE_FINISH_WAIT;
    break;
  }

  case SDCARD_WRITE_MULTIPLE_FINISH_WAIT: {
    if (!spi_dma_is_ready(SDCARD_SPI_PORT)) {
      break;
    }

    if (dma_block_buffer[IDLE_BYTES - 1] == 0xFF) {
      sdcard_deselect();
      state = SDCARD_WRITE_MULTIPLE_DONE;
      break;
    }

    memset(dma_block_buffer, 0xFF, IDLE_BYTES);
    spi_dma_transfer_begin(SDCARD_SPI_PORT, (uint8_t *)&dma_block_buffer, IDLE_BYTES);
    break;
  }

  case SDCARD_READY:
    return 1;

  case SDCARD_WRITE_MULTIPLE_DONE:
  case SDCARD_READ_MULTIPLE_DONE:
  case SDCARD_DETECT_FAILED:
    return 0;
  }

  return 0;
}

uint8_t sdcard_read_sectors(uint8_t *buf, uint32_t sector, uint32_t count) {
  if (state != SDCARD_READY) {
    if (state == SDCARD_READ_MULTIPLE_DONE) {
      state = SDCARD_READY;
      return 1;
    }
    return 0;
  }

  sdcard_select();

  const uint32_t addr = sdcard_info.high_capacity ? sector : sector * SDCARD_BLOCK_SIZE;

  uint8_t ret = sdcard_command(SDCARD_READ_MULTIPLE_BLOCK, addr);
  if (ret != 0x0) {
    sdcard_deselect();
    return 0;
  }

  operation.done = 0;
  operation.buf = buf;
  operation.sector = sector;
  operation.count = count;
  operation.count_done = 0;

  state = SDCARD_READ_MULTIPLE_START;
  return 0;
}

uint8_t sdcard_write_sectors_start(uint32_t sector, uint32_t count) {
  if (state != SDCARD_READY) {
    if (state == SDCARD_WRITE_MULTIPLE_START) {
      return 1;
    }
    return 0;
  }

  sdcard_select();

  if (sdcard_app_command(SDCARD_ACMD_SET_WR_BLK_ERASE_COUNT, count * 2) != 0x0) {
    sdcard_deselect();
    return 0;
  }

  const uint32_t addr = sdcard_info.high_capacity ? sector : sector * SDCARD_BLOCK_SIZE;
  if (sdcard_command(SDCARD_WRITE_MULTIPLE_BLOCK, addr) != 0x0) {
    sdcard_deselect();
    return 0;
  }

  operation.done = 0;
  operation.buf = NULL;
  operation.sector = sector;
  operation.count = 0;
  operation.count_done = 0;

  state = SDCARD_WRITE_MULTIPLE_START;
  return 0;
}

uint8_t sdcard_write_sectors_continue(uint8_t *buf) {
  if (state == SDCARD_WRITE_MULTIPLE_START) {
    operation.buf = buf;
    operation.count++;

    state = SDCARD_WRITE_MULTIPLE_CONTINUE;
    return 0;
  }

  if (state == SDCARD_WRITE_MULTIPLE_SECTOR_SUCCESS) {
    state = SDCARD_WRITE_MULTIPLE_START;
    return 1;
  }

  return 0;
}

uint8_t sdcard_write_sectors_finish() {
  if (state == SDCARD_WRITE_MULTIPLE_SECTOR_SUCCESS) {
    state = SDCARD_WRITE_MULTIPLE_START;
    return 0;
  }
  if (state == SDCARD_WRITE_MULTIPLE_START) {
    state = SDCARD_WRITE_MULTIPLE_FINISH;
    return 0;
  }
  if (state == SDCARD_WRITE_MULTIPLE_DONE) {
    state = SDCARD_READY;
    return 1;
  }

  return 0;
}

uint8_t sdcard_write_sector(uint8_t *buf, uint32_t sector) {
  if (state == SDCARD_READY) {
    sdcard_write_sectors_start(sector, 1);
  }
  if (state == SDCARD_WRITE_MULTIPLE_SECTOR_SUCCESS) {
    sdcard_write_sectors_continue(buf);
  }
  if (state == SDCARD_WRITE_MULTIPLE_DONE) {
    if (sdcard_write_sectors_finish()) {
      return 1;
    }
  }
  if (state == SDCARD_WRITE_MULTIPLE_START) {
    if (operation.count_done == 0) {
      sdcard_write_sectors_continue(buf);
    } else {
      sdcard_write_sectors_finish();
    }
  }
  return 0;
}

void sdcard_dma_rx_isr() {
  // spi_csn_disable(SDCARD_NSS_PIN);
}

#endif