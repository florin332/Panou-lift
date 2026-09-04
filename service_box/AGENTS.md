# AGENTS.md — Service Box

## 1. SCOPE

These instructions apply to the entire `service_box` project.

The Service Box is a dedicated service/configuration device for the
Panou-lift project.

The project contains hardware-specific implementations. Changes must
always be made for the explicitly requested target and must not
accidentally affect another hardware target.

---

## 2. GENERAL WORKING RULES

Before modifying code:

1. Inspect the current repository state.
2. Inspect the relevant existing implementation.
3. Inspect the corresponding project documentation.
4. Check `platformio.ini` and identify the affected PlatformIO environment.
5. Inspect the relevant project specification and TODO section.
6. Reuse existing abstractions and implementations whenever possible.

Do not assume that an older implementation, previous prompt, or previous
analysis still represents the current state of the project.

The current repository is the source of truth for software structure.

Hardware configuration must be taken from `hardware_map.md`.

Functional UI requirements must be taken from the current UI specification
and approved project documentation.

---

## 3. TARGET SEPARATION

The Service Box may contain multiple hardware implementations.

In particular:

- Marble Pico
- Waveshare RP2350 Touch LCD 2.8"

Do NOT modify one hardware target when working on another target unless
the task explicitly requires it.

A task concerning one target must remain isolated to that target whenever
possible.

---

## 4. HARDWARE CONFIGURATION

`hardware_map.md` is the authoritative document for hardware
configuration.

This includes, but is not limited to:

- GPIO assignments;
- peripheral buses;
- display connections;
- touch connections;
- SD connections;
- RS485 connections;
- power and control signals;
- board-specific hardware information.

The agent MUST inspect `hardware_map.md` before making hardware-related
changes.

The agent MUST NOT duplicate the hardware mapping in `AGENTS.md`.

The agent MUST NOT invent hardware assignments based on generic examples,
library defaults, or convenience.

If the current implementation conflicts with `hardware_map.md`, the agent
must report the discrepancy and create a proposal in:

    todo.md → Agent Proposals

The agent must not silently change either the code or the hardware
documentation to resolve the conflict.

---

## 5. HARDWARE TARGET ISOLATION

When working on a hardware-specific task:

1. identify the target;
2. inspect the corresponding hardware documentation;
3. inspect the corresponding implementation;
4. modify only the required target.

Do not change another hardware implementation merely because it uses a
different or apparently simpler configuration.

If a shared change is genuinely required, identify it explicitly in the
final report.

---

## 6. BUS AND PERIPHERAL SAFETY

Bus and peripheral configuration must be taken from the current project
implementation and `hardware_map.md`.

Do not change:

- SPI configuration;
- I2C configuration;
- UART configuration;
- peripheral instances;
- bus ownership;
- chip-select handling;

merely because a library example uses a different configuration.

Before changing a bus or peripheral assignment:

1. inspect `hardware_map.md`;
2. inspect the existing implementation;
3. inspect the relevant HAL;
4. check for other users of the peripheral.

If a bus or peripheral change appears necessary but is not explicitly
defined by the current requirements, do not make the architectural
decision autonomously.

Report the discrepancy and create a proposal in:

    todo.md → Agent Proposals

---

## 7. EXISTING HARDWARE ABSTRACTION

The project contains a hardware abstraction layer.

Before adding new hardware code, inspect:

    src/hal/HardwareInterface.h

and the corresponding hardware implementations.

Reuse existing abstractions whenever they can support the requested
functionality.

Do not create parallel hardware APIs without a concrete reason.

UI code should not directly access hardware when the HAL already provides
the required interface.

---

## 8. EXISTING WORKING HARDWARE IMPLEMENTATION

When working on another feature, preserve existing hardware functionality.

Do not unnecessarily:

- rewrite working drivers;
- replace working libraries;
- change working initialization sequences;
- change hardware mappings;
- change bus assignments.

A task concerning one subsystem must not become an excuse to rewrite
another working subsystem.

Any required change to existing hardware behaviour must be justified by:

- an explicit requirement;
- a confirmed hardware issue;
- a functional bug;
- or an approved project decision.

---

## 9. UI ARCHITECTURE

