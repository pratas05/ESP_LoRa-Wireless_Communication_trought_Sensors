#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C


#define RED_LED 13
#define BLUE_LED 12
#define BUZZER_PIN 4  // Define o pino do buzzer


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// Estrutura para receber os dados
typedef struct struct_message {
 float distance;
 int joystickX;
 int joystickY;
 bool buttonPressed;
} struct_message;


// Criar estrutura para armazenar os dados recebidos
struct_message myData;


// *Callback atualizado para nova versão do ESP-NOW*
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
   memcpy(&myData, incomingData, sizeof(myData));


   // Exibir no monitor serial
   Serial.print("Distância recebida: ");
   Serial.print(myData.distance);
   Serial.println(" cm");
   Serial.print("Joystick X: ");
   Serial.print(myData.joystickX);
   Serial.print(" Y: ");
   Serial.print(myData.joystickY);
   Serial.print(" Botão: ");
   Serial.println(myData.buttonPressed ? "Pressionado" : "Solto");


   // Atualiza o display OLED
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


   // Verifica se a distância é menor que 10cm e aciona o buzzer
   if (myData.distance < 20.0 && myData.distance > 10) {
       digitalWrite(BUZZER_PIN, HIGH);  // Liga o buzzer
       digitalWrite(RED_LED, HIGH);
       delay(300);
       digitalWrite(RED_LED, LOW);
       digitalWrite(BUZZER_PIN, LOW);   // Desliga o buzzer
   }


   if (myData.distance < 10.0) {
       digitalWrite(BUZZER_PIN, HIGH);  // Liga o buzzer
       digitalWrite(BLUE_LED, HIGH);
       delay(1200);
       digitalWrite(BLUE_LED, LOW);
       digitalWrite(BUZZER_PIN, LOW);   // Desliga o buzzer
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


   // Configurar o pino do buzzer como saída
   pinMode(BUZZER_PIN, OUTPUT);
   pinMode(RED_LED, OUTPUT);
   pinMode(BLUE_LED, OUTPUT);
   digitalWrite(BUZZER_PIN, LOW);
   digitalWrite(RED_LED, LOW);
   digitalWrite(BLUE_LED, LOW);
}

void loop() {
   // O ESP32 receptor apenas aguarda os dados do transmissor
}
