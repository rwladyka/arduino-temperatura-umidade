
#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>  
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

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

// Constantes
String ID = "262121";
String SERVER_ADDR = "homologacao.gestox.com.br"; //Server address
String RASP_ADDR = "192.168.100.123"; //Raspberry hostname
int RASP_PORT;

void sendRasp(float t1, float t2) {
  if(RASP_ADDR == NULL) {
    Serial.println(F("Raspberry not configured"));
    return;
  } 
 
  int str_len = RASP_ADDR.length() + 1; 
  char addr[str_len];
  RASP_ADDR.toCharArray(addr, str_len);
  request(t1, t2, addr, 7500);  
}

void sendServer(float t1, float t2) {
  if(SERVER_ADDR == NULL) {
    Serial.println(F("Server address not configured"));
    return;
  }
  int str_len = SERVER_ADDR.length() + 1; 
  char addr[str_len];
  SERVER_ADDR.toCharArray(addr, str_len);
  request(t1, t2, addr, 80);  
}

void request(float t1, float t2, char host[], int port) {
  
  pinMode(4,OUTPUT); 
  digitalWrite(10, HIGH); // select ethernet mode

  if (client.connect(host, port)) {
    String json = "{ \"name\": \"" + ID + "\", \"t2\": " + String(t2) + ", \"t1\": " + String(t1) + " }";

    Serial.println(host);
    Serial.println(json);
    client.println(F("POST /temperaturas/informar HTTP/1.1"));
    client.println("Host: " + String(host));
    client.println(F("Content-Type: application/json;"));
    client.println(F("User-Agent: Arduino/1.0"));
    client.println("Content-Length: " + String(json.length() + 1));
    client.println(F("Connection: close"));
    client.println(F(""));
    client.println(json);

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

  sendRasp(t1, t2);
  sendServer(t1, t2);
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
}


void loop() {
  delay(60000);
  lcd.setCursor(0, 1); // POSICIONA O CURSOR NA PRIMEIRA COLUNA DA LINHA 2
  readSensor();
}
