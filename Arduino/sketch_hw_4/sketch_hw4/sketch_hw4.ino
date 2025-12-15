#include <EtherCard.h>
#include <SD.h>

// ---------------------- NETWORK CONFIG --------------------------
static byte myip[]  = {169, 254, 238, 216};
static byte gwip[]  = {169, 254, 135, 79};
static byte mymac[] = {0x74, 0x69, 0x69, 0x2D, 0x30, 0x31};

byte Ethernet::buffer[2500];

// ---------------------- PINS ------------------------------------
#define TRIG_PIN 3
#define ECHO_PIN 2

#define SD_CS   36
#define ETH_CS  53   // ENC28J60 CS

// ---------------------- SENSOR DATA ------------------------------
#define HISTORY_SIZE 20

unsigned long lastMeasure = 0;
long currentDistance = 0;
long history[HISTORY_SIZE];
int histIndex = 0;

// ---------------------- MEASURE DISTANCE ------------------------
long measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

// ---------------------- SEND FILE FROM SD ------------------------
void sendFile(const char* filename, const char* contentType) {
  File file = SD.open(filename);
  if (!file) {
    ether.httpServerReply(0);
    return;
  }

  char* ptr = (char*)ether.tcpOffset();
  ptr += sprintf(ptr,
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Connection: close\r\n\r\n",
    contentType
  );

  while (file.available()) {
    *ptr++ = file.read();
  }

  file.close();
  ether.httpServerReply(ptr - (char*)ether.tcpOffset());
}

// ---------------------- SETUP -----------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // ---- SPI CS FIX (ОБЯЗАТЕЛЬНО) ----
  pinMode(53, OUTPUT);
  digitalWrite(53, HIGH);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  pinMode(ETH_CS, OUTPUT);
  digitalWrite(ETH_CS, HIGH);

  // ---- SD INIT ----
  Serial.print("SD init... ");
  if (!SD.begin(SD_CS)) {
    Serial.println("FAIL");
    while (1);
  }
  Serial.println("OK");

  // ---- ETHERNET INIT ----
  ether.begin(sizeof Ethernet::buffer, mymac, ETH_CS);
  ether.staticSetup(myip, gwip);

  Serial.println("Web server started");
}

// ---------------------- LOOP ------------------------------------
void loop() {

  // ---- SENSOR UPDATE ----
  if (millis() - lastMeasure >= 1000) {
    lastMeasure = millis();

    currentDistance = measureDistance();
    history[histIndex] = currentDistance;
    histIndex = (histIndex + 1) % HISTORY_SIZE;
  }

  // ---- ETHERNET ----
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  if (!pos) return;

  char* req = (char*)Ethernet::buffer + pos;

  // ---- JSON DATA ----
  if (strncmp(req, "GET /data", 9) == 0) {
    char* reply = (char*)ether.tcpOffset();

    int l = sprintf(reply, "{\"current\":%ld,\"values\":[", currentDistance);

    for (int i = 0; i < HISTORY_SIZE; i++) {
      int idx = (histIndex + i) % HISTORY_SIZE;
      l += sprintf(reply + l, "%ld%s",
                   history[idx],
                   (i < HISTORY_SIZE - 1) ? "," : "");
    }

    l += sprintf(reply + l, "]}");
    ether.httpServerReply(l);
    return;
  }

  // ---- ROUTING ----
  if (strncmp(req, "GET / ", 6) == 0) {
    sendFile("html/index.htm", "text/html");
  }
  else if (strncmp(req, "GET /page2.htm", 14) == 0) {
    sendFile("html/page2.htm", "text/html");
  }
  else if (strncmp(req, "GET /page3.htm", 14) == 0) {
    sendFile("html/page3.htm", "text/html");
  }
  else if (strncmp(req, "GET /page4.htm", 14) == 0) {
    sendFile("html/page4.htm", "text/html");
  }
  else if (strncmp(req, "GET /css/style.css", 18) == 0) {
    sendFile("css/style.css", "text/css");
  }
  else if (strncmp(req, "GET /js/app.js", 14) == 0) {
    sendFile("js/app.js", "application/javascript");
  }
  else {
    ether.httpServerReply(0);
  }
}
