# Palier Duplex Panel - Visual Layout Engine States

This document specifies the internal display rendering behaviors of the dual TFT screen matrices when service routines are asserted.

## 1. Base Service Frame Layer
* **Visual Action:** Instantly intercepts normal runtime rendering blocks.
* **Graphic Overlay:** Draws a dedicated high-contrast tracking border frame around both the Left (Lift 1) and Right (Lift 2) TFT screens.
* **Context Lockout:** Disables standard button call monitoring functions.

## 2. Dynamic Telemetry Vector Layer
* **Visual Action:** Triggered via data updates over the parser pipeline.
* **Layout Distribution:**
  * **Left TFT Display:** Translates the parsed string payload array into discrete visual display meters representing Lift 1 telemetry parameters.
  * **Right TFT Display:** Mirrored translation block tracking Lift 2 telemetry parameters.

## 3. Peripheral Test Routine Visuals
* **Visual Action:** Fired by validation diagnostics.
* **Screen 1 Isolation:** Clears the Left TFT display matrix completely and paints an isolated fullscreen verification window showing a solid **"PASS"** indicator block.
* **Screen 2 Isolation:** Clears the Right TFT display matrix and paints the isolated fullscreen **"PASS"** indicator block.
* **Layout Tear-down:** Upon tracking the teardown sequence token, the visual pipeline flushes the isolated blocks and re-renders the base service frame.
