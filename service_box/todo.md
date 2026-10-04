# SVC_BOX TODO

This file tracks implementation, verification and remaining work.

It is NOT the authoritative source for:
- hardware mappings;
- UI requirements;
- menu architecture;
- agent working rules.

Authoritative project documents:

- `AGENTS.md` — agent working rules
- `hardware_map.md` — hardware configuration and confirmed hardware mapping
- `ui_requirements.md` — UI functional requirements
- `svcbox_menu.drawio` — menu structure and navigation

Implementation status belongs here.

---

## 1. Documentation / Project Synchronization

### 1.1 Documentation consistency

- [ ] Verify implementation against `AGENTS.md`
- [ ] Verify implementation against `hardware_map.md`
- [ ] Verify implementation against `ui_requirements.md`
- [ ] Verify menu implementation against `svcbox_menu.drawio`
- [ ] Identify obsolete documentation
- [ ] Remove documentation that describes superseded implementation

### 1.2 Documentation conflicts

- [ ] Resolve all known implementation/documentation discrepancies
- [ ] Do not silently modify protected documentation
- [ ] Record unresolved discrepancies in `Agent Proposals`

---

# 2. Hardware Bring-up

## 2.1 Waveshare RP2350

### Display

- [x] LCD / Display — physically tested
- [x] LCD initialization — working
- [ ] Final display integration with UI
- [ ] Long-run display stability

### Touchscreen

- [x] CST328 hardware connection — physically verified
- [x] Touch controller communication — physically verified
- [ ] Touch integration through HAL
- [ ] Touch coordinate validation
- [ ] Touch/display coordinate alignment
- [ ] Touch calibration flow
- [ ] Calibration persistence, if required
- [ ] Long-run touch stability

### SD card

- [ ] Detect SD card
- [ ] Initialize SD
- [ ] Mount filesystem
- [ ] Read directory
- [ ] Read firmware files
- [ ] Handle missing SD
- [ ] Handle unreadable/corrupted SD
- [ ] Long-run SD test

### Battery / charging

- [ ] Battery voltage measurement
- [ ] Battery percentage calculation
- [ ] Charging detection
- [ ] Battery status integration
- [ ] Low-battery handling
- [ ] Long-run battery operation

### RS485

- [ ] Electrical verification
- [ ] Interface initialization
- [ ] RX/TX verification
- [ ] Communication test
- [ ] Timeout verification
- [ ] Error handling

### Combined operation

- [ ] LCD + Touch
- [ ] LCD + SD
- [ ] LCD + RS485
- [ ] Touch + SD
- [ ] Touch + RS485
- [ ] SD + RS485
- [ ] LCD + Touch + SD
- [ ] LCD + Touch + RS485
- [ ] LCD + Touch + SD + RS485
- [ ] Long-run stability

---

## 2.2 Marble Pico

### Display

- [ ] LCD / Display hardware verification
- [ ] Display initialization
- [ ] Display test
- [ ] Long-run display stability

### Touchscreen

- [ ] Touchscreen hardware verification
- [ ] Touch initialization
- [ ] Touch coordinate verification
- [ ] Touch/display alignment
- [ ] Touch calibration

### SD card

- [ ] Detect SD card
- [ ] Initialize SD
- [ ] Mount filesystem
- [ ] Read directory
- [ ] Read firmware files
- [ ] Handle missing SD
- [ ] Handle unreadable/corrupted SD

### Battery / charging

- [ ] Battery voltage measurement
- [ ] Battery percentage
- [ ] Charging detection
- [ ] Low-battery handling

### RS485

- [ ] Electrical verification
- [ ] Interface initialization
- [ ] RX/TX verification
- [ ] Communication test
- [ ] Timeout verification
- [ ] Error handling

### USB / UF2

- [ ] USB connection verification
- [ ] UF2 bootloader detection
- [ ] USB disconnect detection
- [ ] USB reconnect detection
- [ ] Blank/unconfigured target detection

### Combined operation

- [ ] LCD + Touch
- [ ] LCD + SD
- [ ] LCD + RS485
- [ ] Touch + SD
- [ ] Touch + RS485
- [ ] SD + RS485
- [ ] LCD + Touch + SD
- [ ] LCD + Touch + SD + RS485
- [ ] Long-run stability

---

# 3. Hardware Abstraction Layer

## 3.1 Target separation

- [ ] Verify Waveshare-specific implementation isolation
- [ ] Verify Marble-specific implementation isolation
- [ ] Verify common UI does not depend directly on hardware target
- [ ] Verify target-specific code remains inside HAL where appropriate

## 3.2 Display HAL

- [ ] Verify common display interface
- [ ] Verify target-specific display implementation
- [ ] Verify UI does not access display hardware directly
- [ ] Preserve existing working display implementation

## 3.3 Touch HAL

- [ ] Verify common touch interface
- [ ] Verify `updateTouch()`
- [ ] Verify `isScreenTouched()`
- [ ] Verify `getTouchX()`
- [ ] Verify `getTouchY()`
- [ ] Verify target-specific touch implementation
- [ ] Verify UI uses logical touch input
- [ ] Implement calibration through the appropriate HAL layer

## 3.4 SD HAL

- [ ] SD initialization
- [ ] Filesystem access
- [ ] Directory access
- [ ] Firmware file access
- [ ] Error reporting

## 3.5 Battery / power HAL

- [ ] Battery measurement
- [ ] Charging detection
- [ ] Battery state reporting
- [ ] Low-battery state

## 3.6 RS485 HAL

- [ ] RS485 initialization
- [ ] RX/TX handling
- [ ] Timeout handling
- [ ] Communication state
- [ ] Error reporting

