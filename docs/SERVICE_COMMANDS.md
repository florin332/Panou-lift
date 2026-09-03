# USB CDC ASCII Protocol Specification (Single Source of Truth)

This file specifies exclusively the text-based ASCII transport syntax, pipeline limits, and register maps over native USB CDC.

## 1. Frame Constraints & Global Responses
* **Protocol Core:** Point-to-Point ASCII (Raw text).
* **Line Terminators:** All transmissions must terminate with both Line Feed and Carriage Return (`\r\n`).
* **Execution Block:** Host dispatches one line and blocks further output until the client responds.
* **Global Error Strings:**
  * `ERR 01 UNKNOWN_CMD`   : String mismatch inside the hardware parser register.
  * `ERR 02 BUSY`          : Asynchronous execution pipeline is currently locked.
  * `ERR 03 NOT_IN_SERVICE`: Restricted command fired before evaluating `mb_in`.

## 2. Command Reference Register

### 2.1 State Controls
* **`mb_in`**  (Legacy alias: `SRV ENTER`) -> Expected Response: `ACK SRV ENTER`
* **`mb_out`** (Legacy alias: `SRV EXIT`)  -> Expected Response: `ACK SRV EXIT`

### 2.2 Telemetry Arrays
* **`mb_com`** -> Expected Response Syntax: `OK mb_com L1=[DATA] L2=[DATA]`
  * *Data Vector Mapping:* `Pos` (Floor), `Dst` (Target), `S/J` (Direction), `Svc` (Service Flag), `Ocp` (Occupant Status).
* **`mb_status`** -> Expected Response Example: `OK STATUS L1=OK L2=No ser. ERR1=0 ERR2=2`
* **`mb_diag`**   -> Expected Response Example: `OK DIAG ERR1=0 ERR2=0 SEQ=0 RESET=1`

### 2.3 Peripherals & Core Informational
* **`mb_display_test [1|2]`** (Legacy: `DISP TEST [1|2]`) -> Expected Response: `ACK`
* **`mb_com_out`** (Legacy: `TEST EXIT`) -> Expected Response: `ACK mb_com_out`
* **`mb_display_reinit [1|2|BOTH]`** -> Expected Response: `OK`
* **`mb_runtime`**    -> Expected Response Example: `OK MCU UPTIME 123`
* **`mb_resets`**     -> Expected Response Example: `OK MCU RESETS 4`
* **`mb_wdt`**        -> Expected Response Example: `OK MCU WDT ENABLED`
* **`mb_lastreset`**  -> Expected Response Example: `OK MCU LASTRESET POR`
* **`mb_temp`**       -> Expected Response Example: `OK MCU TEMP 31.4`
* **`mb_stack [0|1]`** -> Expected Response Example: `OK MCU STACK 0 USED=2048 FREE=4096 HW=8192`



