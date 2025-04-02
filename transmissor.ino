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

#define VRX_PIN  34 // ESP32 pin GPIO34 (ADC6) connected to VRX pin
#define VRY_PIN  35 // ESP32 pin GPIO35 (ADC7) connected to VRY pin
#define SW_PIN   15 // ESP32 pin GPIO15 connected to SW pin

#define JOYSTICK_MIN 0
#define JOYSTICK_MAX 4095
#define JOYSTICK_CENTER 2048  // Valor central para joysticks de 12 bits (4096/2)

#define JOYSTICK_DEADZONE 200  // Margem para ignorar pequenas variações

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
    // Leitura do sensor de distância
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    long duration = pulseIn(ECHO, HIGH);
    float distance = (duration * 0.0343) / 2;

    // Leitura do joystick com média móvel para suavizar
    static int xReadings[5] = {0}, yReadings[5] = {0};
    static byte index = 0;
    
    xReadings[index] = analogRead(VRX_PIN);
    yReadings[index] = analogRead(VRY_PIN);
    index = (index + 1) % 5;
    
    int rawX = 0, rawY = 0;
    for (byte i = 0; i < 5; i++) {
        rawX += xReadings[i];
        rawY += yReadings[i];
    }
    rawX /= 5;
    rawY /= 5;

    // Leitura do botão
    bool buttonPressed = digitalRead(SW_PIN) == LOW;

    // Normalizar para valores entre -100 e 100 com zona morta
    int joystickX = 0;
    int joystickY = 0;
    
    // Processar eixo X
    if (rawX > (JOYSTICK_CENTER + JOYSTICK_DEADZONE)) {
        joystickX = map(rawX, JOYSTICK_CENTER + JOYSTICK_DEADZONE, JOYSTICK_MAX, 0, 100);
    } 
    else if (rawX < (JOYSTICK_CENTER - JOYSTICK_DEADZONE)) {
        joystickX = map(rawX, JOYSTICK_MIN, JOYSTICK_CENTER - JOYSTICK_DEADZONE, -100, 0);
    }
    
    // Processar eixo Y (invertido)
    if (rawY > (JOYSTICK_CENTER + JOYSTICK_DEADZONE)) {
        joystickY = map(rawY, JOYSTICK_CENTER + JOYSTICK_DEADZONE, JOYSTICK_MAX, 0, -100);
    } 
    else if (rawY < (JOYSTICK_CENTER - JOYSTICK_DEADZONE)) {
        joystickY = map(rawY, JOYSTICK_MIN, JOYSTICK_CENTER - JOYSTICK_DEADZONE, 0, 100);
    }

    // Garantir que os valores estejam dentro do intervalo -100 a 100
    joystickX = constrain(joystickX, -100, 100);
    joystickY = constrain(joystickY, -100, 100);

    // Atualizar dados a enviar
    myData.distance = distance;
    myData.joystickX = joystickX;
    myData.joystickY = joystickY;
    myData.buttonPressed = buttonPressed;

    // Enviar dados via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    // Exibir informações no terminal
    Serial.print("Distância enviada: ");
    Serial.print(myData.distance);
    Serial.println(" cm");
    Serial.print("Joystick X: ");
    Serial.print(myData.joystickX);
    Serial.print(" Y: ");
    Serial.print(myData.joystickY);
    Serial.print(" Botão: ");
    Serial.println(myData.buttonPressed ? "Pressionado" : "Solto");

    // Exibir informações no display OLED
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

    delay(600); 
}