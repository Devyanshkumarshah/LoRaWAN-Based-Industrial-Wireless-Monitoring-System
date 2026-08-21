#include<SPI.h>
#include<LoRa.h>
#include<DHT.h>
//pin configuration for LoRa Module
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  26
//pin configuration for DHT 
#define DHT_PIN    4
#define DHT_TYPE   DHT22
#define LORA_FREQ   433E6 
#define NODE_ID   0x01 
#define packetLen 10
uint16_t seqNum = 0;
DHT dht(DHT_PIN, DHT_TYPE);

//CRC function
uint8_t crc8(uint8_t *data, uint8_t len)
{
  uint8_t crc = 0x00;
  
  for(int i = 0; i < len; i++)
  {
    uint8_t byte = data[i];
    for(int b = 0; b < 8; b++)
    {
      if ((crc ^ byte) & 0x80)
      {
        crc = (crc << 1) ^ 0x31;
      }
      else{ 
        crc = (crc << 1);
      }
      byte <<= 1;
    }
  }
  return crc;
}

void buildPacket(uint8_t *buf, float temp, float hum)
{
    int16_t temp10 = (int16_t)(temp * 10);
    uint16_t hum10 = (uint16_t)(hum  * 10);
    uint16_t vbat = 0;

    buf[0] = NODE_ID;
    buf[1] = (seqNum >> 8) & 0xFF;
    buf[2] =  seqNum & 0xFF;
    buf[3] = (temp10 >> 8) & 0xFF;
    buf[4] =  temp10 & 0xFF;
    buf[5] = (hum10  >> 8) & 0xFF;
    buf[6] =  hum10 & 0xFF;
    buf[7] = 0x00;
    buf[8] = 0x00;
    buf[9] = crc8(buf, 9); 
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
    Serial.println("Sensor Node is Ready");
}
void loop() {
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
        Serial.println("[WARN] DHT read failed, using defaults");
        temp = 0.0f;
        hum  = 0.0f;
    }

    uint8_t pkt[packetLen];

    buildPacket(pkt, temp, hum);

    // Transmit
    LoRa.beginPacket();
    LoRa.write(pkt, packetLen);
    int ret = LoRa.endPacket();   // blocking send

    // if (ret) {
    //     Serial.printf("[TX] SEQ=%u  T=%.1f°C  H=%.1f%%  VBAT=%umV  CRC=0x%02X\n",
    //                   seqNum, temp, hum, (pkt[7]<<8)|pkt[8], pkt[9]);
    //     seqNum++;
    // } else {
    //     Serial.println("[ERROR] TX failed");
    // }
    if(ret)
    {
    seqNum++;
    }
    delay(10000);   // Transmit every 10 seconds
}
