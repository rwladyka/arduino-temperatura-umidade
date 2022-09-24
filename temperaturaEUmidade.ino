
#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define W5100_CS  10
#define DHTPIN 7        // Pino digital conectado
#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321

// DEFINIÇÕES DISPLAY
#define endereco  0x27 // Endereços comuns: 0x27, 0x3F
#define colunas   16
#define linhas    2

// INSTANCIANDO OBJETOS
DHT dht(DHTPIN, DHTTYPE, 11);
EthernetClient client;
LiquidCrystal_I2C lcd(endereco, colunas, linhas);

// Constantes
String ID = "0262119";
String SERVER_ADDR = "homologacao.gestox.com.br"; //Server address
//String RASP_ADDR; //Raspberry hostname

//void sendRasp(float h, float t) {
//  if(RASP_ADDR == NULL) {
//    Serial.println(F("Raspberry not configured"));
//    return;
//  }
//  int str_len = RASP_ADDR.length() + 1; 
//  char addr[str_len];
//  RASP_ADDR.toCharArray(addr, str_len);
//  request(h, t, addr);  
//}

void sendServer(float h, float t) {
  if(SERVER_ADDR == NULL) {
    Serial.println(F("Server address not configured"));
    return;
  }
  int str_len = SERVER_ADDR.length() + 1; 
  char addr[str_len];
  SERVER_ADDR.toCharArray(addr, str_len);
  request(h, t, addr);  
}

void request(float h, float t, char host[]) {
  
  pinMode(4,OUTPUT); 
  digitalWrite(10, HIGH); // select ethernet mode

  if (client.connect(host, 80)) {
    String json = "{ \"id\": \"" + ID + "\", \"u\": " + String(h) + ", \"t\": " + String(t) + " }";

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
  lcd.print("T: 15, U: 10");
  Serial.println("T: 15, U: 10");
  float h = dht.readHumidity(); // Le umidade
  float t = dht.readTemperature(); // Le temperatura

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Fail"));
    return;
  }

//  sendRasp(h, t);
  sendServer(h, t);
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
  dht.begin();
  
  lcd.clear();
  Serial.println(F("Setup Finished"));
  lcd.print(F("Setup Finished"));
  delay(1000);
  lcd.clear();
  lcd.print(F("IP: "));
  lcd.print(Ethernet.localIP());
}


void loop() {
  delay(6000);
  lcd.setCursor(0, 1); // POSICIONA O CURSOR NA PRIMEIRA COLUNA DA LINHA 2
  readSensor();
}
