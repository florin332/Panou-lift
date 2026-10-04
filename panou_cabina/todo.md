# Palier Duplex Panel - Local Development TODO List

## 1. Asynchronous USB Client Parser
- [ ] Expand string tokenization inside `processCommand()` using `indexOf()` and `substring()` to split parameters safely.
- [ ] Fix the boundary offset index configuration for `mb_display_reinit` to avoid invalid look-ahead memory reads.
- [ ] Implement direct automated ASCII feedback transmissions: `ACK SRV ENTER`, `ACK SRV EXIT`, and global `ERR` strings.

## 2. Dual TFT Graphic Rendering Layers
- [ ] Code the service layout frame overlay to lock out standard display rendering when `mb_in` status is active.
- [ ] Map the tokenized data array from the `mb_com` response payload directly into separate Left/Right text layout objects.
- [ ] Create the fullscreen visual state switcher for the isolated **"PASS"** graphic testing pattern (`mb_display_test`).
- [ ] Fill the low-level hardware controller reinitialization placeholder inside the display reset branch.

## 3. Background RS485 Processing
- [ ] Secure dual-channel asynchronous packet receiving via `Serial1` (Lift 2) and `Serial2` (Lift 1) inside `Protocol.cpp`.
- [ ] Optimize the `parsePacket()` validator structure to translate raw packets safely into `LiftState` memory spaces.
