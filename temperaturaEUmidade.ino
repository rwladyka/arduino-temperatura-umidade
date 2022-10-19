
#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>  
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

#define W5100_CS  10
#define SENSOR1 7        // Pino digital conectado
#define SENSOR2 8        // Pino digital conectado

// DEFINIÇÕES DISPLAY
#define endereco  0x27 // Endereços comuns: 0x27, 0x3F
#define colunas   16
#define linhas    2

// INSTANCIANDO OBJETOS
EthernetClient client;
LiquidCrystal_I2C lcd(endereco, colunas, linhas);
OneWire oneWire1(SENSOR1);
OneWire oneWire2(SENSOR2);
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);
StaticJsonDocument<200> doc;

// Constantes
String ID = "262121";
char SERVER_ADDR[] = "homologacao.gestox.com.br"; //Server address
int SERVER_PORT = 80;
String RASP_ADDR; //Raspberry hostname
int RASP_PORT;
const unsigned int HTTP_TIMEOUT = 1000;



void sendRasp(float t1, float t2) {
  configRaspberry();
  if(RASP_ADDR == NULL) {
    Serial.println(F("Raspberry not configured"));
    return;
  } 
 
  int str_len = RASP_ADDR.length() + 1; 
  char addr[str_len];
  RASP_ADDR.toCharArray(addr, str_len);
  request(t1, t2, addr, RASP_PORT);  
}

void sendServer(float t1, float t2) {
  if(SERVER_ADDR == NULL) {
    Serial.println(F("Server address not configured"));
    return;
  }
  request(t1, t2, SERVER_ADDR, SERVER_PORT);  
}

void request(float t1, float t2, char host[], int port) {
  Serial.print(host);
  Serial.print(F(":"));
  Serial.println(port);
  if (client.connect(host, port)) {
    String json = "{ \"arduino\": \"" + ID + "\", \"t1\": " + String(t1) + ", \"t2\": " + String(t2) + "\" }";

    Serial.println(json);
    client.println(F("POST /temperaturas/informar HTTP/1.1"));
    client.println("Host: " + String(host));
    client.println(F("Content-Type: application/json;"));
    client.println("Content-Length: " + String(json.length() + 1));
    client.println(F("Connection: close"));
    client.println(F(""));
    client.println(json);

    client.stop();
  } else {
    Serial.println(F("Connection Faild"));  
  }
}

void configRaspberry() {
  if (client.connect(SERVER_ADDR, SERVER_PORT)) {
    char c[128];

    Serial.println(F("Getting Raspberry configs"));
    client.print(F("GET /temperaturas/config-arduino/"));
    client.print(ID);
    client.println(F(" HTTP/1.1"));
    client.println("Host: " + String(SERVER_ADDR));
    client.println(F("Connection: close"));
    client.println();
    client.setTimeout(HTTP_TIMEOUT);

    char endOfHeaders[] = "\r\n\r\n";
    client.find(endOfHeaders);
    
    int index = 0;
    boolean started = false;

    while(client.available()) {
      char ch = client.read();
      if (!started && ch == '{') started = true;
      if (started) {
        c[index] = ch;
        index++;
      }
      if (ch == '}')break;
      
    };

    deserializeJson(doc, c);
    const char* ip = doc["ip"];
    String port = doc["porta"];
    
    if (ip != NULL) RASP_ADDR = String(ip);
    if (port != NULL) RASP_PORT = port.toInt();

    client.stop();
  }
}

void readSensor() {
  sensors1.requestTemperatures();
  sensors2.requestTemperatures();
  float t1 = sensors1.getTempCByIndex(0); // Le temperatura
  float t2 = sensors2.getTempCByIndex(0); // Le temperatura

  if (isnan(t1) || isnan(t2)) {
    Serial.println(F("Fail"));
    return;
  }

  lcd.print(F("T1:"));
  lcd.print(t1);
  lcd.print(F(" T2:"));
  lcd.print(t2);

  sendServer(t1, t2);
  sendRasp(t1, t2);
}

void setupEthernetShield() {
  lcd.clear();
  lcd.println(F("Start Ethernet"));
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };//mac ethernet shield
  if (Ethernet.begin(mac) == 0) {
    while(true);
  }
  lcd.print(Ethernet.localIP());
  delay(2000);
}

void setupDisplay() {
  lcd.init(); // INICIA A COMUNICAÇÃO COM O DISPLAY
  lcd.begin(16, 2);
  lcd.backlight(); // LIGA A ILUMINAÇÃO DO DISPLAY
  lcd.clear(); // LIMPA O DISPLAY
}

void setup() {
  Serial.begin(9600);
  while (!Serial){;}
  setupDisplay();
  Serial.println(F("Starting..."));
  lcd.print(F("Starting..."));
  
  setupEthernetShield();
  sensors1.begin();
  sensors2.begin();
  
  lcd.clear();
  Serial.println(F("Setup Finished"));
  lcd.print(F("Setup Finished"));
  delay(1000);
  lcd.clear();
  lcd.print(F("IP: "));
  lcd.print(Ethernet.localIP());
  configRaspberry();
}


void loop() {
  delay(6000);
  lcd.setCursor(0, 1); // POSICIONA O CURSOR NA PRIMEIRA COLUNA DA LINHA 2
  readSensor();
}