## 3.7 USB / UF2 HAL

- [ ] USB state detection
- [ ] UF2 bootloader detection
- [ ] Disconnect/reconnect handling
- [ ] Flash target state reporting

---

# 4. Startup Sequence

Reference: `ui_requirements.md`

## 4.1 BOOT

- [ ] MCU initialization
- [ ] Required hardware initialization
- [ ] Hardware status collection
- [ ] Hardware initialization error handling
- [ ] Startup state management

## 4.2 BATTERY CHECK

- [x] Battery state acquisition (via IBatteryProvider / BatteryStub)
- [x] Battery percentage display
- [x] Battery status indication (LOW/MEDIUM/GOOD colors)
- [x] Charging detection (via IBatteryProvider / BatteryStub)
- [x] Charging display
- [x] Charging hold behaviour
- [x] Normal timeout behaviour
- [x] LOW threshold blocks automatic advance
- [x] MEDIUM/GOOD automatic advance to START after timeout
- [ ] Physical battery measurement verified
- [ ] Physical charging detection verified

## 4.3 CAL PIN / Touch Calibration

- [ ] CAL PIN detection
- [ ] Enter calibration flow
- [ ] Implement approved calibration algorithm
- [ ] Verify calibration points
- [ ] Verify coordinate transformation
- [ ] Verify calibration persistence if required
- [ ] Return to normal startup flow

> Exact calibration algorithm is not to be invented here.
> Use the approved project requirement and hardware implementation.

## 4.4 Panel identification

- [ ] Establish required communication
- [ ] Perform panel identification
- [ ] Obtain panel type
- [ ] Obtain floor state
- [ ] Preserve valid identification result for START
- [ ] Handle unknown panel
- [ ] Handle communication failure

## 4.5 START

- [ ] Implement START screen
- [ ] Display panel information
- [ ] Display floor state
- [ ] Display Service Box state
- [ ] Display SD state
- [ ] Display connection state
- [ ] Use the existing startup handshake result
- [ ] Implement START action according to `ui_requirements.md`

---

# 5. Main Menu / Navigation

Reference: `svcbox_menu.drawio`

## 5.1 Main navigation

- [ ] Implement current menu structure
- [ ] Implement page navigation
- [ ] Implement RETURN / BACK behaviour
- [ ] Implement touch button states
- [ ] Implement disabled states
- [ ] Implement operation-state restrictions
- [ ] Implement error indication

## 5.2 Mode Selection

- [ ] EXIT SERVICE MODE
- [ ] FLASH
- [ ] FLOOR SET

## 5.3 Navigation verification

- [ ] Verify every implemented transition against `svcbox_menu.drawio`
- [ ] Verify every required transition from `ui_requirements.md`
- [ ] Verify no unapproved pages exist
- [ ] Verify no unapproved transitions exist
- [ ] Verify operation pages cannot be exited incorrectly
- [ ] Verify RETURN behaviour on every relevant page

---

# 6. Panel Identification / Handshake

## 6.1 Panel identification

- [ ] Detect panel
- [ ] Identify panel type
- [ ] Read floor state
- [ ] Handle unknown panel
- [ ] Handle identified panel with unset floor
- [ ] Handle identified panel with configured floor

## 6.2 Handshake

- [ ] Startup handshake
- [ ] Post-flash handshake
- [ ] Post-floor-set handshake
- [ ] Verify handshake timeout
- [ ] Verify communication-loss handling
- [ ] Verify stale identification data is never reused

## 6.3 Service Mode exit

- [ ] Return from Mode Selection to START
- [ ] Verify no unnecessary handshake is performed
- [ ] Verify resulting START state

---

# 7. Communication Test

Reference: `ui_requirements.md`

- [ ] Implement Communication Test
- [ ] Verify communication with the panel/system
- [ ] Display communication state
- [ ] Display relevant errors
- [ ] Test no-panel condition
- [ ] Test communication timeout
- [ ] Test invalid communication data
- [ ] Test communication loss
- [ ] Test recovery

> Communication Test is separate from the Service Box ↔ panel identification handshake.

---

# 8. External Panel Display Test

Reference: `ui_requirements.md`

- [ ] Implement external panel display test
- [ ] Display 1 selection
- [ ] Display 2 selection
- [ ] TEST action
- [ ] TESTING state
- [ ] REINIT action
- [ ] Error handling
- [ ] Verify test affects external panel display only

> This is NOT a Service Box LCD test.

---

# 9. Flash Workflow

## 9.1 Panel Type Selection

- [ ] Implement panel type selection
- [ ] DUPLEX
- [ ] SIMPLEX
- [ ] CAB
- [ ] Filter firmware list according to selected target

## 9.2 Firmware selection

- [ ] Read `.uf2` files from SD
- [ ] Detect valid firmware files
- [ ] Display firmware list
- [ ] Select firmware
- [ ] Handle no valid firmware files

## 9.3 Confirm & Flash

- [ ] Display selected firmware
- [ ] Display target information
- [ ] Wrong-target warning
- [ ] Require confirmation
- [ ] Start flashing only after confirmation

## 9.4 Flash execution

- [ ] Detect target
- [ ] Detect UF2 bootloader
- [ ] Execute flash sequence
- [ ] Display flashing state
- [ ] Handle USB disconnect
- [ ] Handle USB reconnect
- [ ] Detect flash failure
- [ ] Report flash error
- [ ] Verify flashing result

## 9.5 Post-flash

