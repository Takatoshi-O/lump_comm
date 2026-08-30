# lump_comm

`lump_comm` is the transport-layer ESP-IDF component used to communicate with a LEGO SPIKE hub using the LUMP UART message protocol.

## Responsibilities

The component handles the protocol transport rather than sensor-specific behavior:

- LUMP handshake and connection management
- Low-speed bit-banged signaling during synchronization/handshake
- High-speed UART data communication
- Sensor data buffering and round-robin transmission
- Incoming command buffering
- LUMP message construction and checksum calculation

Sensor-specific logic belongs in the separate `lump_comm_sensors` component.

## Directory structure

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

## Configuration

Open `idf.py menuconfig` and select **Component config -> LUMP Communication**.

| Option | Default | Description |
|---|---:|---|
| `LUMP_TX_GPIO` | 8 | TX GPIO |
| `LUMP_RX_GPIO` | 44 | RX GPIO |

The public header converts these Kconfig values to `LUMP_GPIO_TX` and `LUMP_GPIO_RX`.

## Initialization

Start the communication task once:

```c
void app_main(void)
{
    lump_device_start();
}
```

The transport layer then handles the background protocol state machine. Application code normally does not need to manage the handshake directly.

## Reporting sensor data

Use `lump_device_report()` to publish the latest value for a sensor type and sensor instance.

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

`lump_sensor_type_t` separates sensor categories, while `mode` identifies the meaning of the four signed 16-bit values within that category.

The internal slot layer keeps data for each sensor type independently and selects dirty slots in round-robin order for transmission.

## Connection state

```c
if (lump_device_is_connected()) {
    // SPIKE DATA phase is connected.
}
```

## Incoming commands

The command module keeps up to `LUMP_COMMAND_QUEUE_CAPACITY` (=16) pending commands in a ring buffer. `lump_command_push()` parses an incoming fixed-length payload, and `lump_command_pop()` consumes the oldest pending command.

The higher-level `lump_comm_sensors` component uses this queue through its command-dispatch layer.

## Protocol layers

The internal headers separate the implementation into small responsibilities:

- `lump_protocol.h`: protocol constants and message field values
- `lump_message.h`: message packing, payload length, and checksum
- `lump_bitbang.h`: low-speed GPIO signaling
- `lump_slots.h`: sensor-type transmission slots
- `lump_command.h`: incoming command queue

The current transport configuration uses 2400bps during synchronization and 115200bps during the normal run phase.

## Dependencies

- `esp_driver_gpio`
- `esp_driver_uart`
- `freertos`

## Public API

- `lump_device_start()`
- `lump_device_is_connected()`
- `lump_device_report()`
- `lump_command_init()`
- `lump_command_push()`
- `lump_command_pop()`
- `lump_command_count()`

The command APIs are exposed publicly because the sensor layer builds on them.

## License

The component includes an MIT License file.
