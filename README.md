# HK Watches Custom Firmware & Hack Toolkit 🚀

Welcome! This repository contains a beginner-friendly custom firmware toolkit and reverse-engineering guide for **HK Smartwatches** powered by SiFli microcontrollers (such as **SF32LB525**).

---

## 📱 Supported Watch Models

| Model | Chip / MCU | Main Flash Partition | Screen Resolution |
| :--- | :--- | :--- | :--- |
| **HK 10 Pro Max+** | SiFli SF32LB525 (240 MHz) | 3.5 MB Code Region | 410 × 494 AMOLED |
| **HK 10 Ultra 3** | SiFli SF32LB525 (240 MHz) | 3.5 MB Code Region | 410 × 494 AMOLED |
| **HK 9 Ultra 2** | SiFli SF32LB525 (240 MHz) | 3.5 MB Code Region | 320 × 386 AMOLED |
| **HK 9 Mini+** | SiFli SF32LB525 / SF32LB551 | 3.5 MB Code Region | 320 × 386 AMOLED |

---

## ⚡ Key Features & Embedded Apps

This custom firmware includes super-optimized built-in applications:

- ⌚ **Watchface & Interactive Clock**
- 🌀 **Interactive 3D Main Menu**
- 🧮 **Calculator App** (Fast, lightweight grid calculator)
- ⏱️ **Stopwatch App** (Millisecond precision timer with Start/Pause/Reset)
- ⚙️ **Settings & Device Info** (Real-time RAM usage, MCU stats & system info)
- 🎞️ **Custom Media / GIF Demos**

> 💡 **Memory Optimization:** HK watches have strict memory limits. The firmware is optimized to remain under **3.5 MB** to fit safely within the SF32LB525 code partition!

---

## 🛠️ Step-by-Step Setup Guide for Beginners

### Step 1: Install the SiFli SDK (Windows)

1. Download and install the official SiFli SDK following the official guide:
   👉 [SiFli SDK Installation Guide](https://docs.sifli.com/projects/sdk/latest/sf32lb52x/quickstart/install/script/windows.html)
2. After installation, always launch the **"SiFli-SDK Environment"** terminal. This automatically sets up your build paths and `scons` toolchain.

---

### Step 2: Choose and Build Your Firmware

Open the **SiFli-SDK Environment** terminal and navigate to your target model's project directory:

#### For HK 10 Pro Max+:
```bash
cd projects\hk10promaxplus_firmware\v1\watch\project
scons --board=eh-lb525 -j8
```

#### For HK 10 Ultra 3:
```bash
cd projects\hk10ultra3_firmware\v1\watch\project
scons --board=eh-lb525 -j8
```

#### For HK 9 Ultra 2:
```bash
cd projects\hk9ultra2_firmware\v1\watch\project
scons --board=eh-lb525 -j8
```

*(Note: `-j8` compiles using 8 CPU threads. Adjust based on your computer.)*

Build output binaries will be generated inside `build_eh-lb525_hcpu\`.

---

### Step 3: Configure Settings (Optional Menuconfig)

You can customize features, LCD drivers, and debugging flags using the visual menu:

```bash
scons --menuconfig --board=eh-lb525
```
* Use **Up/Down Arrow** keys to navigate.
* Press **Enter** to open submenus and **Space** to toggle options.
* Save and exit when done.

---

### Step 4: Hardware Wiring & Flashing via UART

#### 1. Hardware Pinout
Connect your USB-to-UART Adapter (CH340, FTDI, CP2102) to the watch debug pads:

| Watch Debug Pad | USB-UART Adapter Pin | Description |
| :--- | :--- | :--- |
| **DBG_TX0** | **RX** | Transmit line |
| **DBG_RX0** | **TX** | Receive line |
| **GND** | **GND** | Common Ground |

⚠️ **IMPORTANT SAFETY WARNING:**
* Do **NOT** connect 5V or power pins from the USB adapter to the watch!
* Use 3.3V logic levels (or a 3.3V level shifter).

#### 2. Flashing Command
In your project directory (e.g., `projects\hk10promaxplus_firmware\v1\watch\project`), run:

```bash
build_eh-lb525_hcpu\uart_download.bat
```
Select your adapter's COM port (e.g. `COM3` or `3`) when prompted.

---

## ❓ Troubleshooting & FAQs

- **"scons: command not found" or build errors:**
  Make sure you opened the **SiFli-SDK Environment** shell rather than standard Command Prompt or PowerShell.
- **Flashing failed / Device not found:**
  - Verify TX and RX connections (try swapping TX ↔ RX).
  - Ensure GND is securely connected.
  - Make sure no other program (like Serial Monitor or PuTTY) is using the COM port.
- **Firmware size error:**
  If your firmware binary exceeds ~3.5 MB, check if extra large images or heavy assets were added.

---

## 🤝 Contributing & License

Feel free to submit Pull Requests or open Issues for new apps and improvements!
This project is released under the **Apache License 2.0**.
