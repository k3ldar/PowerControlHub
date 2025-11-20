# SmartFuseBox Setup Guide

This guide covers everything you need to know to successfully check out and build the SmartFuseBox repository on Windows.

---

## 📋 Prerequisites

### Required Software
- **Git for Windows** (with symbolic link support enabled)
- **Arduino IDE** (1.8.19+ or Arduino IDE 2.x)
- **Windows 10/11** (with Developer Mode enabled OR administrator privileges)

### Required Hardware
- **Arduino Mega 2560** (Control Panel)
- **Arduino Uno R4** (Fuse Box)
- **Nextion Display** (3.5" or 5" NX4832T035 recommended)
- See [BOM.md](BOM.md) for complete hardware list

---

## ⚙️ Windows Configuration for Symbolic Links

**IMPORTANT:** This repository uses symbolic links to share code between `BoatControlPanel` and `StaticElectrics` projects. You must configure Windows to handle them properly.

### Option 1: Enable Developer Mode (Recommended)
1. Open **Settings** → **Update & Security** → **For developers**
2. Enable **Developer Mode**
3. Restart your computer
4. Git will now automatically create symbolic links during clone

### Option 2: Use Administrator Privileges
If you cannot enable Developer Mode:
1. Clone the repository normally (symlinks will appear as text files)
2. Run the `SymLinks.bat` scripts manually (see below)

---

## 📥 Cloning the Repository

### With Developer Mode Enabled

Git will automatically create symbolic links during the clone process.

### Without Developer Mode (Admin Required)

After cloning, you **must** manually create symbolic links:

#### For BoatControlPanel:
*(Right-click → "Run as administrator")*

#### For StaticElectrics:
*(Right-click → "Run as administrator")*

---

## 🔧 Git Configuration

### Enable Symlink Support Globally

### Verify Symlinks After Clone

### If Symlinks Are Missing
1. Delete the text files:
2. Run `SymLinks.bat` as administrator:

---

## 📚 Required Arduino Libraries

Install these libraries via **Arduino IDE Library Manager**:

### Third-Party Libraries
- **SerialCommandManager** (by Si Carter)
- **NextionControl** (custom library - see `NextionControl` folder)
- **DHT sensor library** (by Adafruit)
- **QMC5883LCompass** (for magnetic compass sensor)

### Installation
1. Open Arduino IDE
2. Go to **Sketch** → **Include Library** → **Manage Libraries**
3. Search and install each library

### Custom Libraries
If `NextionControl` is not in the Library Manager:
1. Copy `NextionControl` folder to your Arduino `libraries` directory
2. Restart Arduino IDE

---

## 🏗️ Building the Projects

### BoatControlPanel (Arduino Mega 2560)
1. Open `BoatControlPanel/BoatControlPanel.ino` in Arduino IDE
2. Select **Board**: Arduino Mega 2560
3. Select **Port**: (your COM port)
4. Click **Upload**

### StaticElectrics (Arduino Uno R4)
1. Open `StaticElectrics/StaticElectrics.ino` in Arduino IDE
2. Select **Board**: Arduino Uno R4 WiFi (or R4 Minima)
3. Select **Port**: (your COM port)
4. Click **Upload**

---

## 🔍 Verifying Setup

### Check Symlinks
Run this in `BoatControlPanel` or `StaticElectrics`:

You should see entries like:

If you see regular text files, symlinks weren't created correctly.

### Check Arduino Compilation
Both projects should compile without errors. Common issues:

- **Missing library errors**: Install required libraries
- **File not found errors**: Symlinks weren't created properly
- **Multiple definition errors**: Ensure you're not including `.cpp` files

---

## 🚨 Troubleshooting

### Symlinks Appear as Text Files After Clone
**Cause**: Git is not configured to create symlinks, or you lack permissions.

**Fix**:
1. Enable Developer Mode (see above)
2. Re-clone the repository OR run `SymLinks.bat` manually

### "ERROR: Failed to create symlink" When Running SymLinks.bat
**Cause**: Not running with administrator privileges.

**Fix**: Right-click `SymLinks.bat` → **Run as administrator**

### Symlinks Disappear After Git Pull
**Cause**: `core.symlinks` is set to `false`.

**Fix**:

### Files Show as Modified in Git After Creating Symlinks
**Cause**: Git thinks symlinks are regular files.

**Fix**:

---

## 📖 Additional Documentation

- [README.md](README.md) - Project overview and features
- [Commands.md](Commands.md) - Serial command protocol reference
- [BOM.md](BOM.md) - Complete bill of materials
- [ExternalUpdatePattern.md](ExternalUpdatePattern.md) - Display page update architecture

---

## 🤝 Contributing

When making changes to shared files:
1. Edit files in the `Shared/` directory only
2. Do **NOT** commit or modify symlinked files directly
3. Test changes in both `BoatControlPanel` and `StaticElectrics`

---

## 📜 License

GNU General Public License v3.0 (GPLv3) — see [LICENSE](LICENSE) for details.

---

## ❓ Getting Help

If you encounter issues:
1. Check this setup guide thoroughly
2. Review the [README.md](README.md) for project context
3. Open an issue on GitHub with:
   - Your Git configuration (`git config --list | grep symlinks`)
   - Windows version and Developer Mode status
   - Error messages from `SymLinks.bat` or Arduino IDE

---

**Happy building! ⚡🚤**