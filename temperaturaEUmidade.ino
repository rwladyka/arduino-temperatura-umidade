
#include <SPI.h>
#include <SdFat.h>
#include <Ethernet.h>
#include <DHT.h>

#define W5100_CS  10
#define DHTPIN 7        // Pino digital conectado
#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321

DHT dht(DHTPIN, DHTTYPE, 11);
EthernetClient client;

//----------------- BEGIN SD DEFINITIONS -----------------
// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 3
#define USE_ARDUINO_SPI_LIBRARY 1

// SDCARD_SS_PIN is defined for the built-in SD on some boards.
const uint8_t SD_CS_PIN = 4;

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(50)

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif  ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif  // HAS_SDIO_CLASS

#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile file;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

char line[40];

//----------------- END SD DEFINITIONS -----------------

String ID;
String SERVER_ADDR; //Server address
String RASP_ADDR; //Raspberry hostname

void sendRasp(float h, float t) {
  int str_len = RASP_ADDR.length() + 1; 
  char addr[str_len];
  RASP_ADDR.toCharArray(addr, str_len);
  request(h, t, addr);  
}

void sendServer(float h, float t) {
  int str_len = SERVER_ADDR.length() + 1; 
  char addr[str_len];
  SERVER_ADDR.toCharArray(addr, str_len);
  request(h, t, addr);  
}

void request(float h, float t, char host[]) {
  
  pinMode(4,OUTPUT); 
  digitalWrite(10, HIGH); // select ethernet mode

  if (client.connect(host, 8080)) {
    char URL[] = "GET /temperatura/informar?";
    String query = "id=" + ID + "&u=" + String(h) + "&t=" + String(t);
    
    Serial.println(query);
    client.println(String(URL) + query);
    client.println("Host: " + String(host));
    client.println("Connection: close");
    client.println(); 
    
    client.flush();
    client.stop();
  }
}


void readSensor() {
  float h = dht.readHumidity(); // Le umidade
  float t = dht.readTemperature(); // Le temperatura

  if (isnan(h) || isnan(t)) {
    Serial.println("Sensor fail");
    return;
  }

  sendRasp(h, t);
  sendServer(h, t);
}

void setupEthernetShield() {

  Serial.println("Start Ethernet");
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };//mac ethernet shield
  if (Ethernet.begin(mac) == 0) {
    while(true);
  }
  Serial.println(Ethernet.localIP());
}

bool parseLine(char* str) {

  // Set strtok start of line.
  str = strtok(str, "=");
  if (!str) return false;
  
  // Subsequent calls to strtok expects a null pointer.
  if(strstr(str, "ID")) {
    str = strtok(nullptr, "=");
    if (!str) return false;
    ID = str;
    ID.trim();
  } else if(strstr(str, "SERV")) {
    str = strtok(nullptr, "=");
    if (!str) return false;
    SERVER_ADDR = str;
    SERVER_ADDR.trim();
  } else if(strstr(str, "RASP")) {
    str = strtok(nullptr, "=");
    if (!str) return false;
    RASP_ADDR = str;
    RASP_ADDR.trim();
  } else {
    str = strtok(nullptr, "=");
    if (!str) return false;
  }

  return strtok(NULL, "=") == nullptr;
}

void readFile() {
  
  pinMode(10,OUTPUT); 

  Serial.println("Start SD");
  // Initialize the SD.
  if (!sd.begin(SD_CONFIG)) {
    sd.initErrorHalt(&Serial);
    sd.end();
    return;
  }

  // Create the file.
  if (!file.open("config.txt", FILE_READ)) {
    Serial.println("open fail");
    sd.end();
    return;
  }

  while (file.available()) {
    int n = file.fgets(line, sizeof(line));
    if (n <= 0) {
      Serial.println("fgets fail");
    }
    if (line[n-1] != '\n' && n == (sizeof(line) - 1)) {
      Serial.println("line so long");
    }
    if (!parseLine(line)) {
      Serial.println("parse failed");
    }
  }
  
  file.close();
  sd.end();
}

void setup() {
  Serial.begin(9600);
  while (!Serial){;}
  Serial.println("Starting");
  
  setupEthernetShield();
  readFile();
  dht.begin();
  
  Serial.println("Setup Finished");
}


void loop() {
  delay(60000);
  readSensor();
}
