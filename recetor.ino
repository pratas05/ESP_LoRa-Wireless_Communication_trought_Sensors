#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

#define BLUE_LED 13
#define BUZZER_PIN 4  // Pino do buzzer

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Estrutura para receber os dados (incluindo contador de pacotes)
typedef struct struct_message {
    float distance;
    bool buttonPressed;
    bool accessGranted;
    uint8_t packetCounter;
} struct_message;

// Criar estrutura para armazenar os dados recebidos
struct_message myData;
uint8_t lastPacketCounter = 0;
unsigned long lastReceivedTime = 0;
const long timeoutInterval = 2000; // 3 segundos sem comunicação

// Callback atualizado para nova versão do ESP-NOW
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    memcpy(&myData, incomingData, sizeof(myData));

    // Verifica se o pacote é novo (evita repetições)
    if (myData.packetCounter != lastPacketCounter) {
        lastPacketCounter = myData.packetCounter;
        lastReceivedTime = millis(); // Atualiza o tempo da última recepção
        
        // Exibir no monitor serial
        Serial.print("Distância recebida: ");
        Serial.print(myData.distance);
        Serial.println(" cm");

        Serial.print("Acesso: ");
        Serial.println(myData.accessGranted ? "LIBERADO" : "NEGADO");

        // Atualiza o display OLED
        display.clearDisplay();
        display.setCursor(10, 10);
        display.print("Dist: ");
        display.print(myData.distance, 1);
        display.print(" cm");

        display.setCursor(10, 25);
        display.print("Acesso: ");
        display.print(myData.accessGranted ? "OK" : "NÃO");

        display.display();

        // Verifica se a distância é menor que 10cm e aciona o buzzer
        if (myData.distance < 15.0) {
            digitalWrite(BUZZER_PIN, HIGH);  // Liga o buzzer
            digitalWrite(BLUE_LED , HIGH);
            delay(500);
            digitalWrite(BLUE_LED , LOW);
            digitalWrite(BUZZER_PIN, LOW);   // Desliga o buzzer
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Inicializa o display
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("Falha ao iniciar o display OLED!");
        for (;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Configurar ESP32 como Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Inicializar ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Erro ao inicializar ESP-NOW");
        return;
    }

    // Registrar callback para receber dados
    esp_now_register_recv_cb(OnDataRecv);

    // Configurar os pinos de saída
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(BLUE_LED, LOW);

    lastReceivedTime = millis(); // Inicializa o tempo da última recepção
}

void loop() {
    // Verifica se houve timeout na recepção de dados
    if (millis() - lastReceivedTime > timeoutInterval) {
        Serial.println("Conexão perdida! Nenhum dado recebido recentemente.");
        
        // Atualiza o display para indicar falha na conexão
        display.clearDisplay();
        display.setCursor(10, 10);
        display.print("Conexao Perdida!");
        display.display();
    }
}