The Service Box UI is defined by the current UI specification and menu
diagram.

Primary UI references:

    ui_requirements.md
    svcbox_menu.drawio

These documents define the required UI behaviour and intended menu
structure.

The UI implementation must follow these documents.

Do not invent a different menu hierarchy.

Do not redesign the UI unless explicitly requested.

Hardware input handling and UI behaviour should remain separated.

The preferred architecture is:

    hardware
        ↓
    HAL
        ↓
    input/state
        ↓
    UI
        ↓
    menu/application logic

Do not bypass these layers without a concrete reason.

---

## 10. UI REQUIREMENTS VS TODO

`ui_requirements.md` defines the required UI behaviour.

`svcbox_menu.drawio` defines the intended menu structure and navigation
flow.

`todo.md` is used to track:

- implementation;
- verification;
- outstanding work;
- Agent Proposals.

`todo.md` is NOT the source of truth for functional requirements.

Do not change `ui_requirements.md` merely to make an implementation
easier.

Do not change the menu structure merely because a different flow is
easier to implement.

When implementing a UI task, inspect the corresponding requirements and
the corresponding menu flow before modifying code.

---

## 11. TODO STRUCTURE AND TASK BOUNDARIES

When a task is provided using a TODO path such as:

    Hardware bring-up — Waveshare / Touchscreen

the agent must work only within that requested scope unless the task
explicitly requires changes elsewhere.

The hierarchical structure of `todo.md` is intentional.

A parent section identifies the work area.

A subsection identifies the specific component or function.

Individual checklist items identify concrete implementation or validation
tasks.

Do not expand a focused task into unrelated work.

Do not mark a task `DONE` merely because the code compiles.

Hardware tasks require physical verification before being considered
hardware-complete.

---

## 12. AGENT PROPOSALS

`todo.md` contains a dedicated section:

    ## Agent Proposals

This is the ONLY area of `todo.md` that the agent may modify autonomously
for reporting new findings, ambiguities, discrepancies, or proposed
decisions.

If the agent encounters:

- a missing requirement;
- an ambiguity;
- conflicting documentation;
- an implementation decision not defined by the project;
- a hardware conflict;
- a missing prerequisite;
- a potentially necessary architectural change;
- a discrepancy between code and protected documentation;

the agent must NOT invent a solution.

Instead, the agent must add a proposal to:

    todo.md → Agent Proposals

A proposal should contain:

- proposal ID;
- affected area;
- status;
- finding or problem;
- reason;
- suggested action, if applicable;
- human decision.

Example:

    ### AP-001 — Waveshare / Touchscreen

    Status: OPEN

    Finding:
    ...

    Reason:
    ...

    Suggested action:
    ...

    Human decision:
    PENDING

The agent must not modify existing TODO tasks, completed tasks,
requirements, or specification sections in order to silently resolve an
ambiguity.

The agent may continue with the requested task only when the existing
requirements provide an unambiguous implementation path.

If the unresolved issue blocks the requested task, stop and report the
proposal.

A proposal is NOT an approval.

The human project owner decides whether the code, documentation, or
requirement must change.

---

## 13. PROTECTED DOCUMENTATION

The following files are protected project documents:

    svcbox_menu.drawio
    hardware_map.md
    AGENTS.md

The agent MUST NOT modify these files autonomously.

These files are controlled project documents and must not be changed as a
side effect of code implementation.

The agent may inspect them when relevant.

The agent must not modify a protected document merely because the current
implementation differs from it.

---

## 14. DOCUMENTATION CHANGE REQUESTS

During implementation, the agent may discover that a code change would
require a change to a protected document.

Examples include:

- a changed hardware mapping;
- a changed GPIO assignment;
- a changed peripheral bus;
- a changed UI flow;
- a new menu item;
- a changed startup sequence;
- a changed reconnect sequence;
- a changed architectural rule;
- a requirement that no longer matches the implementation.

The agent MUST NOT modify the protected document.

Instead, the agent must:

1. immediately report the discrepancy to the user;
2. add a proposal to `todo.md → Agent Proposals`;
3. identify the affected protected document;
4. identify the affected section or information;
5. describe the discrepancy;
6. identify the code change or observation that caused it;
7. propose the possible resolution(s);
8. state whether the issue blocks the current task.

