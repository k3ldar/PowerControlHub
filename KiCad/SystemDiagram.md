┌──────────────────────────────────────────────────────────────────────┐
│                              12V INPUT                               │
│   • Screw terminal                                                   │
│   • Fuse / polyfuse (40A)                                            │
│   • Reverse polarity protection (diode or MOSFET)                    │
│   • TVS diode                                                        │
└──────────────────────────────────────────────────────────────────────┘

&nbsp;                    │

&nbsp;                    ▼

┌──────────────────────────────────────────────────────────────────────┐
│                             POWER STAGE                              │
│                                                                      │
│  12V\_BUS →→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→→ │
│                                                                      │
│  • Buck converter 12V → 5V                                           │
│       - Inductor, diode (if needed)                                  │
│       - Input/output capacitors                                      │
│       - Feedback resistors                                           │
│                                                                      │
│  • Regulator 5V → 3.3V                                               │
│       - LDO or buck                                                  │
│       - Input/output capacitors                                      │
│                                                                      │
│  Outputs:                                                            │
│       - 12V\_BUS (relays, fan, buzzer)                               │
│       - 5V\_BUS (sensors, LEDs, CAN)                                 │
│       - 3V3\_BUS (ESP32, logic)                                      │
└──────────────────────────────────────────────────────────────────────┘

                          │
                          ▼

┌──────────────────────────────────────────────────────────────────────┐
│                        ESP32-S3 DevKitC-1                            │
│                        (mounted via headers)                         │
│                                                                      │
│  Power: 3V3\_BUS, 5V\_BUS, GND                                       │
│                                                                      │
│  GPIO → Relay Drivers (8 channels)                                   │
│  GPIO → Sensor Ports (6× ports, 2 signals each)                      │
│  CAN\_TX / CAN\_RX → CAN Transceiver                                 │
│  GPIO → Fan Driver                                                   │
│  GPIO → Buzzer Driver                                                │
│  Optional: I2C → Temp Sensor                                         │
└──────────────────────────────────────────────────────────────────────┘

                          │
                          ├───────────────────────────────────────────────┐
                          ▼                                               ▼

┌──────────────────────────────────────┐         ┌─────────────────────────────────────────┐
│          RELAY DRIVER BLOCK          │         │            SENSOR PORTS                 │
│                                      │         │                                         │
│  • R-array (8× 1k)                   │         │  6× identical ports:                    │
│  • ULN2803A (8‑ch driver)            │         │                                         │
│       IN1–IN8 ← ESP32 GPIO           │         │  \[5V]                                  │
│       OUT1–OUT8 → Relay Coils        │         │  \[3.3V]                                │
│       COM → 12V\_BUS                 │         │  \[GND]                                 │
│                                      │         │  \[SIG\_A] → ESP32 (via R + protection) │
│  Optional: LED per channel           │         │  \[SIG\_B] → ESP32 (via R + protection) │
│       5V → 470Ω → LED → ULN OUT      │         │                                         │
└──────────────────────────────────────┘         └─────────────────────────────────────────┘

                          │                                               │
                          ▼                                               ▼

┌──────────────────────────────────────┐         ┌──────────────────────────────────────────┐
│               RELAYS                 │         │               CAN BUS                    │
│         (8× SPDT, 12V coils)         │         │                                          │
│                                      │         │  • CAN Transceiver (e.g., MCP2562)       │
│  Coil1 → ULN OUTx                    │         │  • 120Ω termination (jumper)             │
│  Coil2 → 12V\_BUS                    │         │  • TVS diode                             │
│                                      │         │  • Screw terminal: CAN\_H, CAN\_L, GND   │
│  Contacts:                           │         └──────────────────────────────────────────┘
│     COM → J\_RLx pin 1               │
│     NO  → J\_RLx pin 2               │
│     NC  → J\_RLx pin 3               │
└──────────────────────────────────────┘

                    │
                    ▼

┌──────────────────────────────────────────────────────────────────────┐
│                         MONITORING \& AUX                            │
│                                                                      │
│  • Temperature sensor (I2C or analog)                                │
│  • Fan connector (12V)                                               │
│       - MOSFET / NPN driver                                          │
│       - Flyback diode                                                │
│                                                                      │
│  • Buzzer (5V or 12V)                                                │
│       - Transistor driver                                            │
│       - Series resistor                                              │
└──────────────────────────────────────────────────────────────────────┘