- [ ] Reboot target
- [ ] Detect target reconnect
- [ ] Perform mandatory handshake
- [ ] Re-identify panel
- [ ] Do not reuse stale panel information
- [ ] Return to START with current information

---

# 10. Current Floor Set

## 10.1 Entry

- [ ] Enter from START when floor is unset
- [ ] Enter from Mode Selection
- [ ] Verify both entry paths

## 10.2 Floor setting

- [ ] Implement floor selection/input
- [ ] Validate floor value
- [ ] Store floor value
- [ ] Verify persistence
- [ ] Display setting state

## 10.3 Reconnect

- [ ] Handle target reconnect
- [ ] Display reconnect state
- [ ] Perform mandatory handshake
- [ ] Re-identify panel
- [ ] Verify new floor state
- [ ] Return to START

---

# 11. MCU INFO

Reference: `ui_requirements.md`

- [ ] Uptime
- [ ] Reset information
- [ ] Watchdog information
- [ ] MCU temperature
- [ ] Stack information
- [ ] Available diagnostic counters
- [ ] RETURN to Mode Selection
- [ ] Verify informational/diagnostic-only behaviour

---

# 12. Error Handling / Recovery

## 12.1 Hardware errors

- [ ] Display initialization failure
- [ ] Touch initialization failure
- [ ] SD initialization failure
- [ ] Battery measurement failure
- [ ] Charging detection failure
- [ ] RS485 initialization failure
- [ ] USB failure

## 12.2 Communication errors

- [ ] No panel detected
- [ ] Handshake timeout
- [ ] Communication timeout
- [ ] Invalid frame/data
- [ ] Communication loss
- [ ] Recovery after communication loss

## 12.3 Flash errors

- [ ] No target
- [ ] Invalid UF2
- [ ] Wrong target
- [ ] Bootloader failure
- [ ] USB disconnect failure
- [ ] Flash failure
- [ ] Reconnect failure
- [ ] Recovery behaviour

## 12.4 General recovery

- [ ] Verify safe return from recoverable errors
- [ ] Verify stale state is cleared where required
- [ ] Verify no invalid menu transition remains possible
- [ ] Verify reboot/recovery behaviour

---

# 13. PlatformIO / Build

## 13.1 Environments

- [ ] Verify `waveshare_rp2350`
- [ ] Verify `marble_pico`
- [ ] Verify target isolation
- [ ] Verify correct source files per target

## 13.2 Dependencies

- [ ] Verify required libraries
- [ ] Remove obsolete dependencies
- [ ] Verify no duplicate libraries
- [ ] Verify target-specific dependencies
- [ ] Verify no known-invalid dependency specifications

## 13.3 Build validation

- [ ] Build affected environment when required
- [ ] Review compiler errors
- [ ] Review relevant warnings
- [ ] Check GPIO conflicts
- [ ] Check bus conflicts
- [ ] Check target isolation

### Build status

Use one of:

- `BUILD NOT RUN`
- `BUILD RESULT PROVIDED BY USER`
- `BUILD VERIFIED`
- `HARDWARE VERIFIED`

Do not mark `BUILD VERIFIED` without an actual verified build result.

---

# 15. Agent Proposals

## AP-001 — Branch target for BatteryManagement

Status: RESOLVED — implement on hardware branches, not on `main`

User clarification (2026-09-11):
- `main` should NOT contain hardware-specific code.
- BatteryManagement must be implemented on the dedicated hardware branches (`marble` and/or `waveshare`).
- The `hardware_map.md` for Waveshare is not yet updated (missing documentation).
- No code is to be generated until the architecture is fully clarified.

User clarification (2026-09-11, follow-up):
- First development target for BatteryManagement: `marble` branch.
- Current step: document the plan; do not generate code yet.

Confirmed repository state:
- Current branch: `main`.
- Local branches exist: `marble`, `waveshare`.
- `marble` and `waveshare` branches are at commit `0e2c0fa`.
- `main` is ahead of `marble`/`waveshare` by 4 commits (`cd7c308`, `8edb2e6`, `b6289be`, `b6a1689`).
- The difference between `main` and `marble` is mainly the removal of `BatteryCheckScreen` / `BatteryStub` UI on the `marble` branch (see `git diff main marble -- src/main.cpp`).

Reason:
The project intentionally separates hardware-specific implementations from `main`. Implementing BatteryManagement on `main` would violate this architectural rule.

Suggested action:
1. Switch to the `marble` branch when implementation begins.
2. Port/merge the common UI code from `main` if needed (`BatteryCheckScreen`, `IBatteryProvider`, adapter).
3. Implement `BatteryManagementMarble` only on the `marble` branch.
4. Later, switch to the `waveshare` branch and implement `BatteryManagementWaveshare` after `hardware_map.md` is updated for Waveshare battery/power hardware.
5. Keep `main` hardware-agnostic.

Open sub-decision:
- How to port common UI components from `main` to `marble` without dragging the old `BatteryStub`? Options:
  a) Cherry-pick / manually copy `IBatteryProvider.h` and `BatteryCheckScreen`.
  b) Merge `main` into `marble` and then remove `BatteryStub`.
  c) Recreate UI components on `marble`.

Human decision:
APPROVED — implement on `marble` branch first; document plan now, no code generation yet

---

## AP-002 — GP23 / GP24 logic contradicts `hardware_map.md`

Status: RESOLVED — `hardware_map.md` is the authoritative source for pin meanings

User decision (2026-09-11):
`hardware_map.md` is correct; `human_proposal.md` must be reinterpreted using `hardware_map.md` pin meanings.

