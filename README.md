🔐 STM32 智慧門鎖系統 (Smart Door Lock System)
本專案使用 STM32F407 開發板實作一套智慧門鎖系統，整合 RFID、Keypad、LCD 顯示與藍牙通訊，並透過狀態機（State Machine）設計，實現多模組協同運作與安全機制。

---

## 📌 專案特色

- 🔑 支援雙驗證機制（RFID / PIN 任一即可解鎖）
- 📺 LCD 顯示即時系統狀態
- 🔐 錯誤次數限制與 Lockout 機制
- ⏱ Auto Lock（自動上鎖）
- 📡 藍牙即時事件回傳（HC-05）
- 🧠 使用 State Machine 管理系統流程
- 🚫 採用 Non-blocking 設計（避免使用 delay）

---

## 🧩 系統架構
            +------------------+
            |     main.c       |
            |  (State Machine) |
            +------------------+
               /   |    |    \
              /    |    |     \
             /     |    |      \
    +--------+  +-------+  +--------+  +-----------+
    | rc522  |  | keypad|  |  auth  |  | bluetooth |
    +--------+  +-------+  +--------+  +-----------+
          \        |          |           /
           \       |          |          /
            \      |          |         /
             +--------------------------+
             |        lcd_i2c           |
             +--------------------------+


---

## ⚙️ 使用硬體

| 元件 | 說明 |
|------|------|
| STM32F407G-DISC1 | 主控制器 |
| RC522 | RFID 模組（SPI） |
| 1602 LCD + I2C | 顯示模組 |
| 4x4 Keypad | 密碼輸入 |
| HC-05 | 藍牙模組（UART） |
| LED | 模擬門鎖 |

---

## 🔌 接線說明

### RFID（RC522）

| RC522 | STM32 |
|------|------|
| SDA | PA4 |
| SCK | PA5 |
| MOSI | PA7 |
| MISO | PA6 |
| RST | PB0 |
| VCC | 3.3V |
| GND | GND |

---

### LCD（I2C）

| LCD | STM32 |
|-----|------|
| SDA | PB9 |
| SCL | PB8 |
| VCC | 5V |
| GND | GND |

---

### HC-05（藍牙）

| HC-05 | STM32 |
|-------|------|
| RXD | PA2 |
| TXD | PA3 |
| VCC | 5V |
| GND | GND |

---

### Keypad

- Rows：PC7 ~ PC4（Output）
- Columns：PC3 ~ PC0（Input + Pull-up）

---

## 🔄 系統狀態（State Machine）

```c
typedef enum
{
    SYS_STATE_IDLE = 0,
    SYS_STATE_UNLOCKED,
    SYS_STATE_DENIED,
    SYS_STATE_LOCKOUT
} SystemState_t;

