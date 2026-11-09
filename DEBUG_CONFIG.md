# Debug and serial configuration

How `printk`, the interactive shell, and `LOG_*` records are routed on this
board, and how to move them between USB and UART0. Read this before changing
`prj.conf`, `debug.conf`, or the board overlay.

## Current setup

| Role | Device | Set by |
|---|---|---|
| Console (`printk`, `printf`) | `cdc_acm_uart0` — native USB | `zephyr,console` in the overlay |
| Shell prompt and commands | `cdc_acm_uart0` — native USB | `zephyr,shell-uart` + `CONFIG_SHELL_BACKEND_SERIAL` |
| Application logs | `uart0` — GP0 = TX, GP1 = RX, 115200 8N1 | `zephyr,log-uart` + `CONFIG_LOG_BACKEND_UART` |
| Debug log copy | RTT (J-Link) | `debug.conf` → `CONFIG_LOG_BACKEND_RTT` |

`make build` uses `debug.conf`, so debug builds emit logs on both UART0 and
RTT. `make build EXTRA_CONF=` produces a release build with UART0 logs only.

## Configuration layers

| Layer | File | Use it for |
|---|---|---|
| Board defconfig | `~/zephyrproject/zephyr/boards/raspberrypi/rpi_pico2/*_defconfig` | Stock defaults; do not edit. Already enables `CONSOLE`, `SERIAL`, `UART_CONSOLE`, `UART_INTERRUPT_DRIVEN`, `GPIO`, `CLOCK_CONTROL`, `RESET`, HEX/UF2. |
| Application Kconfig | `prj.conf` | Options common to every board and build type. |
| Debug fragment | `debug.conf` | GDB thread awareness, `-Og`, RTT backend. This is the default `EXTRA_CONF`. |
| Board devicetree | `boards/rpi_pico2_rp2350a_m33_0.overlay` | `chosen` nodes and the `cdc_acm_uart0` node. |
| Optional fragments | `usb-only.conf` | Deltas layered on top of the above. |

Fragments are merged **on top of** `prj.conf` with
`-DEXTRA_CONF_FILE="a.conf;b.conf"`. Do **not** use Zephyr's `CONF_FILE`: it
replaces `prj.conf` instead of merging with it.

## Recipes

| Goal | Command |
|---|---|
| USB console + shell, UART0 logs, RTT (default debug) | `make build && make flash` |
| Same, release build (no RTT, no `-Og`) | `make build EXTRA_CONF= && make flash` |
| Everything on the USB cable, UART0 silent | `make build EXTRA_CONF="debug.conf;usb-only.conf" && make flash` |
| Logs also echoed into the USB shell (UART0 still active) | add `CONFIG_SHELL_LOG_BACKEND=y` to a fragment, plus `CONFIG_SHELL_BACKEND_SERIAL_TX_RING_BUFFER_SIZE=1024` |
| Change log verbosity | `CONFIG_LOG_DEFAULT_LEVEL` in `prj.conf`: `1` = error, `2` = warning, `3` = info (current), `4` = debug |
| Remove only the RTT copy | `make build EXTRA_CONF=` or a fragment with `CONFIG_LOG_BACKEND_RTT=n` |
| Inspect options interactively | `make menuconfig` (see the caveat below) |

### Everything on UART0 (no USB)

Not provided as a fragment. The pieces are: the board already selects `uart0`
for `zephyr,console` and `zephyr,shell-uart`, so remove the overlay's `chosen`
overrides and the `cdc_acm_uart0` node, disable the USB device stack in a
fragment (`CONFIG_USB_DEVICE_STACK_NEXT=n`, `CONFIG_CDC_ACM_SERIAL_*`), and
keep `CONFIG_LOG_BACKEND_UART=y` with the `zephyr,log-uart` node. Build and
test it before relying on it.

## Rules that bite

1. **`chosen` nodes are only defaults.** `zephyr,shell-uart` does nothing until
   `CONFIG_SHELL=y`. `SHELL_BACKEND_SERIAL` then defaults to the device named
   by that node.