Authoritative mapping from `hardware_map.md` §1.5:
- `VBAT_STATUS = GP24`: HIGH (1) = USB Type-C (5V) connected and stable; LOW (0) = USB disconnected.
- `VSYS_STATUS = GP23`: HIGH (1) = battery connected and discharging; LOW (0) = system powered via USB.

Corrected flag derivation (using authoritative pin meanings):
- `usb_pow` = GP24 == 1  (USB 5 V present)
- `bat_pow` = GP23 == 1  (battery is discharging / system runs from battery)
- `bat_chg` = GP24 == 1 AND GP23 == 0  (USB present, system not on battery → charger active)
- `chg_full` = GP24 == 1 AND GP23 == 0 AND ADC3 ≈ 4.2 V  (USB present, no active discharge, battery at max voltage)
- `no_bat` = GP24 == 1 AND GP23 == 0 AND ADC3 < minimum stable battery threshold

Remaining discrepancy:
`hardware_map.md` says "Only read `ADC(29)` when `GPIO23 == LOW` (Battery Mode)". Under the corrected mapping, USB-present = GP23 LOW, so ADC can be read for `chg_full` / `no_bat`. This is consistent. However, `hardware_map.md` does not define how to distinguish `bat_chg` from `chg_full` using only GP23/GP24; it requires the ADC voltage threshold (≈ 4.2 V) as the discriminator.

Suggested action:
Implement BatteryManagement using the corrected mapping above. Do not use the literal GP23/GP24 logic from `human_proposal.md`.

Human decision:
APPROVED — use `hardware_map.md` pin meanings

---

## AP-003 — ADC battery pin / divider inconsistency

Status: RESOLVED — GP29 / ADC3 with divider ratio 3

User decision (2026-09-11):
Use option 1 from the clarification request: GP29 / ADC3 with divider ratio 3.

Authoritative mapping:
- ADC input: GP29 (ADC channel 3)
- Voltage divider: 3 (100 kΩ / 200 kΩ as described in `hardware_map.md`)
- Conversion formula: `Voltage = ADC_Raw / 65535 * 3.3 * 3`

Remaining discrepancy:
`hardware_map.md` §1.5 contains a contradictory subsection that lists `VSYS_ADC = GP23` and `VBAT_ADC = GP24` with divider ratio 1.5. This section conflicts with the confirmed GP29/ADC3 section and with the fact that GP23/GP24 are used as digital inputs. This section must be treated as obsolete/erroneous and should not be used for implementation.

Suggested action:
Implement Marble BatteryManagement using GP29/ADC3 and divider 3. Do not use GP23/GP24 as ADC inputs.

Human decision:
APPROVED — GP29 / ADC3, divider 3

---

## AP-008 — Updated voltage thresholds from `human_proposal.md`

Status: RESOLVED

User decision (2026-09-11):
Use the thresholds from the updated `human_proposal.md` text:
- `bat_100` → active when ADC voltage > 3.85 V
- `bat_40` → active when 3.55 V < ADC voltage ≤ 3.85 V
- `bat_10` → active when ADC voltage ≤ 3.55 V
- `chg_full` → active when GP24 == 1, GP23 == 0, and ADC voltage ≥ 4.15 V (stable)
- `bat_chg` → active when GP24 == 1, GP23 == 0, and ADC voltage < 4.15 V
- `no_bat` → active when GP24 == 1, GP23 == 0, and ADC voltage ≈ 0 V

Finding:
These thresholds override the earlier draft values in the first half of `human_proposal.md` (bat_100 ≥ 4.10 V, bat_40 = 3.65–3.80 V, bat_10 ≤ 3.50 V, chg_full ≈ 4.2 V).

Suggested action:
Use the updated thresholds as authoritative. Mention that they can be refined later based on physical measurement.

Human decision:
APPROVED — use updated thresholds

---

## AP-009 — Stub persistence and Battery Check button behaviour

Status: RESOLVED

User decision (2026-09-11):
- Stub mode is **persisted in EEPROM** across reboots.
- A stub/development button appears in `BatteryCheckScreen` **only when `no_bat` is detected**.
- If stub was previously selected, the system starts directly in stub mode on next boot.

Warning:
Persisting stub mode can mask a real battery that is connected later. The UI must make it clear when the system is running from stub and not from real hardware. Consider adding a visual indicator or requiring a long-press to enter stub mode to avoid accidental activation.

Suggested action:
- Add EEPROM persistence for stub mode.
- Show a clear "STUB / SIMULARE" indicator in Battery Check when stub is active.
- Optionally require confirmation/long-press before entering stub mode.

Human decision:
APPROVED — stub persisted, button visible only on no_bat

---

## AP-010 — Battery percentage resolution

Status: RESOLVED

User decision (2026-09-11):
Use option 2: map the ADC voltage linearly to a 0–100% scale.

Approved mapping:
- 4.2 V → 100%
- 3.4 V → 0%
- Linear interpolation between 3.4 V and 4.2 V.
- Clamp values outside this range to 0% or 100%.

The discrete flags (`bat_10`, `bat_40`, `bat_100`) are still reported to the UI for compatibility with the flag-based API, but `IBatteryProvider::getLevelPercent()` returns the continuous linear value.

Suggested action:
Implement linear voltage-to-percentage mapping in the `IBatteryProvider` adapter. Keep discrete flags available in `BatteryStatus`.

Human decision:
APPROVED — continuous percentage with discrete flags

---

## AP-011 — Waveshare GP29 dual use

Status: RESOLVED — no conflict

User decision (2026-09-11):
There is no conflict. GP29 is used differently on each target:
- Marble: GP29 = ADC3 (battery voltage measurement).
- Waveshare: GP29 = recalibration button.

