# Panou-lift - Global Integration TODO List

## [ ] Sub-Module Stabilization
- [ ] Complete and verify individual local codebases inside `service_box/`.
- [ ] Complete and verify individual local codebases inside `panou_duplex/`.
- [ ] Ensure both environments compile with zero errors under PlatformIO.

## [ ] System Integration & USB Interconnection
- [ ] Perform direct physical USB Type-C linking between the Service Box (Host) and Duplex Panel (Client).
- [ ] Validate the non-blocking ASCII handshake loop (`mb_in` -> `ACK SRV ENTER`) on live hardware.
- [ ] Run continuous stress tests on the `mb_com` telemetry streaming pipeline to monitor virtual port stability.

## [ ] Failure Mode & Edge Case Testing
- [ ] Verify Host reaction times (150ms timeout) by simulating unexpected cable disconnects during runtime.
- [ ] Validate the 3-retry arbitration budget and ensure error alerts pop up correctly on the Master screen.
- [ ] Test system behavior during hot-plugging cables and unexpected power-loss fallbacks.

## [ ] Final Deployment
- [ ] Draft overall system electrical schematics and wire routing diagrams.
- [ ] Complete assembly instructions and mechanical enclosure deployment guidelines.
