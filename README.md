# RISCbot

Source files for the LoFive RISC-V RISCbot

## Building software

This project leverages the freedom-e-sdk bsp, and can use a toolchain that is
part of the freedom-e-sdk, or an external toolchain. You must define the
following environment variables to build the project:

```
export BSP_BASE=/my/desired/location/freedom-e-sdk/bsp
export RISCV_OPENOCD_PATH=/my/desired/location/openocd
export RISCV_PATH=/my/desired/location/riscv64-unknown-elf-gcc-<date>-<version>
```

After this, you can build the software as if you were building an example from the sdk:

```
make software upload PROGRAM=riscbot BOARD=freedom-e300-lofive
```

## Hardware design files

The directory `pcb` contains the KiCAD schematics used to prototype RISCbot.
Also included is a board layout that is complete to the best of my knowledge,
but please note that I have not actually fabricated this PCB as RISCbot was
hand-wired on a perforated circuit board.
