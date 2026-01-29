# Architecture

## Pin abstraction rules

- Use direct substitutions (e.g. `pin: ${pin_buzzer}`) when the pin schema is always the same (ESP32 GPIO everywhere).
- Use variant packages (`*_gpio.yaml` vs `*_tca9555.yaml`) when the same **function** can be routed to different controllers (ESP32 GPIO vs TCA9555).
- Avoid dynamic `!include ${...}` paths: ESPHome does not expand substitutions inside include filenames.

## Layers

- `boards/<device>/rX.yaml`: only substitutions (pins, addresses, feature flags).
- `devices/<device>_rX.yaml`: selects which packages to include for that board.
- `packages/*`: reusable blocks (buttons, LEDs, UARTs, add-ons, ethernet, etc.).
