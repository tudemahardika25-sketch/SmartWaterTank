#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <HTTPClient.h>

// ================= WIFI =================
const char* ssid = "Nokia 3000";
const char* password = "tude1940";

WebServer server(80);

// ================= TELEGRAM =================

String BOT_TOKEN = "8968182029:AAEtll_tJ9GnDKLPZbCBGzLMQnEv1bNmSP8";
String CHAT_ID   = "6093141682";

// ================= GOOGLE SHEET =================

String SCRIPT_URL =
"https://script.google.com/macros/s/AKfycbzUi3WK0O9yjW4hJGVQK66H6irUvLe5mHpF4UoOJLW5VylN_LXrimx0VvBzbuesuw2s/exec";

// TELEGRAM COMMAND
unsigned long lastTelegramCheck = 0;

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
// ================= PIN =================
#define TRIG_PIN          5
#define ECHO_PIN          18

// FLOW SENSOR
#define FLOW_SENSOR_1     27
#define FLOW_SENSOR_2     14

// OUTPUT
#define LED_HIJAU         25
#define LED_MERAH         26
#define BUZZER            33

// ================= VARIABEL =================

// pulse sensor
volatile unsigned long pulseCount1 = 0;
volatile unsigned long pulseCount2 = 0;

// flow rate
float flowRate1 = 0.0;
float flowRate2 = 0.0;

// total liter
float totalLiter1 = 0.0;
float totalLiter2 = 0.0;

// biaya
float hargaPerLiter = 45000;

float totalBiaya1 = 0;
float totalBiaya2 = 0;
// batas biaya
float batasBiaya = 50000;

// kalibrasi sensor
float calibrationFactor1 = 7.5;
float calibrationFactor2 = 7.5;

// timer
unsigned long lastFlowTime  = 0;
unsigned long lastBlinkTime = 0;
unsigned long lastDistanceTime = 0;

// blink
bool blinkState = false;

// status
String currentStatus = "NORMAL";
// ================= TELEGRAM ALERT =================
bool notifKamar1Terkirim = false;
bool notifKamar2Terkirim = false;
bool notifBocorTerkirim = false;
bool notifAirRendahTerkirim = false;

// batas flow kebocoran
float batasFlow = 10.0;

// simpan jarak
float currentDistance = 0;

// ================= INTERRUPT =================

// kamar 1
void IRAM_ATTR countPulse1() {
  pulseCount1++;
}

// kamar 2
void IRAM_ATTR countPulse2() {
  pulseCount2++;
}

// ================= ULTRASONIK =================

float readDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) return -1;

  return (duration * 0.034) / 2;
}

float getStableDistance() {

  float total = 0;
  int count = 0;

  for (int i = 0; i < 5; i++) {

    float d = readDistance();

    if (d > 0 && d < 400) {
      total += d;
      count++;
    }

    delay(20);
  }

  if (count == 0) return -1;

  return total / count;
}

// ================= FLOW =================

void readFlow() {

  if (millis() - lastFlowTime >= 1000) {

    float timeInterval =
      millis() - lastFlowTime;

    // ================= FLOW KAMAR 1 =================

    flowRate1 =
      ((1000.0 / timeInterval) * pulseCount1)
      / calibrationFactor1;

    float literPerSecond1 =
      flowRate1 / 60.0;

    totalLiter1 +=
      literPerSecond1 *
      (timeInterval / 1000.0);

    totalBiaya1 =
      totalLiter1 * hargaPerLiter;

    // ================= FLOW KAMAR 2 =================

    flowRate2 =
      ((1000.0 / timeInterval) * pulseCount2)
      / calibrationFactor2;

    float literPerSecond2 =
      flowRate2 / 60.0;

    totalLiter2 +=
      literPerSecond2 *
      (timeInterval / 1000.0);

    totalBiaya2 =
      totalLiter2 * hargaPerLiter;

    // reset pulse
    pulseCount1 = 0;
    pulseCount2 = 0;

    lastFlowTime = millis();
  }
}

// ================= CEK LEVEL =================

bool adaPerubahanLevel(float distance) {

  static float lastDistance = 0;

  float selisih =
    abs(distance - lastDistance);

  lastDistance = distance;

  if (selisih > 1.0) {
    return true;
  }

  return false;
}

