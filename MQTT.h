/**
 * ============================================================
 *  mqtt_client.h  —  Minimal MQTT 3.1.1 client over raw TCP
 *  Target: STM32L476RG + W5500 Ethernet (lwIP raw API or
 *          WIZnet ioLibrary — choose in mqtt_client.c)
 * ============================================================
 */
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

/* ── Broker config ───────────────────────────────────────── */
#define MQTT_BROKER_IP    "192.168.1.100"  /* Change to your broker IP   */
#define MQTT_BROKER_PORT  1883
#define MQTT_CLIENT_ID    "STM32_GW_01"
#define MQTT_KEEPALIVE_S  60              /* seconds                       */
#define MQTT_QOS          1               /* At-least-once delivery        */
#define MQTT_RETAIN       0

/* Topic template: sensors/node<id>/vib */
#define MQTT_TOPIC_PREFIX "sensors/node"
#define MQTT_TOPIC_SUFFIX "/vib"

/* ── Packet types ────────────────────────────────────────── */
#define MQTT_CONNECT     0x10
#define MQTT_CONNACK     0x20
#define MQTT_PUBLISH     0x30
#define MQTT_PUBACK      0x40
#define MQTT_PINGREQ     0xC0
#define MQTT_PINGRESP    0xD0
#define MQTT_DISCONNECT  0xE0

/* ── State machine ───────────────────────────────────────── */
typedef enum {
    MQTT_STATE_DISCONNECTED = 0,
    MQTT_STATE_CONNECTING,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_ERROR,
} mqtt_state_t;

/* ── Publish payload ─────────────────────────────────────── */
typedef struct {
    uint8_t  node_id;
    uint16_t seq;
    uint8_t  flags;
    uint32_t vib_count;
    int16_t  rssi;
    uint32_t timestamp_ms;  /* HAL_GetTick() at RX time               */
} mqtt_payload_t;

/* ── Public API ──────────────────────────────────────────── */
bool        mqtt_init(void);
mqtt_state_t mqtt_get_state(void);
bool        mqtt_connect(void);
bool        mqtt_publish(const mqtt_payload_t *payload);
void        mqtt_keepalive(void);          /* call periodically            */
void        mqtt_process(void);            /* pump state machine           */
void        mqtt_disconnect(void);

#endif /* MQTT_CLIENT_H */
