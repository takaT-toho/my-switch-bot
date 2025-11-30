#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// ★ここを自分のWi-Fiに合わせて書き換え★
const char* ssid     = "your_ssid";      // ← Wi-Fi SSID
const char* password = "your_password";  // ← Wi-Fi パスワード

WebServer server(80);
Servo servo;

// サーボ信号線のピン (ESP32 の GPIO 番号)
const int SERVO_PIN = 18;

// 角度を「変数」で管理する
int restAngle  = 90;   // 待機位置（現在の角度として扱う）
int pressAngle = 120;  // 押し込む位置
int currentAngle = restAngle;  // 現在角度（論理上の現在位置）

// シンプルなWebページ（押すボタン1つ）
const char MAIN_page[] PROGMEM = R"=====( 
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP32 Switch Pusher</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; text-align: center; margin-top: 40px; }
    button {
      font-size: 24px; padding: 20px 40px; margin: 10px;
      border-radius: 8px; border: none; cursor: pointer;
    }
    .press { background: #4CAF50; color: white; }
  </style>
</head>
<body>
  <h1>Switch Pusher</h1>
  <p>ボタンを押すと、サーボが「押して戻る」動作をします。</p>
  <button class="press" onclick="sendPress()">PUSH</button>

  <script>
    function sendPress() {
      fetch('/press')
        .then(r => r.text())
        .then(t => console.log(t))
        .catch(e => console.error(e));
    }
  </script>
</body>
</html>
)=====";

void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

// 一回「押して戻る」動作
void pressOnce() {
  int startAngle = currentAngle;  // 変数で管理している現在角度

  // 押し込む
  servo.write(pressAngle);
  delay(300); // 押し込み時間（調整可）

  // 元の角度に戻す
  servo.write(startAngle);
  delay(300); // 戻り時間（必要に応じて調整）

  // 今回の設計では「押した後も restAngle に戻る前提」
  // currentAngle は startAngle (= restAngle) のままでよい
}

void handlePress() {
  pressOnce();
  server.send(200, "text/plain", "Pressed");
}

void setup() {
  Serial.begin(115200);

  // サーボ初期化
  servo.attach(SERVO_PIN, 500, 2400);  // パルス幅指定で安定
  currentAngle = restAngle;            // 現在角度を待機位置として扱う
  servo.write(currentAngle);
  delay(500);

  // Wi-Fi 接続
  Serial.println();
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  // ← このアドレスをスマホで開く

  // Webサーバのルーティング
  server.on("/", handleRoot);
  server.on("/press", handlePress);  // 押すだけのエンドポイント

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