The agent must not decide autonomously whether:

- the code should change;
- the documentation should change;
- the requirement should change.

That decision belongs to the human project owner.

---

## 15. DOCUMENTATION VS IMPLEMENTATION CONFLICTS

When the current implementation conflicts with a protected reference
document, the agent MUST NOT decide autonomously which one is correct.

The agent must:

1. immediately report the discrepancy to the user;
2. record it in `todo.md → Agent Proposals`;
3. describe the affected code and documentation;
4. explain the nature and impact of the discrepancy;
5. propose the possible resolution(s);
6. clearly state whether the discrepancy blocks the current task.

Possible proposals may include:

- changing the code to comply with the existing documentation;
- changing the documentation to reflect an intentional implementation
  change;
- clarifying or changing the requirement before implementation continues.

The agent must not silently resolve the conflict.

The existence of working code is not sufficient justification for changing
protected documentation.

The existence of documentation is not sufficient justification for
assuming that the implementation is correct.

The human project owner decides the resolution.

Until that decision is made, the protected documentation remains
unchanged.

---

## 16. PIN SAFETY

Never invent GPIO assignments.

Before assigning, changing, or reusing a GPIO:

1. inspect `hardware_map.md`;
2. inspect the existing HAL implementation;
3. inspect the relevant hardware initialization code;
4. check for existing peripheral use.

The GPIO mapping must be taken from `hardware_map.md`.

Do not duplicate GPIO assignments in `AGENTS.md`.

If a requested implementation conflicts with the documented hardware
mapping, do not resolve the conflict autonomously.

Immediately report the discrepancy and create or update an entry under:

    todo.md → Agent Proposals

The human project owner decides whether the code or hardware
documentation must change.

---

## 17. PLATFORMIO

Always inspect:

    platformio.ini

before changing dependencies or build configuration.

Identify the affected PlatformIO environment before making target-specific
changes.

Do not introduce arbitrary library versions.

Do not reintroduce known-invalid dependency specifications.

Use the existing project dependency arrangement whenever possible.

Do not modify build configuration for unrelated targets.

---

## 18. LIBRARIES

Before adding a library:

1. check whether the project already contains an implementation;
2. check `platformio.ini`;
3. check the local `lib/` directory;
4. check whether the required functionality already exists in the HAL.

Do not add duplicate libraries.

Do not replace a working library without a concrete requirement.

Do not select a library based solely on a generic example when the
project already has a working implementation.

---

## 19. MINIMAL CHANGES

Make the smallest change required to complete the requested task.

Do not perform unrelated refactoring.

Do not:

- rename unrelated files;
- reorganize directories unnecessarily;
- rewrite working drivers;
- change unrelated hardware;
- change unrelated communication behaviour;
- change another hardware target;
- redesign the menu architecture.

Working code has priority over stylistic cleanup.

---

## 20. EXISTING CODE TAKES PRIORITY OVER ASSUMPTIONS

If an existing implementation looks unusual but works with the current
hardware, do not replace it merely because another implementation appears
cleaner.

Before changing working code, establish a concrete reason:

- hardware requirement;
- documented requirement;
- compile failure;
- functional bug;
- explicitly requested architectural change.

Do not "improve" working code without justification.

---

## 21. TASK BOUNDARIES

Implement ONLY the requested task.

For example, if the task is:

    Implement touchscreen input.

the task does NOT automatically include:

- final menu navigation;
- gestures;
- animations;
- button redesign;
- UI redesign;
- calibration redesign;
- battery changes;
- RS485 changes.

Implement the requested layer first.

Validate it.

Only then proceed to another layer when explicitly requested or when it is
clearly part of the same task.

---

## 22. DEBUGGING

Temporary diagnostic output is allowed when it helps validate hardware or
software behaviour.

Diagnostic code should:

- be simple;
- be non-blocking where practical;
- avoid unnecessary long delays;
- not interfere with normal operation;
- be easy to remove.

Do not leave continuous debug output in normal operation unless explicitly
required.

Temporary diagnostics must not silently become permanent architecture.

---

## 23. TIMING AND BLOCKING

Avoid unnecessary `delay()` calls in application and input handling.

