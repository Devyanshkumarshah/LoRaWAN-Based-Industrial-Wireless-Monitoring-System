/* lora_sx1278.h — SX1278 driver header */
#ifndef LORA_SX1278_H
#define LORA_SX1278_H

#include "stm32l4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* SX1278 Register Map */
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FR_MSB               0x06
#define REG_FR_MID               0x07
#define REG_FR_LSB               0x08
#define REG_PA_CONFIG            0x09
#define REG_LNA                  0x0C
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE_ADDR    0x0E
#define REG_FIFO_RX_BASE_ADDR    0x0F
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS_MASK       0x11
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1A
#define REG_MODEM_CONFIG_1       0x1D
#define REG_MODEM_CONFIG_2       0x1E
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_SYNC_WORD            0x39
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4D

/* Modes */
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05
#define MODE_RX_SINGLE           0x06

/* IRQ Flags */
#define IRQ_RX_DONE_MASK         0x40
#define IRQ_CRC_ERROR_MASK       0x20
#define IRQ_TX_DONE_MASK         0x08

/* PA */
#define PA_BOOST                 0x80

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef      *nss_port;
    uint16_t           nss_pin;
    GPIO_TypeDef      *rst_port;
    uint16_t           rst_pin;
    GPIO_TypeDef      *dio0_port;
    uint16_t           dio0_pin;
} LoRa_HandleTypeDef;

/* Public API */
bool  LoRa_Init(LoRa_HandleTypeDef *lora, uint32_t frequency_hz,
                uint8_t sf, uint32_t bw_hz, uint8_t cr,
                uint8_t tx_power, uint8_t sync_word);
int   LoRa_Receive(LoRa_HandleTypeDef *lora, uint8_t *buf, uint8_t max_len,
                   int8_t *rssi_out, int8_t *snr_out);
bool  LoRa_PacketAvailable(LoRa_HandleTypeDef *lora);
void  LoRa_StartReceive(LoRa_HandleTypeDef *lora);

#endif /* LORA_SX1278_H */