// ================= WEB =================
void handleRoot() {

  String html = "<!DOCTYPE html>";

  html += "<html>";
  html += "<head>";

  html += "<meta http-equiv='refresh' content='2'>";

  html += "<title>Smart Water Monitoring</title>";

  html += "<style>";

  html += "body{";
  html += "font-family:Arial;";
  html += "background:#f4f4f4;";
  html += "text-align:center;";
  html += "padding:20px;";
  html += "}";

  html += ".card{";
  html += "background:white;";
  html += "padding:20px;";
  html += "margin:20px auto;";
  html += "max-width:600px;";
  html += "border-radius:15px;";
  html += "box-shadow:0 0 10px rgba(0,0,0,0.1);";
  html += "}";

  html += ".normal{color:green;font-weight:bold;}";
  html += ".warning{color:red;font-weight:bold;}";
  html += ".title{color:#0077cc;}";

  html += "</style>";

  html += "</head>";
  html += "<body>";

  html += "<h1 class='title'>SMART WATER MONITORING SYSTEM</h1>";

  // ================= TANGKI =================

  html += "<div class='card'>";
  html += "<h2>LEVEL AIR TANGKI</h2>";
  html += "<p><b>";
  html += String(currentDistance, 1);
  html += " cm</b></p>";
  html += "</div>";

  // ================= KAMAR 1 =================

  html += "<div class='card'>";

  html += "<h2>KAMAR 1</h2>";

  html += "<p>Flow Rate : <b>";
  html += String(flowRate1, 2);
  html += " L/min</b></p>";

  html += "<p>Total Air : <b>";
  html += String(totalLiter1, 2);
  html += " Liter</b></p>";

  html += "<p>Total Biaya : <b>Rp ";
  html += String(totalBiaya1, 0);
  html += "</b></p>";

  if (totalBiaya1 >= batasBiaya) {

    html += "<p class='warning'>";
    html += "TAGIHAN KAMAR 1 MELEBIHI Rp50.000";
    html += "</p>";
  }

  html += "</div>";

  // ================= KAMAR 2 =================

  html += "<div class='card'>";

  html += "<h2>KAMAR 2</h2>";

  html += "<p>Flow Rate : <b>";
  html += String(flowRate2, 2);
  html += " L/min</b></p>";

  html += "<p>Total Air : <b>";
  html += String(totalLiter2, 2);
  html += " Liter</b></p>";

  html += "<p>Total Biaya : <b>Rp ";
  html += String(totalBiaya2, 0);
  html += "</b></p>";

  if (totalBiaya2 >= batasBiaya) {

    html += "<p class='warning'>";
    html += "TAGIHAN KAMAR 2 MELEBIHI Rp50.000";
    html += "</p>";
  }

  html += "</div>";

  // ================= STATUS =================

  html += "<div class='card'>";

  html += "<h2>STATUS SISTEM</h2>";

  if (currentStatus == "NORMAL") {

    html += "<p class='normal'>";
    html += currentStatus;
    html += "</p>";
  }
  else {

    html += "<p class='warning'>";
    html += currentStatus;
    html += "</p>";
  }

  html += "</div>";

  // ================= ALERT GLOBAL =================

  if (totalBiaya1 >= batasBiaya ||
      totalBiaya2 >= batasBiaya) {

    html += "<div class='card'>";

    html += "<h2 style='color:red'>";
    html += "PERINGATAN";
    html += "</h2>";

    html += "<p class='warning'>";
    html += "BATAS PEMAKAIAN AIR TELAH TERLAMPAUI";
    html += "</p>";

    html += "</div>";
  }

  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}

