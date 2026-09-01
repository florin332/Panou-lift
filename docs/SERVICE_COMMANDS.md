# USB CDC ASCII Communication Protocol Specification: Service Box ↔ Display Panels

This document defines exclusively the text-based ASCII transport layer, syntax constraints, and command register for communication over USB. UI navigation states, screens, and localized menus are documented in each module's respective `service_menu.md` file.

## 1. Physical Layer & Topology
* **Interface:** Native USB Type-C (USB CDC - Virtual COM Port).
* **Configuration:** `115200` baud, 8 data bits, No parity, 1 stop bit (`115200 8N1`).
* **Format:** Raw Human-Readable **ASCII Text**. This enables direct manual interfacing using any standard PC serial terminal emulator (e.g., PuTTY, Serial Monitor).
* **Topology:** **Master-Slave (Host-Client)**.
  * **HOST (Master):** `Service Box`. Automatically initiates and orchestrates all transaction loops.
  * **CLIENT (Slave):** `Palier Panel`. Listens passively on the USB interface and responds only when directly queried by the Host.

---

## 2. Framing Syntax & Serial Constraints
To ensure predictable parsing by the embedded ASCII state-machines, all data packets must adhere to the following rules:

1. **Line Endings:** Every command string sent by the Host must terminate with **Both NL & CR** (`\r\n`).
2. **Transaction Blocking:** The Host must dispatch exactly one command line at a time and await the explicit text response from the Client before streaming the next command.
3. **Internal Parser Offset:** Commands use strict space-separated or exact-match parameters.

### Global Error Responses (ASCII)
If a transaction fails validation or constraint checking, the Client will immediately return one of the following standard strings:
* `ERR 01 UNKNOWN_CMD` : The string sent does not match any register alias.
* `ERR 02 BUSY`        : The hardware parser is processing a previous command.
* `ERR 03 NOT_IN_SERVICE` : The command requires entering Service Mode first via `mb_in`.

---

## 3. Command Register Matrix (ASCII)

### 3.1 Service Mode Control
Used to toggle the display layout between normal operation and the service diagnostic layout.

* **`mb_in`**
  * **Direction:** Host → Client
  * **Description:** Requests entry into Service Mode. Activates the service layout border on the TFT panels.
  * **Expected Response:** `ACK SRV ENTER`

* **`mb_out`**
  * **Direction:** Host → Client
  * **Description:** Exits Service Mode. Reverts the display panels to normal elevator runtime rendering.
  * **Expected Response:** `ACK SRV EXIT`

---

### 3.2 Live Communication Data
Commands in this section require the Client to be in Service Mode (`mb_in` must be called first).

* **`mb_com`**
  * **Direction:** Host → Client
  * **Description:** Commands the panel to extract lift telemetry data snapshots and output them.
  * **Expected Response Format:** `OK mb_com L1=[DATA] L2=[DATA]`
  * *Telemetry Fields:* `Pos` (Position/Floor), `Dst` (Target), `S/J` (Direction: UP/DWN/--), `Svc` (Status: OK/Def./Rev./No ser.), `Ocp` (Occupied: YES/NO).

* **`mb_status`**
  * **Direction:** Host → Client
  * **Description:** Sends/Requests the operational status of both lifts and communication frame packet drops.
  * **Expected Response Example:** `OK STATUS L1=OK L2=No ser. ERR1=0 ERR2=2`

* **`mb_diag`**
  * **Direction:** Host → Client
  * **Description:** Requests the communication error summary, seqlock collision counts, and hardware boot codes.
  * **Expected Response Example:** `OK DIAG ERR1=0 ERR2=0 SEQ=0 RESET=1`

---

### 3.3 Display Output Diagnostics

* **`mb_display_test 1`** or **`mb_display_test 2`**
  * **Direction:** Host → Client
  * **Description:** Commands the targeted TFT panel (1 = Left, 2 = Right) to output a isolated "PASS" verification graphic.
  * **Expected Response:** `ACK` (followed by layout swap)

* **`mb_com_out`**
  * **Direction:** Host → Client
  * **Description:** Terminates the current active display test or telemetry screen overlay, returning to base service layer.
  * **Expected Response:** `ACK mb_com_out`

* **`mb_display_reinit 1`** | **`mb_display_reinit 2`** | **`mb_display_reinit BOTH`**
  * **Direction:** Host → Client
  * **Description:** Forces a hardware reinitialization sequence on the specified TFT controller.
  * **Expected Response:** `OK`

---

### 3.4 Microcontroller (MCU) Performance Telemetry

| Command | Description | Expected Response Example |
| :--- | :--- | :--- |
| **`mb_runtime`** | Requests the elapsed MCU uptime in seconds. | `OK MCU UPTIME 123` |
| **`mb_resets`**  | Requests total boot/reset counts stored in NVRAM. | `OK MCU RESETS 4` |
| **`mb_wdt`**     | Requests current Hardware Watchdog state. | `OK MCU WDT ENABLED` |
| **`mb_lastreset`**| Requests the execution cause flag of the last reset. | `OK MCU LASTRESET POR` |
| **`mb_temp`**    | Requests the internal MCU die temperature in °C. | `OK MCU TEMP 31.4` |
| **`mb_stack 0`** | Requests Core 0 stack tracking (USED/FREE/HIGH WATER). | `OK MCU STACK 0 USED=2048 FREE=4096 HW=8192` |
| **`mb_stack 1`** | Requests Core 1 stack tracking (USED/FREE/HIGH WATER). | `OK MCU STACK 1 USED=2048 FREE=4096 HW=8192` |

---

## 4. Host Arbitration Timeout & Failure Recovery
1. **Response Timeout Window:** Because USB CDC runs virtual pipelines at full native controller bus speed, the Host enforces a maximum waiting window of **150ms** per command string.
2. **Retry Protocol Budget:** If an expected response header (`OK` or `ACK`) is missing or a timeout occurs, the Host will retransmit the exact ASCII sequence up to **3 consecutive times**. If all drop, the Host triggers an *"Error: Panel Comm Failure"* notification across the dashboard.

