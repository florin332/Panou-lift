# Web UI

## Access and security

The W5500 serves the interface on port `80`. With DHCP enabled, open `http://<panel-ip>/`; the active address is available from the router's lease table. HTML, CSS, and JavaScript are compiled into firmware and served from flash. The page sources live in `web/` (`index.html`, `app.css`, `app.js`) and are embedded at build time by `scripts/embed_webui.py`, which generates `WebUiContent.h` with the `PROGMEM` constants used by `src/Drivers/WebUi.cpp`.

Read-only status endpoints are available without authentication on the trusted LAN. Every state-changing endpoint requires HTTP Basic Auth, a same-origin `Origin` header, and the expected plain-text request format. Service operations are also checked by `ServiceProtocol`; network configuration additionally requires Service Mode. The fixed username is `service`.

Set a local password before using service or configuration actions. Create `src/Core/Models/WebAuth.local.h` (this path is ignored by Git) with:

```cpp
#define WEB_UI_PASSWORD "replace-with-a-long-unique-password"
```

Rebuild and upload the firmware after setting it. An empty password disables all mutating web operations. Basic Auth over this HTTP server does not encrypt credentials or traffic; do not expose port `80` outside an isolated, trusted local network. TLS is not implemented.

In the page, enter the password in the service section and select **Autentificare**. The browser sends the Basic Auth header with service/configuration requests; the password field is cleared after the request. Select **Deconectare** to clear the in-memory credential for the current page.

A failed login returns `401` with a short reason that is also shown in the page: password not configured in firmware, missing `Authorization` header, invalid base64 encoding, or wrong username/password. Responses to authentication failures include a `WWW-Authenticate: Basic` header.

## API

`GET /api/state` returns lift position, destination, direction and service state; uptime and reset reason; receive/error counters; current network address; and configured system/network settings.

`POST /api/service` accepts one ASCII `text/plain` service command. Supported commands include service entry/exit, display test/reset, communication-page and receive-counter controls, lift number (`mb_11 <number>`), audio gain (`mb_12 <gain>`), and restoring defaults (`mb_30`). Commands use the same `ServiceProtocol` dispatcher as USB and the existing service TCP channel.

`POST /api/network` accepts `DHCP` or `STATIC <ip> <subnet> <gateway> <dns>`. Static values are validated and saved in a checksummed EEPROM record. The current network session is not interrupted; restart the panel to apply changes. The MAC and existing TCP listener roles/ports remain fixed.

Only one HTTP client is serviced at a time. The web listener uses the fourth W5500 server socket alongside ports `2000`, `2004`, and `2005`.

## Firmware update (OTA via SD)

The service section exposes a three-phase update flow, visible after authentication and enabled only in Service Mode:

1. **Upload pe SD** — streams a `.uf2` file to `/UPDATE/FIRMWARE.UF2` (`POST /api/update/upload`, raw `application/octet-stream`, filename in `X-Filename`). While the panel owns the SD card, Core 1 pauses audio and display rendering (SD and LCD share SPI0) and the watchdog is armed (2 s) against hangs.
2. **Verifica** — `POST /api/update/verify`: full UF2 structural validation (magics, RP2040 family, 256 B payloads, coherent block numbering, strictly consecutive addresses within physical flash, excluding the settings sector) plus SHA-256 over the entire file; results are stored in `/UPDATE/FIRMWARE.META`.
3. **Aplica update** — `POST /api/update/apply`: re-hashes the file against the manifest, gracefully parks Core 1 (audio stream closed, SPI released, ack), then a Core 0 installer programs 4 KB pages, skipping identical pages and the boot2/OTA-shim region below `0x10003000`, with read-back verification. The panel reboots into the new firmware.

The UF2 file is kept on the card until the new firmware confirms a successful boot (first boot removes `/UPDATE/BOOT.PND` and the UI shows "boot confirmat"). A failed install leaves `/UPDATE/FAILED.TXT`, reported as the update error on next boot. `POST /api/update/cancel` abandons the flow. Progress and errors are exposed in `GET /api/state` under `update`.

Recovery remains unchanged: physical BOOTSEL or the Service Box (`mb_02`). Filenames following the `<tip>_<hh>_<v.vv>` convention (e.g. `C_01_1.02.uf2`) are compatibility-checked against the panel identity; known mismatches are rejected.