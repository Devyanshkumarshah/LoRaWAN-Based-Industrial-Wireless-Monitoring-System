/* lora_sx1278.c — SX1278 HAL driver */
#include "lora_sx1278.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  SPI helpers                                                         */
/* ------------------------------------------------------------------ */
static void spi_select(LoRa_HandleTypeDef *lora) {
    HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_RESET);
}
static void spi_deselect(LoRa_HandleTypeDef *lora) {
    HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);
}

static uint8_t read_reg(LoRa_HandleTypeDef *lora, uint8_t addr) {
    uint8_t tx[2] = { addr & 0x7F, 0x00 };
    uint8_t rx[2] = { 0 };
    spi_select(lora);
    HAL_SPI_TransmitReceive(lora->hspi, tx, rx, 2, HAL_MAX_DELAY);
    spi_deselect(lora);
    return rx[1];
}

static void write_reg(LoRa_HandleTypeDef *lora, uint8_t addr, uint8_t val) {
    uint8_t tx[2] = { addr | 0x80, val };
    spi_select(lora);
    HAL_SPI_Transmit(lora->hspi, tx, 2, HAL_MAX_DELAY);
    spi_deselect(lora);
}

static void read_fifo(LoRa_HandleTypeDef *lora, uint8_t *buf, uint8_t len) {
    uint8_t addr_byte = REG_FIFO & 0x7F;
    spi_select(lora);
    HAL_SPI_Transmit(lora->hspi, &addr_byte, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(lora->hspi, buf, len, HAL_MAX_DELAY);
    spi_deselect(lora);
}

/* ------------------------------------------------------------------ */
/*  Init                                                                */
/* ------------------------------------------------------------------ */
bool LoRa_Init(LoRa_HandleTypeDef *lora, uint32_t freq_hz,
               uint8_t sf, uint32_t bw_hz, uint8_t cr,
               uint8_t tx_power, uint8_t sync_word)
{
    /* Hardware reset */
    HAL_GPIO_WritePin(lora->rst_port, lora->rst_pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(lora->rst_port, lora->rst_pin, GPIO_PIN_SET);
    HAL_Delay(10);

    /* Check version register */
    uint8_t ver = read_reg(lora, REG_VERSION);
    if (ver != 0x12) return false;   // SX1276/77/78/79 = 0x12

    /* Sleep → LoRa mode */
    write_reg(lora, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
    HAL_Delay(10);

    /* Set frequency */
    uint64_t frf = ((uint64_t)freq_hz << 19) / 32000000ULL;
    write_reg(lora, REG_FR_MSB, (uint8_t)(frf >> 16));
    write_reg(lora, REG_FR_MID, (uint8_t)(frf >>  8));
    write_reg(lora, REG_FR_LSB, (uint8_t)(frf >>  0));

    /* FIFO base addresses */
    write_reg(lora, REG_FIFO_TX_BASE_ADDR, 0x00);
    write_reg(lora, REG_FIFO_RX_BASE_ADDR, 0x00);

    /* LNA boost */
    write_reg(lora, REG_LNA, read_reg(lora, REG_LNA) | 0x03);

    /* Modem Config 1: BW | CR | implicit header off */
    uint8_t bw_bits;
    if      (bw_hz <=   7800) bw_bits = 0;
    else if (bw_hz <=  10400) bw_bits = 1;
    else if (bw_hz <=  15600) bw_bits = 2;
    else if (bw_hz <=  20800) bw_bits = 3;
    else if (bw_hz <=  31250) bw_bits = 4;
    else if (bw_hz <=  41700) bw_bits = 5;
    else if (bw_hz <=  62500) bw_bits = 6;
    else if (bw_hz <= 125000) bw_bits = 7;
    else if (bw_hz <= 250000) bw_bits = 8;
    else                      bw_bits = 9;

    uint8_t cr_bits = (cr - 4) & 0x07;   // 4/5→1 … 4/8→4
    write_reg(lora, REG_MODEM_CONFIG_1, (bw_bits << 4) | (cr_bits << 1) | 0x00);

    /* Modem Config 2: SF | CRC on */
    write_reg(lora, REG_MODEM_CONFIG_2, (sf << 4) | 0x04);

    /* Modem Config 3: AGC auto on */
    write_reg(lora, REG_MODEM_CONFIG_3, 0x04);

    /* TX power (PA_BOOST pin) */
    if (tx_power > 17) {
        tx_power = 20;
        write_reg(lora, REG_PA_DAC, 0x87);
        write_reg(lora, REG_PA_CONFIG, PA_BOOST | 0x0F);
    } else {
        write_reg(lora, REG_PA_DAC, 0x84);
        write_reg(lora, REG_PA_CONFIG, PA_BOOST | (uint8_t)(tx_power - 2));
    }

    /* Sync word */
    write_reg(lora, REG_SYNC_WORD, sync_word);

    /* DIO0 → RxDone */
    write_reg(lora, REG_DIO_MAPPING_1, 0x00);

    /* Preamble length = 8 symbols */
    write_reg(lora, REG_PREAMBLE_MSB, 0x00);
    write_reg(lora, REG_PREAMBLE_LSB, 0x08);

    /* Standby */
    write_reg(lora, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
    return true;
}

/* ------------------------------------------------------------------ */
/*  Start continuous receive                                            */
/* ------------------------------------------------------------------ */
void LoRa_StartReceive(LoRa_HandleTypeDef *lora) {
    write_reg(lora, REG_FIFO_ADDR_PTR, read_reg(lora, REG_FIFO_RX_BASE_ADDR));
    write_reg(lora, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
}

/* ------------------------------------------------------------------ */
/*  Poll DIO0 (RxDone)                                                  */
/* ------------------------------------------------------------------ */
bool LoRa_PacketAvailable(LoRa_HandleTypeDef *lora) {
    return (read_reg(lora, REG_IRQ_FLAGS) & IRQ_RX_DONE_MASK) != 0;
}

/* ------------------------------------------------------------------ */
/*  Read received packet                                                */
/* ------------------------------------------------------------------ */
int LoRa_Receive(LoRa_HandleTypeDef *lora, uint8_t *buf,
                 uint8_t max_len, int8_t *rssi_out, int8_t *snr_out)
{
    uint8_t irq = read_reg(lora, REG_IRQ_FLAGS);
    /* Clear IRQ flags */
    write_reg(lora, REG_IRQ_FLAGS, irq);

    if (!(irq & IRQ_RX_DONE_MASK))  return -1;  // No packet
    if   (irq & IRQ_CRC_ERROR_MASK) return -2;  // CRC error

    uint8_t len = read_reg(lora, REG_RX_NB_BYTES);
    if (len > max_len) len = max_len;

    /* Point FIFO pointer to start of received packet */
    write_reg(lora, REG_FIFO_ADDR_PTR,
              read_reg(lora, REG_FIFO_RX_CURRENT_ADDR));
    read_fifo(lora, buf, len);

    /* RSSI / SNR */
    if (rssi_out) *rssi_out = (int8_t)read_reg(lora, REG_PKT_RSSI_VALUE) - 164;
    if (snr_out)  *snr_out  = (int8_t)read_reg(lora, REG_PKT_SNR_VALUE)  / 4;

    return (int)len;
}
