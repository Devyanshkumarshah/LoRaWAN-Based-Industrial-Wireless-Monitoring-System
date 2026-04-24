/**
 * ============================================================
 *  sx1278.h  —  SX1278 LoRa radio driver (SPI, STM32 HAL)
 *  Target: STM32L476RG  (STM32CubeIDE)
 * ============================================================
 *
 *  SPI wiring (SPI1 default):
 *    SX1278 SCK   → PA5
 *    SX1278 MISO  → PA6
 *    SX1278 MOSI  → PA7
 *    SX1278 NSS   → PA4  (CS — GPIO output)
 *    SX1278 RESET → PB0  (GPIO output)
 *    SX1278 DIO0  → PB1  (GPIO EXTI input — RxDone/TxDone)
 * ============================================================
 */
#ifndef SX1278_H
#define SX1278_H

#include "stm32l4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ── RF parameters ───────────────────────────────────────── */
#define SX1278_FREQ_MHZ       433.0f   /* Must match sensor node          */
#define SX1278_BW_125K        0x70     /* RegModemConfig1 bits [7:4]      */
#define SX1278_SF10           10       /* Spreading factor                */
#define SX1278_CR_4_5         1        /* Coding rate 4/5                 */
#define SX1278_PREAMBLE       8
#define SX1278_TX_POWER_DBM   17
#define SX1278_SYNC_WORD      0x12     /* Private network                 */
#define SX1278_MAX_PKT_LEN    255

/* ── SX1278 Register map (subset) ───────────────────────── */
#define REG_FIFO                0x00
#define REG_OP_MODE             0x01
#define REG_FRF_MSB             0x06
#define REG_FRF_MID             0x07
#define REG_FRF_LSB             0x08
#define REG_PA_CONFIG           0x09
#define REG_LNA                 0x0C
#define REG_FIFO_ADDR_PTR       0x0D
#define REG_FIFO_TX_BASE_ADDR   0x0E
#define REG_FIFO_RX_BASE_ADDR   0x0F
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS           0x12
#define REG_RX_NB_BYTES         0x13
#define REG_MODEM_CONFIG_1      0x1D
#define REG_MODEM_CONFIG_2      0x1E
#define REG_PREAMBLE_MSB        0x20
#define REG_PREAMBLE_LSB        0x21
#define REG_PAYLOAD_LENGTH      0x22
#define REG_MODEM_CONFIG_3      0x26
#define REG_PKT_SNR_VALUE       0x19
#define REG_PKT_RSSI_VALUE      0x1A
#define REG_RSSI_VALUE          0x1B
#define REG_SYNC_WORD           0x39
#define REG_DIO_MAPPING_1       0x40
#define REG_VERSION             0x42
#define REG_PA_DAC              0x4D

/* ── Operating modes ────────────────────────────────────── */
#define MODE_LONG_RANGE_MODE    0x80
#define MODE_SLEEP              0x00
#define MODE_STDBY              0x01
#define MODE_TX                 0x03
#define MODE_RX_CONTINUOUS      0x05
#define MODE_RX_SINGLE          0x06

/* ── IRQ flags ───────────────────────────────────────────── */
#define IRQ_TX_DONE_MASK         0x08
#define IRQ_RX_DONE_MASK         0x40
#define IRQ_PAYLOAD_CRC_ERR_MASK 0x20

/* ── Init config struct ──────────────────────────────────── */
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef      *cs_port;
    uint16_t           cs_pin;
    GPIO_TypeDef      *rst_port;
    uint16_t           rst_pin;
} sx1278_config_t;

/* ── RX result ───────────────────────────────────────────── */
typedef struct {
    uint8_t  buf[SX1278_MAX_PKT_LEN];
    uint8_t  len;
    int16_t  rssi;
    int8_t   snr;
} sx1278_rx_t;

/* ── Public API ──────────────────────────────────────────── */
bool  sx1278_init(const sx1278_config_t *cfg);
void  sx1278_set_rx_continuous(void);
bool  sx1278_rx_available(void);          /* poll DIO0 via IRQ flag        */
bool  sx1278_read_packet(sx1278_rx_t *rx);
void  sx1278_clear_irq(void);

/* Internal — used by gateway layer */
uint8_t sx1278_read_reg(uint8_t addr);
void    sx1278_write_reg(uint8_t addr, uint8_t value);

#endif /* SX1278_H */
