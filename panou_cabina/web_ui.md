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