// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Lucian Copeland for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

// I2C
#define I2C_BANK_ARRAY_LEN 3
extern I2C_TypeDef *mcu_i2c_banks[I2C_BANK_ARRAY_LEN];

#define SDA_LIST_LEN 8
#define SCL_LIST_LEN 4
extern const mcu_periph_obj_t mcu_i2c_sda_list[SDA_LIST_LEN];
extern const mcu_periph_obj_t mcu_i2c_scl_list[SCL_LIST_LEN];

// SPI
#define SPI_BANK_ARRAY_LEN 5
extern SPI_TypeDef *mcu_spi_banks[SPI_BANK_ARRAY_LEN];

#define SCK_LIST_LEN 15
#define MOSI_LIST_LEN 14
#define MISO_LIST_LEN 12
#define NSS_LIST_LEN 12
extern const mcu_periph_obj_t mcu_spi_sck_list[SCK_LIST_LEN];
extern const mcu_periph_obj_t mcu_spi_mosi_list[MOSI_LIST_LEN];
extern const mcu_periph_obj_t mcu_spi_miso_list[MISO_LIST_LEN];
extern const mcu_periph_obj_t mcu_spi_nss_list[NSS_LIST_LEN];

// UART
// #define MAX_UART 8
extern USART_TypeDef *mcu_uart_banks[MAX_UART];
extern bool mcu_uart_has_usart[MAX_UART];

#define UART_TX_LIST_LEN 11
#define UART_RX_LIST_LEN 12
extern const mcu_periph_obj_t mcu_uart_tx_list[UART_TX_LIST_LEN];
extern const mcu_periph_obj_t mcu_uart_rx_list[UART_RX_LIST_LEN];

// Timers
#define TIM_BANK_ARRAY_LEN 14
#define TIM_PIN_ARRAY_LEN 60
extern TIM_TypeDef *mcu_tim_banks[TIM_BANK_ARRAY_LEN];
extern const mcu_tim_pin_obj_t mcu_tim_pin_list[TIM_PIN_ARRAY_LEN];
