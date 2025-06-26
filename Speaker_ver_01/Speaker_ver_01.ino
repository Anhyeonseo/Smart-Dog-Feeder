#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

// ESP8266Audio 라이브러리
#include <AudioFileSourceSPIFFS.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

const char* ssid = "SK_WiFiGIGAE498_2.4G";
const char* password = "1704017757";

WebServer server(80);
File fsUploadFile;

AudioGeneratorWAV *wav = nullptr;
AudioFileSourceSPIFFS *file = nullptr;
AudioOutputI2S *out = nullptr;

void handleUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("업로드 시작: %s\n", upload.filename.c_str());
    fsUploadFile = SPIFFS.open("/voice.wav", FILE_WRITE);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
    Serial.println("업로드 완료");
    server.send(200, "text/plain", "파일 업로드 성공!");
  }
}

void handlePlay() {
  Serial.println("재생 요청 수신");

  if (wav) { delete wav; wav = nullptr; }
  if (file) { delete file; file = nullptr; }
  if (out) { delete out; out = nullptr; }

  file = new AudioFileSourceSPIFFS("/voice.wav");
  out = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC); // 내장 DAC 사용!
  out->SetGain(0.5);  // 볼륨 조정
  wav = new AudioGeneratorWAV();

  if (wav->begin(file, out)) {
    server.send(200, "text/plain", "재생 시작됨");
    Serial.println("재생 시작됨...");
  } else {
    server.send(500, "text/plain", "재생 실패");
    Serial.println("재생 실패: 파일 열기 실패");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("WiFi 연결 중...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }

  Serial.println("WiFi 연결 성공");
  Serial.print("IP 주소: ");
  Serial.println(WiFi.localIP());

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS 초기화 실패");
    return;
  }

  server.on("/upload", HTTP_POST, []() {
    server.send(200);
  }, handleUpload);

  server.on("/play", HTTP_GET, handlePlay);

  server.begin();
  Serial.println("서버 시작됨 → /upload, /play 대기 중");
}

void loop() {
  server.handleClient();

  if (wav && wav->isRunning()) {
    wav->loop();
  }
}
