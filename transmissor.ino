#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define TRIG 5
#define ECHO 18

#define VRX_PIN  22 // ESP32 pin GPIO22 connected to VRX pin
#define VRY_PIN  17 // ESP32 pin GPIO17 connected to VRY pin
#define SW_PIN   15 // ESP32 pin GPIO15 connected to SW pin

// MAC Address do ESP32 receptor
uint8_t broadcastAddress[] = {0x08, 0xF9, 0xE0, 0xCF, 0xE8, 0xF4};

// Estrutura para enviar os dados
typedef struct struct_message {
  float distance;
  int joystickX;
  int joystickY;
  bool buttonPressed;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Status do envio: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sucesso" : "Falha");
}

void setup() {
    Serial.begin(115200);
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
    pinMode(VRX_PIN, INPUT);
    pinMode(VRY_PIN, INPUT);
    pinMode(SW_PIN, INPUT_PULLUP);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("Falha ao iniciar o display OLED!");
        for (;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Erro ao inicializar ESP-NOW");
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Falha ao adicionar o receptor");
        return;
    }
}

void loop() {
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    long duration = pulseIn(ECHO, HIGH);
    float distance = (duration * 0.0343) / 2;

    int joystickX = analogRead(VRX_PIN);
    int joystickY = analogRead(VRY_PIN);
    bool buttonPressed = digitalRead(SW_PIN) == LOW;

    myData.distance = distance;
    myData.joystickX = joystickX;
    myData.joystickY = joystickY;
    myData.buttonPressed = buttonPressed;

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    Serial.print("Distância enviada: ");
    Serial.print(myData.distance);
    Serial.println(" cm");
    Serial.print("Joystick X: ");
    Serial.print(myData.joystickX);
    Serial.print(" Y: ");
    Serial.print(myData.joystickY);
    Serial.print(" Botão: ");
    Serial.println(myData.buttonPressed ? "Pressionado" : "Solto");

    display.clearDisplay();
    display.setCursor(10, 10);
    display.print("Dist: ");
    display.print(myData.distance, 1);
    display.print(" cm");
    display.setCursor(10, 25);
    display.print("X: ");
    display.print(myData.joystickX);
    display.setCursor(10, 35);
    display.print("Y: ");
    display.print(myData.joystickY);
    display.setCursor(10, 45);
    display.print("Botao: ");
    display.print(myData.buttonPressed ? "Press" : "Solto");
    display.display();

    delay(500);
}