void handleTelegram() {

  int numNewMessages =
    bot.getUpdates(
      bot.last_message_received + 1
    );

  while (numNewMessages) {

    for (int i = 0; i < numNewMessages; i++) {

      String text =
        bot.messages[i].text;

      String chat_id =
        bot.messages[i].chat_id;

      // ================= HELP =================

      if (text == "/help") {

        String pesan =
          "🤖 SMART WATER MONITORING\n\n"
          "📊 Monitoring\n"
          "/status\n"
          "/kamar1\n"
          "/kamar2\n\n"

          "📋 Laporan\n"
          "/laporan\n\n"

          "🔄 Reset\n"
          "/reset1\n"
          "/reset2\n"
          "/resetall";

        bot.sendMessage(chat_id, pesan, "");
      }
      // ================= START =================

else if (text == "/start") {

  String pesan =
    "🤖 SMART WATER MONITORING\n\n"
    "Gunakan command berikut:\n\n"

    "/status\n"
    "/kamar1\n"
    "/kamar2\n"
    "/laporan\n"
    "/reset1\n"
    "/reset2\n"
    "/resetall\n"
    "/help";

  bot.sendMessage(chat_id, pesan, "");
}

      // ================= KAMAR 1 =================

      else if (text == "/kamar1") {

        String pesan =
          "📊 DATA KAMAR 1\n\n";

        pesan +=
          "Flow Rate : " +
          String(flowRate1, 2) +
          " L/min\n";

        pesan +=
          "Total Air : " +
          String(totalLiter1, 2) +
          " Liter\n";

        pesan +=
          "Total Biaya : Rp" +
          String(totalBiaya1, 0);

        bot.sendMessage(chat_id, pesan, "");
      }

      // ================= KAMAR 2 =================

      else if (text == "/kamar2") {

        String pesan =
          "📊 DATA KAMAR 2\n\n";

        pesan +=
          "Flow Rate : " +
          String(flowRate2, 2) +
          " L/min\n";

        pesan +=
          "Total Air : " +
          String(totalLiter2, 2) +
          " Liter\n";

        pesan +=
          "Total Biaya : Rp" +
          String(totalBiaya2, 0);

        bot.sendMessage(chat_id, pesan, "");
      }

      // ================= STATUS =================

      else if (text == "/status") {

        String pesan =
          "📡 SMART WATER MONITORING\n\n";

        pesan +=
          "Jarak Air : " +
          String(currentDistance, 1) +
          " cm\n\n";

        pesan +=
          "Kamar 1 : Rp" +
          String(totalBiaya1, 0) +
          "\n";

        pesan +=
          "Kamar 2 : Rp" +
          String(totalBiaya2, 0) +
          "\n\n";

        pesan +=
          "Status : " +
          currentStatus;

        bot.sendMessage(chat_id, pesan, "");
      }

      // ================= LAPORAN =================

      else if (text == "/laporan") {

        String pesan =
          "📋 LAPORAN BIAYA\n\n";

        pesan +=
          "KAMAR 1\n";

        pesan +=
          "Liter : " +
          String(totalLiter1, 2) +
          "\n";

        pesan +=
          "Biaya : Rp" +
          String(totalBiaya1, 0) +
          "\n\n";

        pesan +=
          "KAMAR 2\n";

        pesan +=
          "Liter : " +
          String(totalLiter2, 2) +
          "\n";

        pesan +=
          "Biaya : Rp" +
          String(totalBiaya2, 0);

        bot.sendMessage(chat_id, pesan, "");
      }

      // ================= RESET KAMAR 1 =================

      else if (text == "/reset1") {

        String laporan =
          "📋 LAPORAN BULANAN\n\n";

        laporan +=
          "KAMAR 1\n";

        laporan +=
          "Total Air : " +
          String(totalLiter1, 2) +
          " Liter\n";

        laporan +=
          "Total Tagihan : Rp" +
          String(totalBiaya1, 0);

        bot.sendMessage(chat_id, laporan, "");

        simpanKeSpreadsheet(
  "Kamar1",
  totalLiter1,
  totalBiaya1
);

        totalLiter1 = 0;
        totalBiaya1 = 0;

        notifKamar1Terkirim = false;

        bot.sendMessage(
          chat_id,
          "✅ KAMAR 1 berhasil direset",
          ""
        );
      }

      // ================= RESET KAMAR 2 =================

      else if (text == "/reset2") {

        String laporan =
          "📋 LAPORAN BULANAN\n\n";

        laporan +=
          "KAMAR 2\n";

        laporan +=
          "Total Air : " +
          String(totalLiter2, 2) +
          " Liter\n";

        laporan +=
          "Total Tagihan : Rp" +
          String(totalBiaya2, 0);

        bot.sendMessage(chat_id, laporan, "");

        simpanKeSpreadsheet(
  "Kamar2",
  totalLiter2,
  totalBiaya2
);

        totalLiter2 = 0;
        totalBiaya2 = 0;

        notifKamar2Terkirim = false;

        bot.sendMessage(
          chat_id,
          "✅ KAMAR 2 berhasil direset",
          ""
        );
      }

      // ================= RESET ALL =================

      else if (text == "/resetall") {

        String laporan =
          "📋 LAPORAN BULANAN\n\n";

        laporan +=
          "KAMAR 1 : Rp" +
          String(totalBiaya1, 0) +
          "\n";

        laporan +=
          "KAMAR 2 : Rp" +
          String(totalBiaya2, 0);

        bot.sendMessage(chat_id, laporan, "");

        simpanKeSpreadsheet(
  "Kamar1",
  totalLiter1,
  totalBiaya1
);

simpanKeSpreadsheet(
  "Kamar2",
  totalLiter2,
  totalBiaya2
);

      totalLiter1 = 0;
      totalBiaya1 = 0;

      totalLiter2 = 0;
      totalBiaya2 = 0;

      notifKamar1Terkirim = false;
      notifKamar2Terkirim = false;
      notifBocorTerkirim = false;
      notifAirRendahTerkirim = false;

        bot.sendMessage(
          chat_id,
          "✅ SELURUH DATA BERHASIL DIRESET",
          ""
        );
      }
    }

    numNewMessages =
      bot.getUpdates(
        bot.last_message_received + 1
      );
  }
}