Because the targets are compiled conditionally and use different GPIO mappings, no runtime conflict exists.

Suggested action:
Ensure target isolation is maintained. Marble battery code must not assume GP29 is available for ADC on Waveshare, and Waveshare recalibration code must not run on Marble.

Human decision:
APPROVED — per-target pin usage, no conflict

---

## AP-004 — Battery UI interface contract change

Status: RESOLVED — adapter pattern

User decision (2026-09-11):
Use option 3: `BatteryManagement` exposes the flag-based API required by `human_proposal.md`; `IBatteryProvider` remains and is implemented as an adapter over the flag-based API.

Approved architecture:
- `BatteryManagement` (target-specific, e.g. `BatteryManagementMarble`) owns the GPIO/ADC reads and exposes a flag/status structure.
- An `IBatteryProvider` implementation (e.g. `BatteryProviderAdapter`) translates the flags into the existing contract:
  - `getLevelPercent()` derived from `bat_10`, `bat_40`, `bat_100` (or finer if available).
  - `isCharging()` = `bat_chg`.
  - `getState()` = `CHARGING` if `bat_chg`; `CRITICAL` if `bat_10`; `LOW` if needed; otherwise `NORMAL`.
- `BatteryCheckScreen` continues to consume `IBatteryProvider` with minimal or no changes.

Suggested action:
Keep `IBatteryProvider.h` unchanged. Add a new `BatteryManagement` class and an adapter class that implements `IBatteryProvider`.

Human decision:
APPROVED — adapter pattern

---

## AP-005 — Stub permanence vs. current stub design

Status: RESOLVED — fallback with UI button when battery is not detected

User decision (2026-09-11):
Use option 2, extended: the stub becomes the active implementation when the hardware reports `no_bat`. Additionally, `BatteryCheckScreen` must show a button only when `no_bat` is true, allowing the technician to switch to stub mode / simulate battery states.

Approved design:
- `BatteryManagement` detects `no_bat` at boot or runtime.
- When `no_bat` is true, the system uses a permanent stub path that provides the same flag-based API.
- `BatteryCheckScreen` displays a button only in `no_bat` state; pressing it enters the stub/test mode.
- The existing serial commands (`bat 10`, `bat 30`, etc.) may remain as a secondary test harness.

Remaining work:
- Update `BatteryStub.h` comments to remove "DESTINAT ELIMINĂRII" and describe the new fallback + test-harness role.
- Decide whether stub mode persists across reboots (EEPROM) or is selected per session.

Suggested action:
Implement `no_bat` detection in `BatteryManagement`, route to stub when active, and add conditional UI button in `BatteryCheckScreen`.

Human decision:
APPROVED — fallback with Battery Check stub button

---

## AP-006 — Waveshare battery path undefined

Status: RESOLVED — Waveshare will use the same flag API; develop Marble first

User decision (2026-09-11):
Use option 2: Waveshare will eventually be adapted to expose the same flag-based API (`usb_pow`, `bat_pow`, `no_bat`, `bat_chg`, `chg_full`, `bat_10`, `bat_40`, `bat_100`). Development focus is on Marble now; Waveshare adaptation will follow.

Approved design:
- Define a common flag/status structure used by both targets.
- `BatteryManagementMarble` implements it for Marble (GP23/GP24/GP29).
- Later, `BatteryManagementWaveshare` (or an adapted `bsp_battery`) implements it for Waveshare (GP25/GP26/GP27).
- Both feed the same `IBatteryProvider` adapter.

Suggested action:
Design the common flag/status structure now so Waveshare adaptation does not break the contract.

Human decision:
APPROVED — common flag API, Marble first

---

## AP-007 — HAL abstraction mismatch

Status: RESOLVED — new target-specific source file

User decision (2026-09-11):
Use option 2: place `BatteryManagement` in a new target-specific source file (e.g. `src/battery/BatteryManagementMarble.cpp` / `.h`).

Approved design:
- Do not reintroduce a full HAL directory just for this module.
- Keep the existing `main.cpp` target-isolation pattern (`#if defined(SERVICEBOX_MARBLE)`).
- New files are compiled only for the relevant target via `src_filter` or `#ifdef` guards.
- The flag/status structure and `IBatteryProvider` adapter remain common/shared.

Suggested action:
Create `src/battery/BatteryManagementMarble.h/.cpp` and include/use it inside the Marble section of `main.cpp`.

Human decision:
APPROVED — new target-specific file

---

# 14. Integration Tests

## 14.1 Startup

- [ ] Cold boot
- [ ] Warm reboot
- [ ] Battery operation
- [ ] USB-powered operation
- [ ] Charging
- [ ] Low battery
- [ ] No SD
- [ ] SD inserted
- [ ] Touch operation
- [ ] Touch calibration
- [ ] No panel
- [ ] Identified panel
- [ ] Unconfigured floor
- [ ] Configured floor

## 14.2 Peripheral combinations

- [ ] LCD + Touch
- [ ] LCD + SD
- [ ] LCD + RS485
- [ ] Touch + SD
- [ ] Touch + RS485
- [ ] SD + RS485
- [ ] LCD + Touch + SD
- [ ] LCD + Touch + RS485
- [ ] LCD + Touch + SD + RS485

## 14.3 Navigation

- [ ] START → Mode Selection
- [ ] Mode Selection → EXIT
- [ ] Mode Selection → FLASH
- [ ] Mode Selection → FLOOR SET
- [ ] FLASH → Panel Type Selection
- [ ] FLASH → Firmware Selection
- [ ] FLASH → Confirm & Flash
- [ ] FLASH → reconnect → handshake → START
- [ ] FLOOR SET → reconnect → handshake → START
- [ ] RETURN behaviour
- [ ] Invalid navigation prevention