2. **`SHELL_LOG_BACKEND` defaults to `y` whenever `CONFIG_LOG=y`.** If the shell
   is enabled, its serial backend also flips the default of
   `LOG_BACKEND_UART` to `n`. That is why a stock Pico 2 with `LOG=y` and
   `SHELL=y` prints logs through the shell prompt on UART0. This application
   overrides both to keep the two ports independent:

   ```kconfig
   CONFIG_SHELL_LOG_BACKEND=n
   CONFIG_LOG_BACKEND_UART=y
   ```

3. **The UART log backend follows the console unless told otherwise.** If no
   `zephyr,log-uart` node exists, `LOG_BACKEND_UART` writes to
   `zephyr,console`. The overlay points it at `uart0` with:

   ```dts
   zephyr,log-uart = &log_uarts;
   log_uarts: log_uarts {
       compatible = "zephyr,log-uart";
       uarts = <&uart0>;
   };
   ```

   Adding more phandles to `uarts` duplicates the log stream onto more UARTs.

4. **Deferred is the safe log mode with USB.** `debug.conf` sets
   `CONFIG_LOG_MODE_IMMEDIATE=y`. Combined with the shell log backend, that
   drives `cdc_acm_poll_out()` from the caller's context; without echo
   mitigation no CDC TX work is scheduled and bytes are dropped, so records
   arrive truncated and the prompt disappears. `usb-only.conf` forces
   `CONFIG_LOG_MODE_DEFERRED=y`.

5. **The shell TX ring must fit one record plus the prompt.** The 128-byte ring
   in `prj.conf` is enough for the two-port split, but not for a colorized log
   line plus the prompt redraw. The single-cable fragment uses 1024 bytes.

6. **`make menuconfig` is not persistent.** It edits `build/zephyr/.config`
   only. `make build` pristines the build directory, so the changes are lost.
   Copy any value you want to keep into `prj.conf` or a fragment.

## Verify what you built

```bash
grep -E '^CONFIG_(SHELL|LOG|LOG_BACKEND|LOG_MODE|CONSOLE|UART_CONSOLE|PRINTK)' \
    build/zephyr/.config
grep -A6 'chosen {' build/zephyr/zephyr.dts
ls -l /dev/ttyACM* /dev/ttyUSB*
```

Then open the port that carries the shell (`tio /dev/ttyACM1 -b 115200` for
USB, `/dev/ttyUSB0` for UART0). Baud rate is irrelevant for CDC-ACM. If you
attach after boot, the prompt was written before the host opened the port:
press Enter once.

Expected shell reply:

```
uart:~$ kernel version
Zephyr version 4.5.0
uart:~$
```

## Single-cable mode

`usb-only.conf` moves the log stream into the USB shell so one cable carries
console, shell, and logs. It changes four settings:

```kconfig
CONFIG_SHELL_LOG_BACKEND=y
CONFIG_LOG_BACKEND_UART=n
CONFIG_LOG_MODE_DEFERRED=y
CONFIG_SHELL_BACKEND_SERIAL_TX_RING_BUFFER_SIZE=1024
```

No devicetree or source change is needed; the overlay's `zephyr,log-uart` node
is inert once the UART backend is compiled out.

Rollback is the default build:

```bash
make build          # debug.conf only
make flash
```

Caveats of single-cable mode:

- Boot output and early records are lost, because they are written before the
  host opens the CDC port.
- Panic output now depends on USB. In `debug.conf` builds RTT remains a second
  sink, so `west rtt` still works if USB itself is what failed.
- The shell redraws its prompt after each log record, so an unlucky keystroke
  can momentarily garble the line being edited. It recovers and the command
  executes.
- `/dev/ttyACM*` is assigned by the host and can change after a re-plug.

## Stock Pico 2 defaults

For reference, a bare application with an empty `prj.conf` gets:

- `CONSOLE=y`, `SERIAL=y`, `UART_CONSOLE=y`, `UART_INTERRUPT_DRIVEN=y`
- `zephyr,console = &uart0` and `zephyr,shell-uart = &uart0`, 115200 8N1
- `CONFIG_PRINTK=y`, so `printk` output appears on UART0
- `CONFIG_LOG` **off** — `LOG_*` calls compile out
- `CONFIG_SHELL` **off** — no `uart:~$` prompt
- no USB console; CDC-ACM requires the overlay and USB stack enabled here