void simpanKeSpreadsheet(
  String kamar,
  float liter,
  float biaya
) {

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = SCRIPT_URL;

  url += "?kamar=" + kamar;
  url += "&liter=" + String(liter, 2);
  url += "&biaya=" + String((long)biaya);

  Serial.println(url);

  http.setFollowRedirects(
    HTTPC_STRICT_FOLLOW_REDIRECTS
  );

  http.begin(client, url);

  int httpCode = http.GET();

  Serial.print("HTTP Code : ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    Serial.println(http.getString());
  }

  http.end();
}
void setup() {

  Serial.begin(115200);

  // ultrasonik
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // flow sensor
  pinMode(FLOW_SENSOR_1, INPUT_PULLUP);
  pinMode(FLOW_SENSOR_2, INPUT_PULLUP);

  // output
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // interrupt kamar 1
  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR_1),
    countPulse1,
    FALLING
  );

  // interrupt kamar 2
  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR_2),
    countPulse2,
    FALLING
  );

  // ================= WIFI =================

  WiFi.begin(ssid, password);

  Serial.print("Menghubungkan WiFi");

while (WiFi.status() != WL_CONNECTED) {

  delay(500);
  Serial.print(".");
}

Serial.println();
Serial.println("WiFi Connected");

// Tambahkan ini untuk Telegram HTTPS
client.setInsecure();

Serial.print("IP Address : ");
Serial.println(WiFi.localIP());


  // ================= SERVER =================


server.on("/", handleRoot);

server.begin();

// KIRIM PESAN KE TELEGRAM SAAT ESP32 MENYALA
bot.sendMessage(
  CHAT_ID,
  "ESP32 Smart Water Monitoring Online",
  ""
);

Serial.println("Web Server Started");

lastFlowTime = millis();
}

// ================= LOOP =================

