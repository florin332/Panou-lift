# cabin Panel - Local Development TODO List

## 1. Asynchronous USB Client Parser
- [ ] Expand string tokenization inside `processCommand()` using `indexOf()` and `substring()` to split parameters safely.
- [ ] Implement direct automated ASCII feedback transmissions: `ACK SRV ENTER`, `ACK SRV EXIT`, and global `ERR` strings.

## 2. Dual TFT Graphic Rendering Layers
- [ ] Code the service layout frame overlay to lock out standard display rendering when `mb_01` status is active.
- [x] Implement a nonblocking LCD color test following the `panou_duplex` model (`mb_32 1` / `mb_33`).



## 4. W5500 Ethernet Migration - Cabin Panel
- [x] Update the panel hardware map: keep the ILI9341 on SPI0; connect W5500 to SPI1 (`GP12` MISO, `GP13` CS, `GP14` SCK, `GP15` MOSI). Keep I2S on `GP8`-`GP10`.
- [x] Add a W5500 hardware-SPI driver/HAL that exposes network and TCP byte-stream services only; keep encoder, alarm, and service message formats outside the HAL.
- [x] Add independent TCP channels for encoder data (default port `2000`), alarm equipment (`2004`), and service (`2005`); default the panel to TCP server mode.
- [x] Configure the W5500 with MAC `3C-AB-72-52-68-12` and DHCP; the router's MAC reservation is already configured.
- [x] Keep the W5500 driver owned by one core; route requests from other cores through a queue or synchronized shared state.
- [x] Adapt encoder reception to consume the TCP byte stream while retaining frame parsing, CRC checks, and semantic validation in hardware-independent modules.
- [ ] Define `AlarmProtocol` separately from transport; implement/test the GSM and SSC30KQ command formats when each device protocol is specified.
- [ ] Implement reconnect, disconnect, partial-read/write, timeout, and per-channel status handling; verify the remote CH9121 bridges against the panel's TCP roles.
- [ ] Provide the web service over W5500; decide its HTTP port and confirm the available socket/RAM budget (W5500 has up to 8 sockets).
- [ ] Route basic service commands through one shared dispatcher for USB and web; expose advanced network settings only in the web interface.
- [ ] Add persistent service configuration for per-channel TCP role and ports, plus network settings; initially use DHCP, the fixed MAC above, server mode, and ports `2000`/`2004`/`2005` as defaults.
- [ ] Allow the MAC to be changed later through the web service interface; document that changing it requires updating the router's DHCP reservation.
- [ ] Test simultaneous encoder, alarm, service-TCP, and web traffic; verify TCP stream framing, W5500 SPI1 operation alongside LCD SPI0, and behavior across both cores.
- [ ] Keep the web interface on a trusted local network unless TLS or another secure access layer is added.
