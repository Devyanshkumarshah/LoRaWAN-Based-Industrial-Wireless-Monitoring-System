/**
 * ============================================================
 *  lora_gateway.h
 *  LoRaWAN RX → Packet Parser → MQTT Framer
 *  Target: STM32L476RG  (STM32CubeIDE)
 * ============================================================
 */
#ifndef LORA_GATEWAY_H
#define LORA_GATEWAY_H

#include <stdint.h>
#include <stdbool.h>

/* ── Packet constants (must match sensor node) ─────────── */
#define PKT_MAGIC      0xA5
#define PKT_LEN        12
#define FLAG_VIB       (1 << 0)
#define FLAG_ALERT     (1 << 1)

/* ── Parsed packet structure ─────────────────────────────  */
typedef struct {
    uint8_t  node_id;
    uint16_t seq;
    uint8_t  flags;
    uint32_t vib_count;
    int16_t  rssi;       /* RSSI filled by gateway after RX */
    bool     valid;      /* CRC passed */
} lora_packet_t;

/* ── Return codes ────────────────────────────────────────── */
typedef enum {
    GW_OK            = 0,
    GW_ERR_CRC       = -1,
    GW_ERR_MAGIC     = -2,
    GW_ERR_LEN       = -3,
    GW_ERR_DUPLICATE = -4,
} gw_err_t;

/* ── Public API ──────────────────────────────────────────── */
void      lora_gateway_init(void);
void      lora_gateway_process(void);          /* call from main loop        */
gw_err_t  lora_parse_packet(const uint8_t *buf, uint8_t len,
                             int16_t rssi, lora_packet_t *out);

#endif /* LORA_GATEWAY_H */
