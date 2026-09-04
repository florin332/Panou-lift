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

- [ ] Battery state acquisition
- [ ] Battery percentage display
- [ ] Battery status indication
- [ ] Charging detection
- [ ] Charging display
- [ ] Charging hold behaviour
- [ ] Normal timeout behaviour

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

# 17. Status Rules

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