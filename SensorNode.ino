// ============================================================
// SENSOR NODE: ESP32 + SX1278 LoRa
// Arduino IDE | LoRa library by Sandeep Mistry
// Wiring: SX1278 -> ESP32
//   NSS  -> GPIO5   MOSI -> GPIO23
//   SCK  -> GPIO18  MISO -> GPIO19
//   RST  -> GPIO14  DIO0 -> GPIO26
// ============================================================

#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>          // Optional: DHT22 sensor
#include <Arduino.h>

// ---------- LoRa Pin Config ----------
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  26

// ---------- LoRa RF Config ----------
#define LORA_FREQ        433E6   // 433 MHz (match SX1278 band)
#define LORA_SF          7       // Spreading Factor 7–12
#define LORA_BW          125E3   // Bandwidth 125 kHz
#define LORA_CR          5       // Coding Rate 4/5
#define LORA_TX_POWER    17      // dBm (max 20)
#define LORA_SYNC_WORD   0x34    // Private network sync word

// ---------- Sensor Config ----------
#define DHT_PIN    4
#define DHT_TYPE   DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// ---------- Node Identity ----------
#define NODE_ID    0x01          // Unique ID per sensor node

// ---------- Packet Structure ----------
// [NODE_ID(1)] [SEQ(2)] [TEMP_x10(2)] [HUM_x10(2)] [VBAT_mV(2)] [CRC8(1)]
#define PKT_LEN    10

uint16_t seqNum = 0;

// ---- CRC-8 (Dallas/Maxim) ----
uint8_t crc8(uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if ((crc ^ byte) & 0x80) crc = (crc << 1) ^ 0x31;
            else                      crc = (crc << 1);
            byte <<= 1;
        }
    }
    return crc;
}

// ---- Read battery voltage (ADC pin 34, voltage divider 100k/100k) ----
uint16_t readBatteryMv() {
    uint32_t raw = analogRead(34);
    // ESP32 ADC: 0–4095 → 0–3.3V, with 2:1 divider → 0–6.6V
    return (uint16_t)((raw * 3300UL * 2) / 4095);
}

void buildPacket(uint8_t *buf, float temp, float hum) {
    int16_t temp10 = (int16_t)(temp * 10);
    uint16_t hum10 = (uint16_t)(hum  * 10);
    uint16_t vbat  = readBatteryMv();

    buf[0] = NODE_ID;
    buf[1] = (seqNum >> 8) & 0xFF;
    buf[2] =  seqNum       & 0xFF;
    buf[3] = (temp10 >> 8) & 0xFF;
    buf[4] =  temp10       & 0xFF;
    buf[5] = (hum10  >> 8) & 0xFF;
    buf[6] =  hum10        & 0xFF;
    buf[7] = (vbat   >> 8) & 0xFF;
    buf[8] =  vbat         & 0xFF;
    buf[9] = crc8(buf, 9);           // CRC over bytes 0–8
}

void setup() {
    Serial.begin(115200);
    dht.begin();

    // Init LoRa
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(LORA_FREQ)) {
        Serial.println("[ERROR] LoRa init failed");
        while (true);
    }
    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setCodingRate4(LORA_CR);
    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.setSyncWord(LORA_SYNC_WORD);
    LoRa.enableCrc();

    Serial.println("[OK] Sensor Node Ready");
}

void loop() {
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
        Serial.println("[WARN] DHT read failed, using defaults");
        temp = 0.0f;
        hum  = 0.0f;
    }

    uint8_t pkt[PKT_LEN];
    buildPacket(pkt, temp, hum);

    // Transmit
    LoRa.beginPacket();
    LoRa.write(pkt, PKT_LEN);
    int ret = LoRa.endPacket();   // blocking send

    if (ret) {
        Serial.printf("[TX] SEQ=%u  T=%.1f°C  H=%.1f%%  VBAT=%umV  CRC=0x%02X\n",
                      seqNum, temp, hum, (pkt[7]<<8)|pkt[8], pkt[9]);
        seqNum++;
    } else {
        Serial.println("[ERROR] TX failed");
    }

    delay(10000);   // Transmit every 10 seconds
}
