# lump_comm

`lump_comm` は、LEGO SPIKE HubとLUMP UART Message Protocolで通信するためのESP-IDFコンポーネントです。

## 役割

このコンポーネントはセンサー固有の処理ではなく、通信トランスポート層を担当します。

- LUMPハンドシェイクと接続管理
- 同期・ハンドシェイク時の低速ビットバンギング通信
- 高速UARTによるデータ通信
- センサーデータのバッファリングとラウンドロビン送信
- 受信コマンドのキュー管理
- LUMPメッセージ生成とチェックサム計算

センサー固有の処理は別コンポーネントの `lump_comm_sensors` が担当します。

## ディレクトリ構成

```text
lump_comm/
├── include/
│   ├── lump_comm.h
│   └── lump_command.h
├── src/
│   ├── lump_comm.c
│   ├── lump_message.c/.h
│   ├── lump_bitbang.c/.h
│   ├── lump_slots.c/.h
│   └── lump_protocol.h
├── Kconfig
├── CMakeLists.txt
└── idf_component.yml
```

## 設定

`idf.py menuconfig` の **Component config -> LUMP Communication** から設定します。

| 項目 | デフォルト | 説明 |
|---|---:|---|
| `LUMP_TX_GPIO` | 8 | TX GPIO |
| `LUMP_RX_GPIO` | 44 | RX GPIO |

公開ヘッダーでは、このKconfig値を `LUMP_GPIO_TX` / `LUMP_GPIO_RX` として利用します。

## 初期化

通信タスクは1回だけ起動します。

```c
void app_main(void)
{
    lump_device_start();
}
```

以降のハンドシェイクや通信状態遷移はバックグラウンドで処理されます。通常、アプリケーション側でハンドシェイクを直接管理する必要はありません。

## センサーデータ送信

センサー種別とセンサーインスタンスの最新値を `lump_device_report()` で通知します。

```c
lump_device_report(
    LUMP_TYPE_1,
    1,
    0,
    value1,
    value2,
    value3,
    value4
);
```

`lump_sensor_type_t` がセンサー分類を、`mode` がその分類内の4つの16bit値の意味を表します。

内部のスロット層ではセンサー種別ごとに独立した保存領域を持ち、更新されたスロットをラウンドロビン順で送信します。

## 接続状態

```c
if (lump_device_is_connected()) {
    // SPIKEとのDATAフェーズが確立しています。
}
```

## 受信コマンド

コマンドモジュールは、未処理コマンドを最大 `LUMP_COMMAND_QUEUE_CAPACITY` (=16) 件リングバッファに保持します。`lump_command_push()` が受信パケットを解析してキューへ追加し、`lump_command_pop()` が最も古いものから取り出します。

上位の `lump_comm_sensors` は、このキューをコマンドディスパッチ機構経由で利用します。

## プロトコル層

内部ヘッダーは責務ごとに分かれています。

- `lump_protocol.h` : プロトコル定数とメッセージフィールド値
- `lump_message.h` : メッセージ組み立て、ペイロード長、チェックサム
- `lump_bitbang.h` : 低速GPIO通信
- `lump_slots.h` : センサー種別ごとの送信スロット
- `lump_command.h` : 受信コマンドキュー

現在の通信設定では同期フェーズに2400bps、通常のデータフェーズに115200bpsを使用します。

## 依存コンポーネント

- `esp_driver_gpio`
- `esp_driver_uart`
- `freertos`

## 公開API

- `lump_device_start()`
- `lump_device_is_connected()`
- `lump_device_report()`
- `lump_command_init()`
- `lump_command_push()`
- `lump_command_pop()`
- `lump_command_count()`

コマンドAPIは、上位のセンサー層が利用するため公開されています。

## ライセンス

コンポーネントにはMIT Licenseファイルが含まれています。