## 14.4 Communication

- [ ] No panel
- [ ] One panel
- [ ] Required multi-panel configuration
- [ ] Communication loss
- [ ] Recovery
- [ ] Timeout behaviour
- [ ] Long-run communication

## 14.5 Flash

- [ ] Blank Marble Pico
- [ ] UF2 from SD
- [ ] Successful flash
- [ ] Failed flash
- [ ] USB reconnect
- [ ] Post-flash reboot
- [ ] Post-flash handshake
- [ ] Verify configuration behaviour
- [ ] Verify floor behaviour

---

# 15. Final Validation

## 15.1 Hardware

- [ ] Full Waveshare hardware validation
- [ ] Full Marble hardware validation
- [ ] Peripheral combination validation
- [ ] Power-cycle validation
- [ ] Long-duration stability test

## 15.2 Software

- [ ] Full startup sequence
- [ ] Full touch interaction
- [ ] Full menu navigation
- [ ] Panel identification
- [ ] Communication Test
- [ ] External Display Test
- [ ] MCU INFO
- [ ] Floor Set workflow
- [ ] Flash workflow
- [ ] Error/recovery handling

## 15.3 Documentation

- [ ] Final `hardware_map.md` consistency check
- [ ] Final `ui_requirements.md` consistency check
- [ ] Final `svcbox_menu.drawio` consistency check
- [ ] Final `AGENTS.md` consistency check
- [ ] Remove obsolete documentation
- [ ] Verify TODO reflects actual implementation status

---

# 16. Agent Proposals

This section is specifically reserved for findings, discrepancies, ambiguities,
and changes requiring a human decision.

The agent may add entries here autonomously.

The agent must immediately report every modification made to this section.

The agent must NOT modify protected documentation merely to resolve a proposal.

## Proposal format

### AP-XXX — Short title

- **Status:** OPEN / APPROVED / REJECTED / IMPLEMENTED
- **Affected area:** file / module / task
- **Finding:**
- **Why it matters:**
- **Affected documentation:**
- **Possible resolution:**
- **Does it block the current task:** YES / NO
- **Decision:** human decision required

---

### AP-012 — Marble / BatteryManagement logic cannot detect charge/full/no-bat from current hardware

- **Status:** OPEN
- **Affected area:** src/battery/BatteryManagementMarble.h/.cpp,
  hardware_map.md §1.5, human_proposal.md §3 / §5 / §6
- **Finding:**
  Schematic analysis of `REV0.0.3 RLJDMV_GS Marble Pico.kicad_sch` shows:
  1. GP23 and GP24 are **inputs** (global labels `(shape input)`) and are not
     connected to the gate of Q1 or any other output driver.
  2. Q1 (CJE3134K) is an **N-channel** MOSFET, not P-channel. Its gate is tied
     to +3V3 (through resistor R? — trace at 369.57,93.98→99.06), source to
     the midpoint of the GP29 divider, drain to `GPIO29_A3`.
  3. GP29/ADC3 is connected to **VSYS/VBUS** through a /3 divider made of
     R10 = 200 kΩ and R11 = 100 kΩ. The lower resistor R11 goes to GND;
     the upper resistor R10 connects to the VBUS net.
  4. Therefore, when USB is connected (VBUS ≈ 5 V), GP29 reads ~5 V / 3 ≈
     1.65 V at the ADC pin, regardless of battery state.
  5. When USB is disconnected, VSYS equals battery voltage, so GP29 reads
     VBAT / 3 and can be used for `bat_10` / `bat_40` / `bat_100`.
  6. The TP4065 charger IC (`IC1`) has its `CHRG` pin routed only to test
     point `TP3` (CHG) / charge LED; it is **not connected to any GPIO**.

  As a result, the 8-flag logic requested in `human_proposal.md` cannot be
  implemented literally:
  - `bat_chg` / `chg_full` / `no_bat` require distinguishing battery voltage
    while USB is present, but GP29 measures VBUS in that condition.
  - There is no digital status signal from TP4065 available to the MCU.
  - `bat_pow` is detectable only indirectly (USB not present).
- **Why it matters:**
  Implementing `BatteryManagementMarble` with the exact truth table from
  `human_proposal.md` would produce incorrect states on real hardware:
  `bat_chg`, `chg_full` and `no_bat` could not be distinguished.
- **Affected documentation:**
  - `hardware_map.md` §1.5 (partially outdated/inconsistent, e.g. GP29 divider
    section contradicts itself and the schematic).
  - `human_proposal.md` §3 flag derivation table.
- **Possible resolution:**
  1. **Simplified realistic contract (recommended):**
     - `usb_pow` = GP24 == HIGH.
     - `bat_pow` = GP24 == LOW (use GP29 for level thresholds).
     - When `usb_pow` is active, report `usb_pow` and optionally `bat_chg`
       as a conservative default; do not report `chg_full`/`no_bat` based on
       ADC alone.
     - `no_bat` becomes a manual/atelier mode activated through the stub/test
       button in BatteryCheckScreen (per AP-005/AP-009), not an automatic
       hardware detection.
  2. **Hardware change:** route TP4065 `CHRG` / `STDBY` (or another battery-
     presence signal) to an available GPIO so the firmware can detect charge
     state and battery presence.
- **Does it block the current task:** YES
  Until the contract is clarified, the `BatteryManagementMarble` class cannot
  implement the requested 8 flags without inventing invalid hardware behaviour.
