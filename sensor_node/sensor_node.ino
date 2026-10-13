#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>


#define DHTPIN  15        
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

uint8_t gatewayAddress[] = {0x24, 0x0A, 0xC4, 0x00, 0x01, 0x10};

typedef struct struct_message {
  float temperature;
  float humidity;
} struct_message;

typedef struct struct_ack {
  int  status;          
  char message[64];     
} struct_ack;

struct_message  outgoingReadings;
struct_ack      incomingAck;
esp_now_peer_info_t peerInfo;


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("[TX] Transmission Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingAck, incomingData, sizeof(incomingAck));
  if (incomingAck.status == 200) {
    Serial.println("[RX] Gateway Confirmed Receipt (200 OK)");
    Serial.printf("[RX] Gateway Reply: \"%s\"\n", incomingAck.message);
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  WiFi.mode(WIFI_STA);

  Serial.println("=== Sensor Node Booting ===");

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERR] ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb((esp_now_send_cb_t)OnDataSent);
  esp_now_register_recv_cb((esp_now_recv_cb_t)OnDataRecv);

  memcpy(peerInfo.peer_addr, gatewayAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ERR] Failed to register Gateway peer");
    return;
  }

  Serial.println("[OK] ESP-NOW ready — transmitting every 2s");
}

void loop() {
  outgoingReadings.temperature = dht.readTemperature();
  outgoingReadings.humidity    = dht.readHumidity();

  if (isnan(outgoingReadings.temperature) || isnan(outgoingReadings.humidity)) {
    Serial.println("[ERR] DHT22 read failed — check wiring on GPIO 15");
    delay(2000);
    return;
  }

  Serial.printf("[TX] Temp: %.2f C | Humidity: %.2f %%\n",
                outgoingReadings.temperature, outgoingReadings.humidity);

  esp_now_send(gatewayAddress, (uint8_t *)&outgoingReadings, sizeof(outgoingReadings));

  delay(2000);
}
