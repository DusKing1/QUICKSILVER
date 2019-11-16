#pragma once

#include "defines.h"

#ifdef F405
#include "stm32f4xx_usart.h"
#endif
#ifdef F0
#include "stm32f0xx_usart.h"
#endif

typedef struct {
  uint8_t channel_index;
  USART_TypeDef *channel;

  GPIO_TypeDef *gpio_port;
  uint32_t gpio_af;

  uint8_t rx_pin_index;
  uint32_t rx_pin;
  uint32_t rx_pin_source;
  uint8_t tx_pin_index;
  uint32_t tx_pin;
  uint32_t tx_pin_source;
} usart_port_def_t;

extern usart_port_def_t usart_port_defs[USART_PORTS_MAX];

void serial_init(void);
void serial_enable_rcc(usart_ports_t port);
void serial_enable_isr(usart_ports_t port);

void usart_rx_init(uint8_t RXProtocol);
void serial_smart_audio_init(void);