# SPDX-License-Identifier: Apache-2.0
#
# Thin wrapper around west. Activate the workspace venv first:
#   source ~/zephyrproject/.venv/bin/activate
#
# EXTRA_CONF is merged *on top of* prj.conf. Leave it empty for a release
# build:  make build EXTRA_CONF=
# (Do not use Zephyr's CONF_FILE variable here: it replaces prj.conf.)

BOARD      ?= rpi_pico2/rp2350a/m33_0
RUNNER     ?= jlink
EXTRA_CONF ?= debug.conf
BUILD_DIR  ?= build

ifeq ($(strip $(EXTRA_CONF)),)
EXTRA_ARGS :=
else
EXTRA_ARGS := -DEXTRA_CONF_FILE=$(EXTRA_CONF)
endif

.PHONY: all build flash debug attach rtt menuconfig pristine ram-report rom-report clean

all: build

build:
	west build -p always -d $(BUILD_DIR) -b $(BOARD) . -- \
		$(EXTRA_ARGS) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

flash:
	west flash -d $(BUILD_DIR) --runner $(RUNNER)

debug:
	west debug -d $(BUILD_DIR) --runner $(RUNNER) --tui

attach:
	west attach -d $(BUILD_DIR) --runner $(RUNNER)

rtt:
	west rtt -d $(BUILD_DIR) --runner $(RUNNER)

menuconfig:
	west build -d $(BUILD_DIR) -t menuconfig

pristine:
	west build -d $(BUILD_DIR) -t pristine

ram-report:
	west build -d $(BUILD_DIR) -t ram_report

rom-report:
	west build -d $(BUILD_DIR) -t rom_report

clean:
	rm -rf $(BUILD_DIR)
