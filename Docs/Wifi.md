# WiFi Architecture Overview

This document describes the WiFi architecture for the SmartFuseBox project, including the persistent connection model and component responsibilities.

## Architecture Components

The WiFi subsystem consists of three primary components:

### 1. WifiController
**Responsibility**: High-level WiFi lifecycle management and configuration coordination

- Manages WiFi enable/disable state
- Validates and applies configuration from `Config` struct
- Owns the `WifiServer` instance lifecycle
- Registers network command handlers and JSON status visitors
- Integrates with `WarningManager` for error reporting

**Key Methods**:
- `setEnabled(bool)`: Enable or disable WiFi functionality
- `applyConfig(const Config*)`: Apply WiFi settings from persistent configuration
- `update()`: Called from main loop to process WiFi tasks
- `registerHandlers()`: Register HTTP endpoint handlers
- `registerJsonVisitors()`: Register status data providers

### 2. WifiServer
**Responsibility**: Network connection management and HTTP request processing

- Manages WiFi connection state machine (Client and Access Point modes)
- Implements persistent HTTP connection handling
- Routes incoming HTTP requests to appropriate handlers
- Monitors connection health and signal strength (RSSI)
- Implements automatic reconnection with exponential backoff

**Operating Modes**:
- **Access Point (AP)**: Creates a WiFi network others can join
- **Client Mode**: Connects to an existing WiFi network (supports persistent connections)

**Key Methods**:
- `setAccessPointMode()`: Configure as WiFi access point
- `setClientMode()`: Configure as WiFi client
- `begin()`: Initialize WiFi and start connection
- `update()`: Process connection state and HTTP requests
- `isConnected()`: Check current connection status

### 3. Network Command Handlers
**Responsibility**: Process specific HTTP API endpoints

Handlers implement `INetworkCommandHandler` interface and provide:
- Route pattern (e.g., `/api/relay`, `/api/sound`)
- Request processing logic
- JSON response formatting
- Status data serialization via `JsonVisitor`

**Available Handlers**:
- `RelayNetworkHandler`: Controls relay states (`/api/relay`)
- `SoundNetworkHandler`: Manages sound/horn functions (`/api/sound`)
- `WarningNetworkHandler`: Warning system access (`/api/warning`)
- `SystemNetworkHandler`: System status and heartbeat (`/api/system`)

## Persistent Connection Architecture

### Why Persistent Connections?

Traditional HTTP connections open and close for each request, which is inefficient for embedded systems with limited resources. The SmartFuseBox implements persistent connections to:

1. **Reduce overhead**: Eliminate TCP handshake for every request
2. **Improve responsiveness**: Keep connection ready for rapid command sequences
3. **Lower memory usage**: Reuse buffers and connection state
4. **Better reliability**: Maintain stable connection during multi-step operations

### Connection Lifecycle

### Client Handling States

The `WifiServer` maintains a single `ActiveClient` structure with four states:

#### 1. Idle
- Waits for new client connection
- No active HTTP session
- Transitions to `ReadingRequest` when client connects

#### 2. ReadingRequest
- Reads incoming HTTP request headers (non-blocking)
- Enforces read timeout (5 seconds)
- Rejects concurrent connections with HTTP 503
- Transitions to `ProcessingRequest` when headers complete (`\r\n\r\n`)

#### 3. ProcessingRequest
- Parses request (method, path, query parameters)
- Dispatches to appropriate handler
- Sends HTTP response
- Checks `X-Connection-Type: persistent` header
- Transitions to `Idle` (transient) or `KeepAlive` (persistent)

#### 4. KeepAlive
- Maintains connection for 30 seconds of inactivity
- Monitors for new requests on same connection
- Rejects concurrent connections with HTTP 503
- Transitions to `ReadingRequest` on new data or `Idle` on timeout

### Request Header for Persistent Connection

Clients must include the custom header to request persistent connection:

````````

### Response Headers

**Persistent Connection**:

````````

**Transient Connection**:

````````


## Connection State Machine (Client Mode)

When operating as a WiFi client, the server manages connection health:

### Connection Management Features

#### Automatic Reconnection
- Monitors connection status every 500ms when connecting
- Attempts reconnection after 10 seconds (normal)
- Uses 60-second backoff after 3+ consecutive failures
- Cleans WiFi state (`WiFi.disconnect()`) before reconnecting

#### Signal Strength Monitoring
- Checks RSSI every 5 seconds when connected
- Raises `WeakWifiSignal` warning if RSSI < -70 dBm
- Provides early warning before connection drops

#### HTTP Server Lifecycle
- Starts HTTP server only when connected
- Stops server on disconnect to clean TCP state
- Prevents request processing during reconnection

## Configuration Requirements

### Config Structure Fields

````````

### Validation Rules

The `WifiController::isConfigValid()` enforces:

1. ✅ SSID must not be empty
2. ✅ Port must be non-zero
3. ✅ Password can be empty (open networks in AP mode)

Invalid configuration triggers `WifiInvalidConfig` warning and disables WiFi.

## Integration with Main Loop

The WiFi system integrates into the main loop via `WifiController::update()`:

**What `update()` does**:
1. Checks connection state (Client mode reconnection logic)
2. Monitors RSSI (signal strength warnings)
3. Processes HTTP client requests (state machine)
4. Enforces timeouts (read, persistent connection)

## HTTP API Structure

### Endpoint Pattern
All API endpoints follow: `/api/{handler}/{command}?param1=value1&param2=value2`

**Example**: `/api/relay/R2?state=1` (Turn on relay 2)

### Status Endpoint
`GET /api/index` returns aggregated JSON status from all registered `JsonVisitor` components:

````````

## Concurrency and Resource Limits

### Single Client Model
The server handles **one HTTP client at a time** to conserve memory on embedded systems:

- New connections are rejected with HTTP 503 while processing
- Persistent connections block new clients during keep-alive
- Prevents buffer exhaustion and TCP state overflow

### Memory Constraints
- Request buffer: 1024 bytes max
- Response buffer: 512 bytes max
- Single `ActiveClient` structure (no dynamic allocation during requests)

## Error Handling and Warnings

The WiFi system integrates with `WarningManager` for diagnostics:

| Warning | Trigger | Resolution |
|---------|---------|------------|
| `WifiInvalidConfig` | Invalid SSID/port in config | Check configuration values |
| `WifiInitFailed` | Cannot create WifiServer | Check hardware/memory |
| `WeakWifiSignal` | RSSI < -70 dBm | Move closer to access point |

## Best Practices for Clients

### For Persistent Connections
1. ✅ Include `X-Connection-Type: persistent` header
2. ✅ Reuse connection for multiple requests
3. ✅ Close connection after idle period (< 30s timeout)
4. ✅ Handle HTTP 503 gracefully (server busy)

### For Transient Connections
1. ✅ Keep requests small and fast
2. ✅ Don't rely on connection state between requests
3. ✅ Implement retry logic for transient network errors

### General Guidelines
1. ✅ Parse `Connection` header in responses
2. ✅ Respect `Keep-Alive: timeout` value
3. ✅ Limit concurrent requests to 1
4. ✅ Use `/api/index` for efficient status polling

## Future Enhancements

Potential improvements for the WiFi architecture:

- **WebSocket support**: True bidirectional communication for real-time updates
- **mDNS/Bonjour**: Automatic device discovery without hardcoded IPs
- **TLS/HTTPS**: Encrypted communication for security
- **Multiple client queuing**: FIFO request queue instead of immediate rejection
- **Streaming responses**: Support for large datasets (sensor history)
- **OTA updates**: Over-the-air firmware updates via WiFi

---

*Last Updated: December 7, 2025*
