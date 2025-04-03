#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pinos
#define TRIG 5
#define ECHO 18
#define VRX_PIN 34
#define VRY_PIN 35
#define SW_PIN 15

// Configurações do Joystick
#define JOYSTICK_MIN 0
#define JOYSTICK_MAX 4095
#define JOYSTICK_CENTER_X 2300
#define JOYSTICK_CENTER_Y 2300
#define JOYSTICK_DEADZONE 300

// Sistema de senha
#define PASSWORD_LENGTH 4
#define ACCESS_GRANTED_TIME 3000
enum Direction { DIR_NONE, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
const Direction correctPassword[PASSWORD_LENGTH] = {DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT};

// Endereço MAC do receptor
uint8_t broadcastAddress[] = {0x08, 0xF9, 0xE0, 0xCF, 0xE8, 0xF4};

// Estrutura de dados
typedef struct __attribute__((packed)) struct_message {
  float distance;
  bool buttonPressed;
  bool accessGranted;
  uint8_t packetCounter;  // Contador de pacotes para verificação
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// Variáveis de tempo
unsigned long lastDistanceTime = 0;
unsigned long lastSendTime = 0;
unsigned long lastDisplayUpdate = 0;
const long distanceInterval = 150;  // 1 segundo para atualização da distância
const long sendInterval = 40;       // 50ms para envio de dados
const long displayInterval = 50;   // 100ms para atualização do display

// Variáveis do sistema de senha
Direction inputSequence[PASSWORD_LENGTH];
int currentInputIndex = 0;
unsigned long lastInputTime = 0;
unsigned long accessGrantedTime = 0;
bool joystickCentered = true;

// Variáveis de controle de comunicação
uint8_t lastPacketCounter = 0;
unsigned long lastSuccessfulSend = 0;
const long connectionTimeout = 2000; // 2 segundos sem comunicação

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status == ESP_NOW_SEND_SUCCESS) {
    lastSuccessfulSend = millis();
    lastPacketCounter = myData.packetCounter;
  } else {
    Serial.println("Falha no envio - tentando novamente...");
    // Tenta reenviar imediatamente em caso de falha
    esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configuração de pinos
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(VRX_PIN, INPUT);
  pinMode(VRY_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  digitalWrite(TRIG, LOW);

  // Inicialização do display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Falha ao iniciar o display OLED!");
    while(1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.display();

  // Configuração WiFi e ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // Desativa o sleep para melhor desempenho
  
  if(esp_now_init() != ESP_OK) {
    Serial.println("Falha ao inicializar ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if(esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Falha ao adicionar peer");
    return;
  }

  // Inicialização dos dados
  memset(&myData, 0, sizeof(myData));
  myData.packetCounter = 0;
  lastSuccessfulSend = millis();
}

Direction getJoystickDirection(int rawX, int rawY) {
  int xDiff = rawX - JOYSTICK_CENTER_X;
  int yDiff = rawY - JOYSTICK_CENTER_Y;
  
  if(abs(yDiff) > JOYSTICK_DEADZONE && abs(yDiff) > abs(xDiff)) {
    return yDiff < 0 ? DIR_UP : DIR_DOWN;
  }
  if(abs(xDiff) > JOYSTICK_DEADZONE) {
    return xDiff < 0 ? DIR_LEFT : DIR_RIGHT;
  }
  return DIR_NONE;
}

float readDistance() {
  // Medição otimizada de distância
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration = pulseIn(ECHO, HIGH, 30000); // Timeout de 30ms
  return duration * 0.01715; // Conversão para cm
}

void updateDisplay(float distance, bool btn, bool access, bool connectionOK) {
  if(millis() - lastDisplayUpdate < displayInterval) return;
  lastDisplayUpdate = millis();

  display.clearDisplay();
  
  // Linha 1: Distância
  display.setCursor(0, 0);
  display.print("Dist: ");
  display.print(distance, 1);
  display.print("cm");

  // Linha 2: Progresso da senha
  display.setCursor(0, 12);
  display.print("Senha: [");
  for(int i = 0; i < PASSWORD_LENGTH; i++) {
    display.write(i < currentInputIndex ? 
      (inputSequence[i] == DIR_LEFT ? 'L' : 
       inputSequence[i] == DIR_RIGHT ? 'R' : 
       inputSequence[i] == DIR_UP ? 'U' : 'D') : '_');
  }
  display.print("]");

  // Linha 3: Estado do botão
  display.setCursor(0, 24);
  display.print("Botao: ");
  display.print(btn ? "PRESS" : "SOLTO");

  // Linha 4: Status de acesso e conexão
  display.setCursor(0, 36);
  display.print("Acesso: ");
  display.print(access ? "LIBERADO" : "NEGADO");
  
  display.setCursor(0, 48);
  display.print("Conexao: ");
  display.print(connectionOK ? "OK" : "FALHA");

  display.display();
}

void loop() {
  unsigned long currentTime = millis();

  // Verificação de conexão
  bool connectionOK = (currentTime - lastSuccessfulSend) < connectionTimeout;

  // Controle de acesso temporizado
  if(myData.accessGranted && (currentTime - accessGrantedTime >= ACCESS_GRANTED_TIME)) {
    myData.accessGranted = false;
    currentInputIndex = 0;
    joystickCentered = true;
  }

  // Atualização da distância a cada segundo
  if(currentTime - lastDistanceTime >= distanceInterval) {
    lastDistanceTime = currentTime;
    myData.distance = readDistance();
    Serial.print("Distancia medida: ");
    Serial.print(myData.distance);
    Serial.println(" cm");
  }

  // Processamento principal e envio de dados
  if(currentTime - lastSendTime >= sendInterval) {
    lastSendTime = currentTime;
    myData.packetCounter++;
    
    // Leitura do joystick
    Direction currentDir = getJoystickDirection(analogRead(VRX_PIN), analogRead(VRY_PIN));
    
    // Sistema de senha
    if(!myData.accessGranted) {
      if(currentDir != DIR_NONE && joystickCentered) {
        if(currentInputIndex < PASSWORD_LENGTH) {
          inputSequence[currentInputIndex++] = currentDir;
          lastInputTime = currentTime;
          joystickCentered = false;
        }
      }
      
      if(currentDir == DIR_NONE) joystickCentered = true;
      
      if(currentInputIndex == PASSWORD_LENGTH && joystickCentered) {
        bool correct = true;
        for(int i = 0; i < PASSWORD_LENGTH && correct; i++) {
          correct = (inputSequence[i] == correctPassword[i]);
        }
        
        if(correct) {
          myData.accessGranted = true;
          accessGrantedTime = currentTime;
          Serial.println("Acesso liberado!");
        } else {
          currentInputIndex = 0;
          Serial.println("Senha incorreta!");
        }
      }
      
      if(currentInputIndex > 0 && (currentTime - lastInputTime > 2000)) {
        currentInputIndex = 0;
        joystickCentered = true;
      }
    }
    
    // Leitura do botão
    myData.buttonPressed = (digitalRead(SW_PIN) == LOW);
    if(myData.buttonPressed) {
      myData.accessGranted = false;
      currentInputIndex = 0;
      joystickCentered = true;
      // Serial.println("Botao pressionado - reset");
    }

    // Envio dos dados
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    if(result != ESP_OK) {
      Serial.println("Erro ao enviar dados");
    }
  }

  // Atualização do display
  updateDisplay(myData.distance, myData.buttonPressed, myData.accessGranted, connectionOK);
}