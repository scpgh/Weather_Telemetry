#include <esp_now.h>
#include <WiFi.h>


  float temperature;
  float humidity;
} struct_message;

typedef struct struct_ack {
  int  status;          
  char message[64];     
} struct_ack;

struct_message incomingReadings;
struct_ack     ackMessage;


char customReply[64] = "OK";


uint8_t senderMac[6];
volatile bool packetPending = false;


void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  memcpy(senderMac, mac, 6);
  packetPending = true;

  Serial.println("\n─────────────────────────────────────");
  Serial.printf("[RX] Temp: %.2f C | Humidity: %.2f %%\n",
                incomingReadings.temperature, incomingReadings.humidity);
  Serial.println("[?]  Type a reply and press Enter (or wait 2s for default):");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  Serial.println("=== Gateway Node Booting ===");
  Serial.print("[INFO] MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERR] ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  Serial.println("[OK] ESP-NOW ready — waiting for Sensor Node packets...");
  Serial.println("[TIP] Type a custom message in Serial Monitor to reply to the sensor.\n");
}

void loop() {

  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0) {
      input.substring(0, 63).toCharArray(customReply, 64);
      Serial.printf("[SET] Custom reply set: \"%s\"\n", customReply);
    }
  }


  if (packetPending) {
    packetPending = false;

    ackMessage.status = 200;
    strncpy(ackMessage.message, customReply, 64);

    esp_now_send(senderMac, (uint8_t *)&ackMessage, sizeof(ackMessage));
    Serial.printf("[TX] ACK sent → 200 OK | Reply: \"%s\"\n", ackMessage.message);
  }
}
