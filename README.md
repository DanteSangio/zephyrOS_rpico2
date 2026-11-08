# pico2-app — Zephyr on the Raspberry Pi Pico 2

Application repository for the RP2350-based Raspberry Pi Pico 2, built on
Zephyr 4.5. It starts as a blinking user LED with logging and is meant to be
extended.

Toolchain installation, J-Link wiring, and host setup are documented in the
workspace notes: [`~/zephyrproject/README.md`](../../README.md).

| | |
|---|---|
| Board target | `rpi_pico2/rp2350a/m33_0` (Cortex-M33) |
| Debug probe | SEGGER J-Link Base over SWD |
| Console + shell | native USB (CDC-ACM), `/dev/ttyACM*` |
| Application logs | UART0 — GP0 = TX, GP1 = RX, 115200 8N1, `/dev/ttyUSB*` |
| User LED | `led0` alias, GPIO25 |
| Zephyr | 4.5.0 |

## Quick start

```bash
cd ~/zephyrproject/apps/pico2-app
source ~/zephyrproject/.venv/bin/activate

make build      # west build -p always -b rpi_pico2/rp2350a/m33_0 .
make flash      # west flash --runner jlink
make debug      # west debug --runner jlink --tui
```

`make build` uses `debug.conf`, which enables GDB thread awareness, `-Og`
optimisation, and the RTT log backend. Build a plain release image with:

```bash
make build EXTRA_CONF=    # prj.conf only
```

Do not use Zephyr's `CONF_FILE` CMake variable for this: it *replaces*
`prj.conf` instead of adding to it.

## Layout

```
.
├── CMakeLists.txt                    # app entry point, lists the sources
├── prj.conf                          # application Kconfig (all boards)
├── debug.conf                        # debug-only Kconfig fragment
├── boards/                           # per-board .conf/.overlay, see boards/README.md
├── src/
│   ├── main.c                        # boot, logging, calls into the app
│   ├── app.c                         # application logic
│   └── app.h
├── tests.yaml                        # twister metadata
├── Makefile                          # shortcuts around west
└── .editorconfig, .clang-format      # Zephyr coding style
```

The repository is a **Zephyr application**, not a workspace: `west` finds the
workspace by walking up to `~/zephyrproject/.west`. Never edit
`~/zephyrproject/zephyr` or `~/zephyrproject/modules` — configure through
`prj.conf` and devicetree overlays instead.

## Configuration

- **Kconfig** — `prj.conf` holds options common to every board. The board
  defconfig already enables `CONSOLE`, `SERIAL`, `UART_CONSOLE`, `GPIO`,
  `CLOCK_CONTROL`, `RESET`, and the UF2/HEX outputs; don't repeat them.
- **Board-specific Kconfig** — `boards/rpi_pico2_rp2350a_m33_0.conf`.
- **Devicetree** — `boards/rpi_pico2_rp2350a_m33_0.overlay` (or `app.overlay`
  as a fallback). The overlay is merged on top of the board DTS.
- **Extra fragments** — `west build ... -- -DEXTRA_CONF_FILE="a.conf;b.conf"`
  and `-DEXTRA_DTC_OVERLAY_FILE=...`.

Adding a source file means adding it to `target_sources(app PRIVATE ...)` in
`CMakeLists.txt`. Adding a header next to the `.c` file needs nothing.

## Debugging

```bash
west debug --runner jlink                                   # flash, halt at reset, GDB
west debug --runner jlink --tui                             # source + register views
west debug --runner jlink --gdb-init "break main" --gdb-init "continue"
west attach --runner jlink                                  # no flash, no reset
west rtt --runner jlink                                     # RTT console
```

`west debug` stops at the reset vector; start with `break main` then
`continue`. With `debug.conf`, the runner reports `thread info enabled` and GDB
understands Zephyr threads:

```gdb
info threads
thread apply all bt
```

A full GDB cheat sheet is in the workspace README.

## Serial ports

Two independent channels are available at the same time:

| Channel | Port | Carries |
|---|---|---|
| Console + shell | Pico 2 native USB, `/dev/ttyACM*` | `printk`, shell prompt and commands |
| Log output | UART0 on GP0/GP1, `/dev/ttyUSB*` | `LOG_*` records from the log subsystem |

### USB console

```bash
ls /dev/ttyACM*                # a phone modem may already own ACM0
tio /dev/ttyACM1 -b 115200     # baud is irrelevant for CDC-ACM
```

If you attach after boot, the prompt was already written and dropped: press
Enter. Expected:

```
uart:~$ kernel version
Zephyr version 4.5.0
uart:~$
```

The device enumerates as `2fe3:0004` with product string `pico2-app console`.

### UART0 logs

Wire an external USB-UART adapter — **3.3 V logic, RP2350 pins are not 5 V
tolerant**:

| Adapter | Pico 2 pin | Signal |
|---|---|---|
| GND | 3 | GND |
| TXD | 2 | GP1 / UART0 RX |
| RXD | 1 | GP0 / UART0 TX |
| VCC | — | not connected; power the Pico over its own USB |

```bash
stty -F /dev/ttyUSB0 115200 raw -echo
cat /dev/ttyUSB0
# or: tio /dev/ttyUSB0 -b 115200
```

Expected:

```
[00:00:00.000,000] <inf> pico2_app: booting on rpi_pico2/rp2350a/m33_0
[00:00:00.001,000] <inf> pico2_app: app_init() done, entering main loop
[00:00:00.002,000] <inf> pico2_app: heartbeat 0: LED ON
```

### How the split is configured

`boards/rpi_pico2_rp2350a_m33_0.overlay` assigns one role per port:

```dts
chosen {
	zephyr,console    = &cdc_acm_uart0;  /* printk + console */
	zephyr,shell-uart = &cdc_acm_uart0;  /* interactive shell */
	zephyr,log-uart   = &log_uarts;      /* LOG_* records */
};
```

`zephyr,log-uart` points at a container node with `uarts = <&uart0>;`. Without
it the UART log backend follows `zephyr,console`, which is why plain Zephyr can
only log where the console is. Adding more phandles to `uarts` duplicates the
log stream onto additional UARTs.

`CONFIG_SHELL_LOG_BACKEND=n` keeps log records out of the USB shell stream, so
the two ports stay independent. Set it to `y` to also see logs inside the
shell.

### Permissions

`/dev/ttyACM*` and `/dev/ttyUSB*` are group `uucp` on Arch. If `tio` reports
"Permission denied", the login session predates `usermod -aG uucp`: run
`newgrp uucp` in that terminal, or log out and back in.

## Testing

`tests.yaml` describes a twister test case that checks the heartbeat output:

```bash
cd ~/zephyrproject
.venv/bin/west twister -T apps/pico2-app -p rpi_pico2/rp2350a/m33_0 \
  --device-testing --device-serial /dev/ttyUSB0
```

The test matches `LOG_INF` records, so it reads UART0, not the USB console.
The Pico 2 is not emulated, so this needs the board attached; drop the
`--device-testing` flags for a build-only check.

## License

Apache-2.0, see [LICENSE](LICENSE). Each source file carries an SPDX header.