Hardware initialization may require delays when dictated by the device
requirements or a verified working implementation.

Runtime UI and input processing should remain responsive.

Do not introduce long blocking waits merely to simplify a test.

---

## 24. UI INPUT

Touch input should be handled as an input source, not mixed directly into
menu rendering code.

The preferred separation is:

    hardware
       ↓
    Hardware HAL
       ↓
    touch state / coordinates
       ↓
    UI input handling
       ↓
    menu action

Raw hardware coordinates and UI/screen coordinates should remain
conceptually separate.

Hardware-specific touch implementation must follow the current hardware
mapping and HAL.

Do not introduce arbitrary calibration or coordinate transformations.

If required calibration behaviour is not defined by the current project
requirements, do not invent it.

Report the missing requirement through:

    todo.md → Agent Proposals

---

## 25. SERVICE BOX STARTUP AND RECONNECT

Startup, panel identification, handshake and reconnect behaviour must
follow the current UI specification.

When the specification requires current panel information before showing
`Start`, do not display stale information.

After operations that can change panel state, follow the specified
reconnect and handshake sequence before returning to `Start`.

Do not bypass required reconnect or handshake stages by reusing stale
panel information.

Do not invent alternative startup or reconnect flows.

---

## 26. COMMUNICATION TEST VS HANDSHAKE

The `Communication` UI function is NOT automatically a test of the
Service Box ↔ panel handshake.

The Service Box ↔ panel handshake is a separate function when used for
panel identification and reconnect.

`Communication Test` must follow the current UI requirements and the
actual panel communication architecture.

Do not implement Communication Test as a duplicate of the handshake
unless explicitly required.

---

## 27. DISPLAY TEST

The meaning of the `Display` function must follow the current UI
requirements and menu specification.

Do not reinterpret an external panel display test as a test of the
Service Box display.

Do not invent additional display-test functionality that is not defined
by the project requirements.

---

## 28. TEMPORARY TESTS

Temporary hardware test code is not automatically part of the final UI.

If a task requires a temporary test:

- clearly isolate it;
- keep it minimal;
- do not let it become permanent architecture;
- remove or disable it when validation is complete unless explicitly
  required to retain it.

Do not modify protected documentation merely because a temporary test
requires a different configuration.

---

## 29. BUILD VALIDATION

The build must NOT be executed automatically by the agent after every
code modification.

Builds are run manually by the user when considered necessary.

After modifying code, the agent should:

1. perform a static review of the changes;
2. check for obvious syntax or structural problems;
3. check for relevant hardware conflicts;
4. check that existing buses have not been changed without justification;
5. check that unrelated targets have not been modified;
6. clearly report what should be verified by a build.

### 29.1 Build Log

The result of a manually executed build may be provided to the agent
through a build log.

When a build log is provided, the agent must analyze it before proposing
additional changes.

The agent must NOT assume that the build should be executed again
automatically.

### 29.2 Build Errors

If the provided build log contains errors:

- identify the actual cause of the error;
- distinguish real build errors from warnings, configuration issues, or
  already-known problems;
- propose or apply only the changes necessary to address the identified
  error;
- do not repeatedly modify and rebuild the code merely to obtain a
  successful build;
- do not perform unrelated refactoring;
- do not automatically start multiple consecutive build attempts for the
  same problem.

If an error can be corrected manually by the user, clearly identify the
manual correction instead of forcing an automated rebuild cycle.

### 29.3 Build Status

The agent must clearly distinguish between:

    BUILD NOT RUN
    BUILD RESULT PROVIDED BY USER
    BUILD VERIFIED
    HARDWARE VERIFIED

The agent must NOT report `BUILD VERIFIED` unless the build has actually
been executed and its result has been verified.

Build validation and hardware validation are separate stages.

---

## 30. HARDWARE VALIDATION

Compilation is not sufficient for hardware changes.

When a task involves physical hardware, distinguish between:

    BUILD VERIFIED

and:

    HARDWARE VERIFIED

Do not claim hardware functionality has been verified merely because the
code compiles.

Physical test results supplied by the user are considered confirmed
hardware information.

If physical verification is still required, state that explicitly.

---

## 31. FILE MODIFICATION RULES

