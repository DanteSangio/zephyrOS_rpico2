# `boards/` — per-board configuration

Files here are picked up automatically by `west build`, but only when the
filename matches the **full qualified board target**, with `/` replaced by `_`.

For the Pico 2 running Zephyr on the Cortex-M33 core, the target is
`rpi_pico2/rp2350a/m33_0`, so the valid names are:

```
boards/rpi_pico2_rp2350a_m33_0.conf      # extra Kconfig, merged after prj.conf
boards/rpi_pico2_rp2350a_m33_0.overlay   # devicetree overlay
```

A short name such as `rpi_pico2.conf` is silently ignored — this board has
qualifiers (`rp2350a`, `m33_0`), so they are part of the filename.

For the RISC-V core, use `rpi_pico2_rp2350a_hazard3_0.*`.

Notes:

- A matching `boards/<target>.overlay` replaces `app.overlay`; the two are
  not additive. Use `-DEXTRA_DTC_OVERLAY_FILE=<file>.overlay` if you need a
  board overlay plus a shared one.
- Only create these files when they contain something. Empty placeholders are
  noise.

Example — enable I2C1 on GP6/GP7 using the pinctrl group the board already
defines (`zephyr/boards/raspberrypi/common/rpi_pico-pinctrl-common.dtsi`):

```dts
/* boards/rpi_pico2_rp2350a_m33_0.overlay */
&i2c1 {
	status = "okay";
	pinctrl-0 = <&i2c1_default>;
	pinctrl-names = "default";
	clock-frequency = <100000>;
};
```

with `CONFIG_I2C=y` in `prj.conf`.
