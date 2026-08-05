# LUMP Communication Library

LUMP Communication Library は、ESP32 と LEGO® SPIKE Hub 間の通信を実現する ESP-IDF コンポーネントです。

LEGO UART Message Protocol（LUMP）に基づいた通信プロトコル、ハンドシェイク、接続管理、パケット送信処理をライブラリ側で処理します。

アプリケーション側では、センサー値を通知するだけで SPIKE Hub との通信を利用できます。

---

## 特徴

* LUMP 通信プロトコルを実装
* 自動ハンドシェイクおよび接続管理
* バックグラウンド通信タスクによる動作
* センサーデータ送信用のシンプルな API
* 複数のセンサータイプ・モードに対応
* スレッドセーフなセンサー値更新

---

## 必要環境

* ESP-IDF 6.0.1 以降
* ESP32 シリーズ MCU
* LEGO SPIKE Hub と接続する UART インターフェース

---

## コンポーネント構成

```
lump_comm/
├── include/
│   └── lump_comm.h
├── src/
│   ├── lump_comm.c
│   ├── lump_message.c
│   ├── lump_bitbang.c
│   └── lump_slots.c
├── CMakeLists.txt
├── idf_component.yml
└── README.md
```

---

## 設定

以下のコマンドを実行します。

```bash
idf.py menuconfig
```

次の項目へ移動します。

```
Component config
    → LUMP Communication
```

以下の項目を設定できます。

- TX GPIO
- RX GPIO

使用するハードウェア構成に合わせて GPIO 番号を変更してください。

---

## 初期化

`app_main()` 内で一度だけ通信タスクを開始します。

```c
void app_main(void)
{
    lump_device_start();
}
```

ライブラリは内部でバックグラウンドタスクを生成し、以下の処理を自動的に管理します。

* ハンドシェイク
* デバイス初期化
* 接続状態監視
* センサーデータ送信

---

## センサーデータの送信

センサー処理タスクから、以下の API を使用して定期的に値を通知します。

```c
lump_device_report(
    type,
    mode,
    sensorID,
    value1,
    value2,
    value3,
    value4
);
```

### 引数

| 引数 | 説明 |
| --- | --- |
| `type` | センサータイプ（`lump_sensor_type_t`） |
| `mode` | センサーモード（0～31） |
| `sensorID` | センサー識別番号 |
| `value1`～`value4` | 4つの符号付き16bitセンサー値 |

各センサータイプごとに独立した保存領域を持っています。

そのため、あるセンサータイプの値を更新しても、別のセンサータイプのデータが上書きされることはありません。

---

## コマンド受信

SPIKE Hub から送信されたコマンドは、以下の API で取得できます。

```c
uint8_t cmd[LUMP_PAYLOAD_LEN];

lump_device_get_command(cmd);
```

`cmd` は `LUMP_PAYLOAD_LEN` バイトのバッファを指定してください。

---

## 接続状態の確認

現在 SPIKE Hub と接続されているか確認できます。

```c
if (lump_device_is_connected()) {
    // 接続中
}
```

---

## センサータイプ

現在、以下のセンサータイプを定義しています。

| Type | 説明 |
| --- | --- |
| `LUMP_SYSTEM` | システムメッセージ |
| `LUMP_TYPE_1`～`LUMP_TYPE_7` | ユーザー定義 |

アプリケーションでは用途に応じて自由に割り当てることができます。

---

## 公開 API

```c
void lump_device_start(void);

bool lump_device_is_connected(void);

void lump_device_report(
    lump_sensor_type_t type,
    uint8_t mode,
    uint8_t sensorID,
    int16_t v1,
    int16_t v2,
    int16_t v3,
    int16_t v4
);

void lump_device_get_command(
    uint8_t out[LUMP_PAYLOAD_LEN]
);
```

---

## ライセンス

このプロジェクトは MIT License のもとで公開されています。