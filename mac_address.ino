#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inicializando a interface Wi-Fi
  WiFi.mode(WIFI_MODE_STA); // Configura o modo Wi-Fi como Station (STA)
  delay(100);

  // Obtendo e imprimindo o MAC Address
  String macAddress = WiFi.macAddress();
  Serial.println("MAC Address do ESP32:");
  Serial.println(macAddress);
}

void loop() {
  // Não há necessidade de loop
}
