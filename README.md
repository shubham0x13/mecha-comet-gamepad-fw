# Mecha Comet Gamepad Extension - Zephyr Firmware

[Zephyr](https://zephyrproject.org) firmware for the **Mecha Comet Gamepad Extension**, the gamepad
of the Mecha Comet. It attaches to the Comet with magnetic pogo pins and also works on its own over
USB-C as a gamepad on any USB host. It has 14 buttons with a D-pad, a left analog stick and a Mecha
Pivot Pip pointing stick as the right stick.

Full documentation is coming.

## Building

The firmware is a Zephyr application in a
[west](https://docs.zephyrproject.org/latest/develop/west/index.html) workspace. This repository is
its manifest: `west update` fetches Zephyr, the Pivot Pip module and the other modules next to it,
and `west patch apply` applies the patches listed in [`zephyr/patches.yml`](zephyr/patches.yml).

You need the host tools of the
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).
As in that guide, the Python packages go into a
[virtual environment](https://docs.python.org/3/library/venv.html), so they stay apart from your
system Python. Set up the workspace once:

```sh
mkdir mecha-comet-gamepad-ws
cd mecha-comet-gamepad-ws
python3 -m venv .venv
source .venv/bin/activate
pip install west
west init -m https://github.com/mecha-org/mecha-comet-gamepad-fw
west update
west packages pip --install
west patch apply
west zephyr-export
west sdk install -t arm-zephyr-eabi
```

`west sdk install` installs the Zephyr SDK version that this Zephyr needs, with only the ARM
toolchain. If you already cloned the repository, run `west init -l mecha-comet-gamepad-fw` in the
folder that holds the clone instead of `west init -m`.

In every new terminal, activate the environment again with `source .venv/bin/activate` in the
workspace folder. Build from the workspace folder. The result is `build/zephyr/zephyr.bin`:

```sh
west build -p -b mecha_comet_gamepad mecha-comet-gamepad-fw/app
```

- Without the Pivot Pip or the left stick board, add `-S no-pivot-pip` or `-S no-left-stick`.
- `west twister -T mecha-comet-gamepad-fw/app --integration` builds all of these, as CI does.

After `west update`, run `west patch clean` and `west patch apply` again.

## Flashing

The MCXC243 has a USB bootloader in its ROM, so no debug probe is needed. Start it in one of two
ways:

- On a running gamepad, hold Select + Start + LB + RB for 3 seconds.
- Hold the BOOT pad (PTA4) low while plugging in the gamepad.

The gamepad then shows up on USB as `15a2:0073`. Flash it with `blhost` from NXP's
[SPSDK](https://pypi.org/project/spsdk) (`pip install spsdk`):

```sh
blhost -u 0x15a2:0x0073 flash-erase-region 0x0 0x1e000
blhost -u 0x15a2:0x0073 write-memory 0x0 build/zephyr/zephyr.bin
blhost -u 0x15a2:0x0073 reset
```

This erases only the firmware, the first 120 KB. The last 8 KB keep the settings, such as the left
stick calibration; `flash-erase-all` would erase them too. On Linux, `blhost` needs access to the
device: run it with `sudo`, or add a udev rule such as
`SUBSYSTEM=="hidraw", ATTRS{idVendor}=="15a2", ATTRS{idProduct}=="0073", TAG+="uaccess"`.

## Using the gamepad

The buttons and sticks follow the Linux gamepad layout, so Linux, SDL and Steam know them without
a controller mapping.

### Status LED

| LED                | Meaning                                                   |
| ------------------ | --------------------------------------------------------- |
| Slow blink         | Waiting for the host                                      |
| On                 | Connected                                                 |
| Off                | The host sleeps, no power, or the USB bootloader          |
| Two short blinks   | Right stick recalibrated                                  |
| Fast blink         | Left stick calibration: release the stick                 |
| Double blink       | Left stick calibration: turn the stick all the way round  |
| Three short blinks | Calibration saved                                         |
| One long blink     | Calibration cancelled or failed, nothing changed          |
| Very fast blink    | Factory reset, or restart into the USB bootloader         |

### Button combinations

| Buttons                            | Hold  | Does                                           |
| ---------------------------------- | ----- | ---------------------------------------------- |
| L3 + Home                          | 1.5 s | Recentres the right stick                      |
| Select + Start + LB + RB           | 3 s   | Restarts into the USB bootloader               |
| L3 + Select, at power-up           | 1 s   | Starts the left stick calibration              |
| Select + Start + Home, at power-up | 5 s   | Factory reset: erases the settings, restarts   |

"At power-up" means held while plugging in the gamepad: these only start within the first second.
Do not touch the right stick while it is recalibrated.

### Left stick calibration

1. Hold L3 + Select while plugging in the gamepad, then release everything. The rest position of
   the stick is measured.
2. When the LED changes to a double blink, turn the stick all the way round a few times.
3. Press A to save, or B to cancel.

The calibration is stored and used at every start, until the next calibration or a factory reset.

## Local development

To change Pivot Pip together with the gamepad, work in `modules/lib/mecha-pivot-pip`. It is a
normal git checkout, and a build uses what is in it, committed or not. Use a branch there, and run
`west update -k` so that west keeps it instead of moving back to the revision of `west.yml`. To use
a clone somewhere else, add `-DEXTRA_ZEPHYR_MODULES=/path/to/mecha-pivot-pip-zephyr` to the build.
It replaces the copy in the workspace.

## License

Licensed under the [BSD 3-Clause license](LICENSE).