- **Decision:** human decision required

---

### AP-001 — Butoane recalibrare touch (GP29 Waveshare / GP5 Marble)

- **Status:** IMPLEMENTED
- **Affected area:** src/main.cpp / hardware_map.md §1.4 si §2.5
- **Resolution applied:**
  hardware_map.md a fost actualizat (cu acordul proprietarului): butonul
  de recalibrare este documentat la §1.4 (Marble, GP5) si §2.5 (Waveshare,
  GP29), ambele TESTED-OK. Ambele sunt INPUT_PULLUP, activ LOW, 2s hold.
- **Finding (istoric):**
  Pinii GP29 (Waveshare) si GP5 (Marble) folositi pentru butonul de
  recalibrare nu apareaau in hardware_map.md si nu erau atribuiti altei
  functii documentate.
- **Decision:** rezolvat - documentatie actualizata si validata hardware

---

### AP-002 — Waveshare / Discrepanta denumire bus I2C (I2C0 vs i2c1)

- **Status:** IMPLEMENTED
- **Affected area:** hardware_map.md §2.3 si §3.2
- **Resolution applied:**
  hardware_map.md corectat: "I2C0" -> "I2C1" in §2.3 (IMU) si §3.2
  (Touch+IMU), conform implementarii validate fizic (bsp_i2c.h, i2c1,
  GP6/GP7). Solutia 1 din variantele de mai jos.
- **Finding (istoric):**
  hardware_map.md §3.2 denumea busul Touch+IMU "I2C0", insa BSP-ul portat
  (bsp_i2c.h) foloseste instanta `i2c1` pe aceiasi pini, validata hardware.
- **Decision:** rezolvat - documentatie corectata conform validarii hardware

---

### AP-003 — Marble / Simulator temporar pe branch-ul graphic_ui

- **Status:** IMPLEMENTED (depasit)
- **Affected area:** src/sim/ (eliminat), src/main.cpp (environment marble_pico)
- **Finding:**
  REZOLVAT: marble_pico foloseste acum drivere reale (ILI9341 + XPT2046),
  portate din tester-touch-lcd / corectii_cod (doar Model 1 BlueTab,
  sectiunile [PRELUARE]; HAL-ul testerului nu a fost preluat). src/sim/
  a fost eliminat; ramane disponibil in istoricul git (commit 5d7e576)
  daca este nevoie ulterior de simulare.
  [Istoric] Dupa eliminarea HAL-ului, environment-ul marble_pico nu mai avea
  drivere reale si folosea temporar simulatorul Serial din src/sim/.
- **Why it matters:**
  Simulatorul NU este hardware real; functionalitatea Marble nu poate fi
  considerata verificata pe acest branch. Driverele reale trebuie portate
  ulterior de pe branch-ul hardware Marble.
- **Affected documentation:** niciuna
- **Possible resolution:**
  1. La reintegrarea branch-urilor, src/sim/ se elimina si marble_pico primeste
     driverele reale ILI9341/XPT2046;
  2. Sau se pastreaza simulatorul doar pentru teste PC.
- **Does it block the current task:** NO
- **Decision:** human decision required

---

### AP-004 — Marble / Board ID 'groundstudio_marble_pico' necunoscut

- **Status:** IMPLEMENTED
- **Affected area:** platformio.ini [env:marble_pico]
- **Resolution applied:**
  [env:marble_pico] a fost comutat de pe platforma stock `raspberrypi` pe
  platforma comunitara maxgerhardt (`https://github.com/maxgerhardt/
  platform-raspberrypi.git`) - aceeasi platforma folosita de env:waveshare
  si de proiectul tester-touch-lcd (validat hardware). Platforma include
  definitia board-ului groundstudio_marble_pico (RP2040, 133 MHz, 8MB flash).
  Build marble_pico: SUCCESS (57s). Solutia 2 din variantele de mai jos.
- **Finding (istoric):**
  La build-ul pe environment-ul marble_pico (branch graphic_ui), PlatformIO
  raporta: `UnknownBoard: Unknown board ID 'groundstudio_marble_pico'`.
  Eroarea aparea INAINTE de compilarea codului. Definitia board-ului lipsea
  din platforma stock `raspberrypi` instalata local.
- **Decision:** rezolvat - build verificat pe ambele environment-uri

---

### AP-005 — Marble / Conflict intern hardware_map.md: GP6 = TFT_DC si SD_SCLK

- **Status:** IMPLEMENTED
- **Affected area:** hardware_map.md §1.3
- **Resolution applied:**
  hardware_map.md §1.3 rescris cu pinii fizici reali ai slotului SD de pe
  placa Marble Pico (conform documentatiei placi): CS=GP17, MOSI=GP19,
  MISO=GP16, SCK=GP18 (toti pe SPI0), card detect=GP22. Conflictul cu GP6
  (TFT_DC) a disparut; GP5 a ramas liber pentru butonul de recalibrare.
  SD ramane NOT TESTED.
- **Finding (istoric):**
  Vechea asignare §1.3 (SD_SCLK=GP6) intra in conflict direct cu TFT_DC=GP6
  si cu butonul de recalibrare (GP5). Pinii erau generici, nu cei fizici.
- **Decision:** rezolvat - documentatie actualizata cu pinii fizici reali

---

### AP-006 — Battery / Pini baterie si detectie incarcare nu sunt documentati

- **Status:** OPEN
- **Affected area:** src/bsp/bsp_battery.h, src/bsp/bsp_battery.c,
  hardware_map.md §4 Current Hardware Validation Status
