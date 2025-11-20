# SmartFuseBox ⚡

A modular, Arduino-powered **12V power distribution and control system** designed for boats and other off-grid applications.  
SmartFuseBox combines fused relays, quick-connect aviation plugs, and Arduino-based monitoring into a single 3D-printed enclosure — with a built-in security twist.

---

## 📖 Backstory

I bought a mid-80s boat and wanted to give it a **modern electrical upgrade**.  
The original wiring was dated, messy, and lacked any real security. Since the boat is kept in a marina, theft was also a concern.  

SmartFuseBox was born out of the need for:
- **Reliable 12V power distribution** for navigation lights, bilge pumps, and other onboard systems.  
- **Quick-connect modularity** so I can easily add/remove systems with aviation connectors.  
- **Integrated monitoring** via Arduino sensors (temperature, humidity, light, etc.).  
- **Basic security** through a “poor man’s immobiliser” — the control unit must return a valid 12V feed before the system powers up.  

---

## 🛠 Features

- **8 fused relays** for distributing 12V power safely.  
- **Aviation connectors** for quick, reliable connections to onboard systems.  
- **Arduino integration** for both control and sensor inputs.  
- **Nextion display + Arduino Mega** in the control unit for user interface and system logic.  
- **Immobiliser-style security**:  
  - Main 12V input passes through the control unit.  
  - The control unit must return a valid 12V feed to enable the system.  
- **3D-printed enclosure** for a compact, customizable housing.  

---

## 🔌 System Architecture

### Fuse Box Unit
- 8 × fused relays  
- Arduino (for relay control and sensor handling)  
- Multiple aviation connectors (12V systems + sensor inputs)  
- Connected to control unit via **7-wire harness**:  
  - 12V in  
  - 12V out (return feed)  
  - Ground  
  - 5V out  
  - TX / RX (serial comms)  
  - Spare line  

### Control Unit
- Arduino Mega  
- Nextion touchscreen display  
- Handles user interface, relay control, and immobiliser logic  
- Returns 12V feed to enable the fuse box  

---

## 🚤 Use Cases

- **Marine upgrades**: modernize older boats with modular, safe wiring.  
- **Off-grid projects**: camper vans, RVs, or solar-powered cabins.  
- **DIY automation**: any 12V system needing fused relay control + monitoring.  

---

## 📷 Project Media

*(Add photos of the 3D-printed enclosure, wiring, and display here once available.)*

---

## 📦 Roadmap

- [ ] Add CAN bus or RS485 support for longer cable runs.  
- [ ] Expand sensor integration (GPS, water level, etc.).  
- [ ] Publish STL files for the 3D-printed enclosure.  
- [ ] Provide example Arduino sketches for both units.  

---

## Setup Instructions (Code)
Please read the [SETUP.md](SETUP.md) file for detailed instructions on setting up the Arduino code.

---

## 🤝 Contributing

Contributions, ideas, and improvements are welcome!  
If you’ve built something similar or adapted SmartFuseBox for your own project, feel free to open an issue or share your setup.

---

## 📜 License

GNU General Public License v3.0 (GPLv3) — see [LICENSE](LICENSE) for details.