void loop() {

  server.handleClient();
  if (millis() - lastTelegramCheck > 1000) {

  handleTelegram();

  lastTelegramCheck = millis();
}

  // baca ultrasonik tiap 2 detik
  if (millis() - lastDistanceTime >= 2000) {

    currentDistance = getStableDistance();

    if (currentDistance == -1) {
      currentDistance = 999;
    }

    lastDistanceTime = millis();
  }

  // baca flow sensor
  readFlow();

  // ================= LOGIKA =================

  bool airRendah =
    (currentDistance > 15);

    if (airRendah && !notifAirRendahTerkirim) {

  bot.sendMessage(
    CHAT_ID,
    "LEVEL AIR TANGKI RENDAH",
    ""
  );

  notifAirRendahTerkirim = true;
}

  bool flowTinggi1 =
    (flowRate1 > batasFlow);

  bool flowTinggi2 =
    (flowRate2 > batasFlow);

    bool biayaLebih1 =
  (totalBiaya1 >= batasBiaya);

  bool biayaLebih2 =
  (totalBiaya2 >= batasBiaya);



  // ================= TELEGRAM ALERT =================

// ALERT KAMAR 1
if (biayaLebih1 && !notifKamar1Terkirim) {

  String pesan =
    "ALERT SMART WATER MONITORING\n\n"
    "Tagihan KAMAR 1 melebihi Rp50.000\n"
    "Total Tagihan : Rp " +
    String(totalBiaya1, 0);

  bot.sendMessage(CHAT_ID, pesan, "");

  notifKamar1Terkirim = true;
}

// ALERT KAMAR 2
if (biayaLebih2 && !notifKamar2Terkirim) {

  String pesan =
    "ALERT SMART WATER MONITORING\n\n"
    "Tagihan KAMAR 2 melebihi Rp50.000\n"
    "Total Tagihan : Rp " +
    String(totalBiaya2, 0);

  bot.sendMessage(CHAT_ID, pesan, "");

  notifKamar2Terkirim = true;
}

  bool kemungkinanBocor = false;

  if (flowTinggi1 || flowTinggi2) {

    if (!adaPerubahanLevel(currentDistance)) {

      kemungkinanBocor = true;

      if (!notifBocorTerkirim) {

  bot.sendMessage(
    CHAT_ID,
    "KEBOCORAN TERDETEKSI PADA SISTEM AIR",
    ""
  );

  notifBocorTerkirim = true;
}
    }
  }

  // ================= OUTPUT =================
// ================= OUTPUT =================

// ===== BIAYA KAMAR 1 DAN 2 =====
if (biayaLebih1 && biayaLebih2) {

  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(BUZZER, LOW);

  if (millis() - lastBlinkTime >= 500) {

    blinkState = !blinkState;
    digitalWrite(LED_MERAH, blinkState);

    lastBlinkTime = millis();
  }

  currentStatus =
    "TAGIHAN KAMAR 1 DAN 2 MELEBIHI Rp50.000";
}

// ===== BIAYA KAMAR 1 =====
else if (biayaLebih1) {

  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(BUZZER, LOW);

  if (millis() - lastBlinkTime >= 500) {

    blinkState = !blinkState;
    digitalWrite(LED_MERAH, blinkState);

    lastBlinkTime = millis();
  }

  currentStatus =
    "TAGIHAN KAMAR 1 MELEBIHI Rp50.000";
}

// ===== BIAYA KAMAR 2 =====
else if (biayaLebih2) {

  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(BUZZER, LOW);

  if (millis() - lastBlinkTime >= 500) {

    blinkState = !blinkState;
    digitalWrite(LED_MERAH, blinkState);

    lastBlinkTime = millis();
  }

  currentStatus =
    "TAGIHAN KAMAR 2 MELEBIHI Rp50.000";
}

// ===== KEBOCORAN =====
else if (kemungkinanBocor) {

  digitalWrite(LED_HIJAU, LOW);

  if (millis() - lastBlinkTime >= 300) {

    blinkState = !blinkState;

    digitalWrite(LED_MERAH, blinkState);
    digitalWrite(BUZZER, blinkState);

    lastBlinkTime = millis();
  }

  currentStatus =
    "KEBOCORAN TERDETEKSI";
}

// ===== AIR RENDAH =====
else if (airRendah) {

  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(LED_MERAH, HIGH);
  digitalWrite(BUZZER, LOW);

  currentStatus =
    "AIR RENDAH";
}
// RESET FLAG NOTIFIKASI
if (!biayaLebih1) {
  notifKamar1Terkirim = false;
}

if (!biayaLebih2) {
  notifKamar2Terkirim = false;
}

if (!kemungkinanBocor) {
  notifBocorTerkirim = false;
}

if (!airRendah) {
  notifAirRendahTerkirim = false;
}

// ===== NORMAL =====
if (
    !biayaLebih1 &&
    !biayaLebih2 &&
    !kemungkinanBocor &&
    !airRendah
   ) {

  digitalWrite(LED_HIJAU, HIGH);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(BUZZER, LOW);

  currentStatus =
    "NORMAL";
}

  // ================= SERIAL MONITOR =================

  Serial.println("================================");

  Serial.print("Jarak Air : ");
  Serial.print(currentDistance);
  Serial.println(" cm");

  Serial.println("----- KAMAR 1 -----");

  Serial.print("Flow Rate : ");
  Serial.print(flowRate1);
  Serial.println(" L/min");

  Serial.print("Total Air : ");
  Serial.print(totalLiter1);
  Serial.println(" Liter");

  Serial.print("Biaya : Rp ");
  Serial.println(totalBiaya1);

  Serial.println("----- KAMAR 2 -----");

  Serial.print("Flow Rate : ");
  Serial.print(flowRate2);
  Serial.println(" L/min");

  Serial.print("Total Air : ");
  Serial.println(totalLiter2);

  Serial.print("Biaya : Rp ");
  Serial.println(totalBiaya2);

  Serial.print("Status : ");
  Serial.println(currentStatus);

  Serial.println("================================");

  delay(200);
}