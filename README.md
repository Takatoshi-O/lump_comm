For Japanese document please see README_ja.md. 日本語版は，README_ja.md を参照
# LUMP Communication Library

LUMP Communication Library is an ESP-IDF component that implements communication between an ESP32 and a LEGO® SPIKE hub using the LEGO UART Message Protocol (LUMP).

The library handles the communication protocol, handshake, connection management, and packet transmission so that application code only needs to report sensor values.

---

## Features

* Implements the LUMP communication protocol
* Automatic handshake and connection management
* Background communication task
* Simple API for reporting sensor data
* Supports multiple sensor types and modes
* Thread-safe sensor data update

---

## Requirements

* ESP-IDF 6.0.1 or later
* ESP32 series MCU
* UART connection to a LEGO SPIKE hub

---

## Component Structure

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

## Configuration

Run

```bash
idf.py menuconfig
```

and navigate to

```
Component config
    → LUMP Communication
```

You can configure

- TX GPIO
- RX GPIO

Modify these values to match your hardware configuration.

---

## Initialization

Start the communication task once from `app_main()`.

```c
void app_main(void)
{
    lump_device_start();
}
```

The library creates its own background task that manages:

* Handshake
* Device initialization
* Connection monitoring
* Data transmission

---

## Sending Sensor Data

Sensor tasks should periodically report values using:

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

### Parameters

| Parameter         | Description                        |
| ----------------- | ---------------------------------- |
| `type`            | Sensor type (`lump_sensor_type_t`) |
| `mode`            | Sensor mode (0–31)                 |
| `sensorID`        | Sensor identifier                  |
| `value1`–`value4` | Four signed 16-bit sensor values   |

Each sensor type has its own storage area, so reporting one sensor does not overwrite data from another sensor type.

---

## Receiving Commands

Commands sent from the SPIKE hub can be obtained with:

```c
uint8_t cmd[LUMP_PAYLOAD_LEN];

lump_device_get_command(cmd);
```

`cmd` must point to a buffer of `LUMP_PAYLOAD_LEN` bytes.

---

## Checking Connection Status

```c
if (lump_device_is_connected()) {
    // Device is connected
}
```

---

## Sensor Types

The library currently defines the following sensor categories.

| Type          | Description         |
| ------------- | ------------------- |
| `LUMP_SYSTEM` | System messages     |
| `LUMP_TYPE_1` | User-defined |
| `LUMP_TYPE_2` | User-defined |
| `LUMP_TYPE_3` | User-defined |
| `LUMP_TYPE_4` | User-defined |
| `LUMP_TYPE_5` | User-defined |
| `LUMP_TYPE_6` | User-defined |
| `LUMP_TYPE_7` | User-defined |

Applications are free to assign these sensor types as needed.

---

## Public API

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

## License

This project is released under the MIT License.