Before modifying a file, inspect its current contents.

Do not replace an entire file when a small modification is sufficient.

Do not modify files outside the requested scope unless required by the
task.

Protected files are governed by the protected-documentation rules above.

After modification, report:

1. every modified file;
2. what was changed;
3. why it was changed;
4. which existing functionality was preserved;
5. any hardware or architectural impact;
6. any TODO / Agent Proposal changes.

Every autonomous modification to `todo.md` must be explicitly reported.

---

## 32. IMPORTANT PROJECT FILES

Relevant Service Box files may include:

    AGENTS.md
    hardware_map.md
    platformio.ini
    svcbox_menu.drawio
    todo.md
    ui_requirements.md

and implementation files such as:

    src/main.cpp
    src/hal/HardwareInterface.h
    src/hal/HardwareWaveshare.cpp
    src/hal/HardwareMarble.cpp

Other files may exist and should be inspected when relevant.

The list above is a navigation aid, not a second source of technical
requirements.

---

## 33. SOURCE OF TRUTH PRIORITY

When information conflicts, use this priority:

1. Confirmed physical hardware information
2. Current working implementation
3. Current hardware documentation
4. Current UI specification and approved project documentation
5. Current TODO and implementation status
6. Historical prompts, analyses or temporary test instructions
7. Generic assumptions or library examples

This priority does NOT give the agent permission to modify protected
documentation.

When a conflict involves protected documentation, the conflict must be
reported and proposed through:

    todo.md → Agent Proposals

Historical prompts and analyses must NOT override the current repository
state.

`todo.md` must not override confirmed requirements or hardware
information.

---

## 34. NO GUESSING RULE

If an important technical detail cannot be established from:

- the current code;
- the current documentation;
- confirmed hardware information;
- or an explicitly supplied requirement;

do not guess.

If the issue affects implementation, add an entry to:

    todo.md → Agent Proposals

and identify the decision that must be made.

The agent's role is to identify and explain the missing information, not
to invent the missing requirement.

---

## 35. TASK GRANULARITY

The agent must normally work within the task boundaries defined in `todo.md`.

If a task is too complex, contains multiple independent implementation or
verification stages, or cannot be reliably completed and validated as a
single unit, the agent may propose temporary task granularization.

The agent must NOT autonomously modify the task structure in `todo.md`.

Instead, the agent must:
1. explain why the task is too complex to track as a single unit;
2. propose a temporary breakdown into clear, independently verifiable steps;
3. wait for human approval before changing the task structure.

Temporary granularization must remain limited to the affected task and must
not duplicate information already defined in authoritative project
documentation.

The agent should propose granularization only when it provides a real benefit
for implementation, verification, debugging, or identifying blockers.

After the task is completed, temporary subtasks should be consolidated back
into the normal task-level representation by the human.

---

## 36. FINAL REPORT

At the end of every implementation task, provide a concise report:

    Files modified:
    - ...

    Changes:
    - ...

    TODO / Agent Proposals modified:
    - ...

    Hardware affected:
    - ...

    Build:
    - ...

    Hardware verification:
    - ...

    Remaining issues:
    - ...

    Proposals requiring human decision:
    - ...

The report must explicitly mention any discrepancy discovered between
implementation and protected documentation.

Do not claim successful hardware operation unless it was actually tested.

Do not claim `BUILD VERIFIED` unless an actual build result has been
verified.

---

## 37. FINAL PRINCIPLE

Preserve what already works.

Make the smallest correct change.

Do not guess hardware.

Take hardware configuration from `hardware_map.md`.

Do not duplicate hardware configuration in `AGENTS.md`.

Do not invent missing requirements.

Do not silently resolve ambiguities or documentation conflicts.

Use `todo.md → Agent Proposals` for unresolved technical decisions.

Immediately report every discrepancy that requires a human decision.

Do not modify protected documentation autonomously.

The human project owner decides whether code, documentation, or
requirements must change.

The agent identifies, explains and proposes.

The human decides.

The agent implements the approved decision.

Do not mix hardware targets.
410
Do not turn a focused task into a refactoring project.

Do not enter automated build/rebuild cycles.

Keep the Service Box architecture deterministic, controlled and
incremental.