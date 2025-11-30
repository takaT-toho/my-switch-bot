ESP32-DevKitC-32E と SG90 で “自作 SwitchBot” みたいなやつを作る手順を一気にまとめます。

---

## 0. 全体像

* ESP32 が Wi-Fi に接続して Webサーバになる
* スマホのブラウザから ON / OFF ボタンを押す
* ESP32 が SG90 サーボを回して、物理的に壁スイッチをカチッと押す / 入切する

**重要な注意**
家庭の AC100V 配線に直接触るのは絶対 NG です。
あくまで「外側から指で押す」のと同じように、サーボでスイッチを動かすだけにしてください。

---

## 1. 必要なもの

### 電子部品

* ESP32-DevKitC-32E 本体
* SG90 マイクロサーボ（3線：茶/赤/橙が多い）
* 5V 電源

  * 例：USB アダプタ + USB → 5V 出力ケーブル
  * 電流は最低 1A くらい（余裕ある方が安心）
* ブレッドボード（あれば便利）
* ジャンパワイヤ (オス-オス、オス-メス)
* Micro USB ケーブル（ESP32 と PC 接続用）

### 固定用（機構）

* 両面テープ / 強力テープ
* サーボホーン（SG90 付属の白いプラスチックの腕）
* スイッチの近くにサーボを固定するための台

  * 厚紙 / アクリル板 / 3Dプリンタ/ レゴなど、なんでも OK

---

## 2. 配線（電気的な接続）

SG90 の配線色はだいたい次の意味です（念のためサーボのラベルも確認してください）：

* **茶**：GND
* **赤**：5V
* **橙**：信号線（PWM）

### 推奨構成：サーボは外部 5V、ESP32 と GND 共有

1. 外部 5V 電源 → サーボの 赤線 (VCC)
2. 外部 5V 電源 → GND を ESP32 の GND にもつなぐ（**共通 GND**）
3. ESP32 の GND → サーボの 茶線 (GND)
4. ESP32 の GPIO 18 (例) → サーボの 橙線 (信号)

> ESP32 の 3.3V ピンからサーボに電源を取るのは **NG**。
> サーボ起動時の電流で電圧が落ちて、ESP32 がリセットしがちです。

---

## 3. 機構設計（スイッチにどう触るか）

ここは現物合わせになりますが、方針だけ。

### トグルスイッチ（上下にパチンと動かすタイプ）

* サーボホーンをスイッチレバーに当たる位置に固定
* 角度 A：スイッチ OFF 側
* 角度 B：スイッチ ON 側
  → サーボを A ↔ B に回すことで ON / OFF

### プッシュボタン型スイッチ

* サーボホーンの先に、小さな板やゴムをつけて、ボタンを押せるようにする
* 通常角度：ボタンから離れた位置
* 押す角度：ボタンを押し込む位置（→ 0.3 秒くらい待って戻す）

**ポイント**

* 壁やスイッチ本体に、サーボの「固定具」を両面テープなどでがっちり固定
* 最初はスイッチを壊さないよう、弱めの角度から試して調整する

---

## 4. 開発環境の準備（Arduino IDE）

1. Arduino IDE をインストール
2. ボードマネージャで「esp32 by Espressif Systems」をインストール
3. ツール → ボード → 「ESP32 Dev Module」または「ESP32-DevKitC」系を選択
4. シリアルポートに ESP32 を選択

### ライブラリ

* 「ESP32Servo」ライブラリを入れておくと楽です
  Arduino IDE → スケッチ → ライブラリを管理 → `ESP32Servo` を検索・インストール

---

## 5. ESP32 のスケッチ例（Webサーバ + サーボ制御）

### 5.1 SSID / パスワードを書き換え

下のコードの `const char* ssid = "YOUR_SSID";` と
`const char* password = "YOUR_PASSWORD";` を自分の Wi-Fi に変更してください。

### 5.2 コード全体

```cpp
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char* ssid     = "YOUR_SSID";       // ← Wi-Fi SSID
const char* password = "YOUR_PASSWORD";   // ← Wi-Fi パスワード

WebServer server(80);
Servo servo;

// サーボ信号線のピン (ESP32 の GPIO 番号)
const int SERVO_PIN = 18;

// スイッチ用の角度（仮値。あとで調整）
int angleOff = 40;   // スイッチOFF側
int angleOn  = 130;  // スイッチON側

// シンプルなWebページ
const char MAIN_page[] PROGMEM = R"=====( 
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP32 Switch Controller</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; text-align: center; margin-top: 40px; }
    button {
      font-size: 24px; padding: 20px 40px; margin: 10px;
      border-radius: 8px; border: none; cursor: pointer;
    }
    .on  { background: #4CAF50; color: white; }
    .off { background: #f44336; color: white; }
  </style>
</head>
<body>
  <h1>Switch Controller</h1>
  <p>タップしてスイッチを操作</p>
  <button class="on"  onclick="sendCmd('on')">ON</button>
  <button class="off" onclick="sendCmd('off')">OFF</button>

  <script>
    function sendCmd(cmd) {
      fetch('/' + cmd)
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

void handleOn() {
  // スイッチをON側に倒す（または押す）
  servo.write(angleOn);
  delay(500); // 0.5秒待つ（必要に応じて調整）

  server.send(200, "text/plain", "ON ok");
}

void handleOff() {
  // スイッチをOFF側に倒す
  servo.write(angleOff);
  delay(500);

  server.send(200, "text/plain", "OFF ok");
}

void setup() {
  Serial.begin(115200);

  // サーボ初期化
  Servo::attach(SERVO_PIN);
  servo.write(angleOff);      // 初期位置をOFF側にしておく
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
  server.on("/on", handleOn);
  server.on("/off", handleOff);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
```

> ※ `Servo::attach(SERVO_PIN);` がうまくいかない場合は、
> `servo.attach(SERVO_PIN, 500, 2400);`（最小/最大パルス幅を指定）
> のように変更してください。

---

## 6. 実際の動作確認（スマホから操作）

1. ESP32 を PC に USB 接続してスケッチを書き込み
2. シリアルモニタ (115200bps) を開く
3. `WiFi connected` のあとに表示される `IP address: 192.168.x.x` をメモ
4. スマホを同じ Wi-Fi に接続
5. スマホのブラウザで `http://192.168.x.x/` を開く
6. 画面の **ON** / **OFF** ボタンをタップ
   → サーボが動いてスイッチを操作する

---

## 7. 調整ポイント

* **角度調整**

  * スイッチが壊れない範囲で、`angleOn` と `angleOff` を少しずつ変えてフィットさせる
  * ボタン式なら「一瞬押して戻す」動きにする
* **サーボのトルク不足**

  * SG90 で足りない場合はトルクの高いサーボに変更
* **Wi-Fi が無い環境で使いたい場合**

  * ESP32 を AP モードにして、ESP32 自身が Wi-Fi を飛ばす構成にすることも可能

---

## 8. 次のステップ案

* ON/OFF の状態を Web に表示する（簡易状態管理）
* パスワード付きのページにする
* Home Assistant や MQTT と連携させる
