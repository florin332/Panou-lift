# Service Box - UI Application & Master Sequences

This document defines the local graphical layer navigation and touch input actions of the Service Box Master module.

## 1. Automated Pipeline Sequencing
When the technician clicks the global hardware profile test interface, the system loops through the protocol registry sequentially. For individual step timing limits and error retransmission counts, see `docs/SERVICE_COMMANDS.md`.

## 2. Touch Screen Menu Layouts

### 2.1 Handshake Layer
* **Trigger Event:** Opening the enclosure switch.
* **Master Logic:** Fires `mb_in` to the client. The UI remains frozen on a loading state until `ACK SRV ENTER` unlocks the sub-menus.

### 2.2 Telemetry Monitoring Screen
* **Button [Read Data]:** Asynchronously fires `mb_com`. Parsed tokens are printed into local text widgets on the panel.
* **Button [Status Info]:** Fires `mb_status` to fetch bus state values.
* **Button [Reset Logs]:** Fires `mb_diag` to track crash contexts.

### 2.3 Hardware Isolation Actions
* **Toggle [TFT 1 Test]:** Automatically dispatches `mb_com_out` and switches execution to `mb_display_test 1`.
* **Toggle [TFT 2 Test]:** Dispatches `mb_com_out` and switches execution to `mb_display_test 2`.
* **Button [Force Reinit]:** Dispatches `mb_display_reinit BOTH`.
* **Button [Exit Setup]:** Dispatches `mb_out` and drops the physical interface back to standby.
