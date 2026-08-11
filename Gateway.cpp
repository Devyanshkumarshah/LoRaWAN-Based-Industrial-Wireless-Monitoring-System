#include<SPI.h>
#include<LoRa.h>
#include<WiFI.h>
#include<PubSubClient.h>

#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIDO 26

//  [node ID (1 byte)] [SEQ (2 byte)] [Temp_x10(2 byte)] [Humidity data (2)][vbat_mV(2 bit)] [CRC891 byte]  
#define Packet_Len 10
#define LORA_Fq 433E6
#define LORA_SF 7
#define LORA_BW 125E3
#define LORA_CR 5
#define LORA_SYNC 0X34


char* MQTT_Server = "broker.hivemq.com";
int MQRR_Port = 1883;
char* MQTT_Topic = "tatasteeldemo/wsn/gateway01/data";
char* WiFi_SSID = "Devyansh"
char* WiFi_Pass = "ESP_test32"


WiFiClient espClient;
PubSubClient client(espClient);

uint8_t crcCheck(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0x00;
    for(int i = 0; i < len i++)
    {
        uint8_t byte = data[i];
        for(int b = 0; b < 8; b++)
        {
            if((crc ^ byte) && 0x80) == (crc << 1) ^0x31);

            else crc = (crc << 1);
            byte <<= 1;
        }
    }
    return crc;
}

void connectWiFi() 
{
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  Serial.println();
  Serial.print("WiFi connected, IP = ");
  Serial.println(WiFi.localIP());
}

void connectMQTT()
{
    while(!client.connected())      
    {
        String clientId = "esp-gateway-01";
        bool connStatus;

        if(strlen(MQTT_USER == 0))
        {
            connStatus = client.connect(clientId.c_str());
        }
        else 
        {
            connStatus = client.connect(clientId.c_str(), MQTT_USER,MQTT_PASS); 
        }
        if (connStatus) 
        {
            Serial.println("connected");
        } 

        else
        {
            delay(2000);
        }
    }
}

void setup()
{
      serial.begin(115200);

      //LoRa Init
      LoRa.setPins(LORA_SS, LORA_RST, LORA_DIDO);
      if (!LoRa.begin(LORA_FREQ)) 
      {
           Serial.println("LoRa init failed, check wiring");
           while (1);
      }
      LoRa.setSpreadingFactor(LORA_SF);
      LoRa.setSignalBandwidth(LORA_BW);
      LoRa.setCodingRate4(LORA_CR);
      LoRa.setSyncWord(LORA_SYNC_WORD);
      LoRa.enableCrc();
      Serial.println("LoRa Gateway ready, waiting for packets...");

     
      connectWiFi();
      client.setServer(MQTT_SERVER, MQTT_PORT);

}

void loop() 
{
    if (!client.connected()) 
    {
        connectMQTT();
    }

    int packetSize = LoRa.parsePacket();
    if (packetSize) 
    {
        if (packetSize != Packet_Len) 
        {
            Serial.println("Got a packet but wrong size, ignoring it");
            while (LoRa.available()) LoRa.read();
            return;
        }
    }

    uint8_t buf[Packet_Len];
    int i = 0;
    while (LoRa.available() && i < Packet_Len)
      {
          buf[i] = LoRa.read();
          i++;
      }
  //crc check
    uint8_t crcCalc = crcCheck(buf, 9);
    if (crcCalc != buf[9]) 
    {
        Serial.println("CRC mismatch, dropping packet");
        return;
    }

  // data decoding 
    uint8_t nodeId = buf[0];
    uint16_t seq = (buf[1] << 8) | buf[2];
    int16_t temp10 = (buf[3] << 8) | buf[4];     
    uint16_t hum10 = (buf[5] << 8) | buf[6];
    uint16_t vbat = (buf[7] << 8) | buf[8];

    float temperature = temp10 / 10.0;
    float humidity    = hum10 / 10.0;
    
    
    char payload[200];
    snprintf(payload, sizeof(payload), "{\"node_id\":%d,\"seq\":%u,\"temperature\":%.1f,\"humidity\":%.1f,\"vbat_mv\":%u,\"rssi\":%d,\"snr\":%.1f}", nodeId, seq, temperature, humidity, vbat, rssi, snr);

    if(client.publish(MQTT_Topic, payload))
    {
        Serial.println("Published to MQTT broker successfully");
    }
    else 
    {
        Serial.println("MQTT publish failed");
    }
}























































