- **Finding:**
  BSP-ul existent `bsp_battery.h` defineste pinii:
  - BSP_BAT_ADC_PIN = 27
  - BSP_BAT_EN_PIN  = 26
  - BSP_BAT_KEY_PIN = 25
  Acesti pini nu sunt documentati in hardware_map.md pentru niciun target.
  De asemenea, hardware_map.md marcheaza "Battery measurement" si
  "Charging detection" ca NOT VERIFIED pentru ambele targeturi.
  Semnificatia pinului BSP_BAT_KEY_PIN (detectie incarcare?) nu este
  confirmata.
  In plus, codul existent din `src/bsp/bsp_battery.c` nu este utilizat in
  `main.cpp`; pentru testarea pe graphic_ui se foloseste un stub controlat
  prin comenzi seriale (`src/battery/BatteryStub.h/.cpp`).
- **Why it matters:**
  Implementarea reala a bateriei pe branch-urile marble/waveshare necesita
  o harta hardware clara si validata. Pana la clarificare, Battery Check va
  folosi un stub testabil prin comenzi seriale.
- **Affected documentation:** hardware_map.md
- **Possible resolution:**
  1. Se adauga in hardware_map.md pinii bateriei pentru fiecare target,
     impreuna cu schema de divizor, ADC si semnal de detectie incarcare;
  2. Se valideaza fizic masurarea tensiunii si detectia de incarcare pe
     fiecare target inainte de a inlocui stub-ul.
- **Does it block the current task:** NO
- **Decision:** human decision required

---

### AP-007 — Battery Check UI / Indicator la încărcare completă

- **Status:** OPEN
- **Affected area:** src/ui/BatteryCheckScreen.h, src/ui/BatteryCheckScreen.cpp
- **Finding:**
  S-a identificat necesitatea ca, în pagina Battery Check standby,
  punctul indicator să rămână aprins continuu sau să clipească cu altă
  frecvență atunci când bateria este complet încărcată (100% + charging).
  Momentan indicatorul clipește cu același ritm indiferent de starea de
  încărcare.
- **Why it matters:**
  Afișarea unei diferențe vizuale pentru „încărcare completă” este o
  îmbunătățire UI clară, dar depinde de capacitatea Battery Manager-ului
  (hardware-dependent) de a distinge „încărcare în desfășurare” de
  „încărcare completă”. State-ul curent `BatteryState` conține doar
  `CHARGING`, nu și un `FULL` explicit.
- **Possible resolution:**
  1. Se confirmă de la hardware/Battery Manager dacă se poate obține
     informația explicită de încărcare completă (de ex. prin `BatteryState::FULL`,
     sau prin `CHARGING + getLevelPercent() == 100`).
  2. Dacă semnalul este disponibil, se modifică doar UI-ul în
     `BatteryCheckScreen` pentru a menține indicatorul verde aprins continuu
     sau pentru a-i schimba frecvența de clipit în standby.
- **Does it block the current task:** NO
- **Decision:** human decision required

---

- `[x]` means the task has actually been completed.
- Code compilation alone does not make a task complete.
- Hardware tasks require physical verification on the corresponding target.
- `BUILD VERIFIED` and `HARDWARE VERIFIED` are separate states.
- Documentation status does not imply implementation status.
- Do not mark tasks complete based on assumptions.
- Do not mark hardware as verified from simulation or compilation alone.
- Do not reuse stale test results after relevant hardware/software changes.
- When a requirement changes, update the authoritative requirement document first.
- Implementation status is then tracked here.

---

# 18. Agent Working Boundary

The agent:

1. inspects the current repository before making changes;
2. identifies the affected target;
3. checks the relevant documentation;
4. checks the existing implementation;
5. makes only the requested change;
6. preserves working unrelated functionality;
7. validates the affected layer;
8. reports the result.

If a change would require modification of protected documentation:

1. stop before modifying that documentation;
2. identify the discrepancy;
3. add an `Agent Proposal`;
4. explain the impact;
5. propose possible resolutions;
6. state whether the task is blocked;
7. wait for human approval.

Protected documentation:

- `AGENTS.md`
- `hardware_map.md`
- `svcbox_menu.drawio`

---

# 19. Current Verified Hardware Status

The following status records only hardware verification that has actually
been established.

## Waveshare RP2350

- LCD / Display: **HARDWARE VERIFIED**
- Touchscreen / CST328: **HARDWARE VERIFIED**
- SD: **NOT VERIFIED**
- Battery measurement: **NOT VERIFIED**
- Charging detection: **NOT VERIFIED**
- RS485: **NOT VERIFIED**

## Marble Pico

- LCD / Display: **NOT VERIFIED**
- Touchscreen: **NOT VERIFIED**
- SD: **NOT VERIFIED**
- Battery measurement: **NOT VERIFIED**
- Charging detection: **NOT VERIFIED**
- RS485: **NOT VERIFIED**
- USB / UF2: **NOT VERIFIED**

Hardware mappings themselves are maintained exclusively in:

`hardware_map.md`

---

# 20. Current Build Status

- Waveshare: `BUILD NOT RUN`
- Marble Pico: `BUILD NOT RUN`

Update these entries only from an actual build result.

---

# 21. Final Principle

`todo.md` answers:

> What remains to be implemented or verified?

It does not answer:

> What are the hardware pins?

That belongs in `hardware_map.md`.

It does not answer:

> What should the UI do?

That belongs in `ui_requirements.md`.

It does not answer:

> What is the menu structure?

That belongs in `svcbox_menu.drawio`.

It does not answer:

> How should the agent work?

That belongs in `AGENTS.md`.

The TODO tracks implementation and verification only.