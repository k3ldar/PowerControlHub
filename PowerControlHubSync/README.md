# SmartFuseSync

A lightweight console utility for applying configuration updates to SmartFuseBox devices over a COM port serial connection.

---

## Commands

Run `SmartFuseSync -?` for a full list of available commands.

### `Config Update` — Apply a configuration file to a device

Reads a `.dat` configuration file and sends each non-blank line to the device one at a time, waiting for an `ACK` or `ERR` response before proceeding to the next command.

| Option | Abbreviation | Required | Description |
|--------|-------------|----------|-------------|
| `filePath` | `-f` | Yes | Path to the `.dat` configuration file |
| `portName` | `-p` | Yes | COM port name (e.g. `COM3`, `COM7`) |
| `baudRate` | `-b` | No | Baud rate (default: `9600`) |

**Examples:**

Using full option names:
```
SmartFuseSync Config Update --filePath:"C:\Configs\MyDevice.dat" --portName:COM7 --baudRate:115200
```

Using abbreviations:
```
SmartFuseSync Config Update -f:"C:\Configs\MyDevice.dat" -p:COM7 -b:115200
```

**Using a `.bat` file (recommended for repeated use):**

Create a `.bat` file with the same base name as your `.dat` file, e.g. `UpdateBasementSensor.bat`:
```bat
SmartFuseSync Config Update -f:"C:\GitProjects\SFB\UpdateBasementSensor.dat" -p:COM7 -b:115200
```

Double-click the `.bat` file or run it from a terminal to apply the configuration.

---

### `Config ListPorts` — List available COM ports

Displays all COM ports currently detected on the system. Useful for identifying the correct port before running an update.

```
SmartFuseSync Config ListPorts
```

---

### `Config Test` — Test connection to a device

Opens the specified COM port and confirms a connection can be established.

| Option | Abbreviation | Required | Description |
|--------|-------------|----------|-------------|
| `portName` | `-p` | Yes | COM port name (e.g. `COM3`, `COM7`) |
| `baudRate` | `-b` | No | Baud rate (default: `9600`) |

```
SmartFuseSync Config Test -p:COM7 -b:115200
```

---

### `Config Read` — Read current configuration from a device

Sends a `READ_CONFIG` request to the device and displays the response. Optionally saves the output to a file.

| Option | Abbreviation | Required | Description |
|--------|-------------|----------|-------------|
| `portName` | `-p` | Yes | COM port name (e.g. `COM3`, `COM7`) |
| `outputPath` | `-o` | No | File path to save the received configuration |
| `baudRate` | `-b` | No | Baud rate (default: `9600`) |

```
SmartFuseSync Config Read -p:COM7 -b:115200
SmartFuseSync Config Read -p:COM7 -b:115200 -o:"C:\Configs\Backup.dat"
```

---

## Configuration file format

Configuration files (`.dat`) contain one command per line. Blank lines are ignored. Each line is sent verbatim to the device.

**Example `UpdateBasementSensor.dat`:**
```
C11:v=1
C12:v=MyNetwork
M0:v=1
M1:v=0
```

The device responds to each command with:
- `ACK:<command>=ok:...` — command accepted
- `ERR:<command>:...` — command rejected (logged as a warning, processing continues)

---

## Serial port settings

All commands use the following fixed serial settings:

| Setting | Value |
|---------|-------|
| Parity | None |
| Data bits | 8 |
| Stop bits | 1 |
| Handshake | None |

The baud rate is configurable via `-b` (default `9600`). Use `-b:115200` for faster devices.
