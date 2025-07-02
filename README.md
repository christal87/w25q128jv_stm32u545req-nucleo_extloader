# W25Q128JV_STM32U545REQ-NUCLEO_EXTLOADER
STM32CubeProg external loader for a Winbond W25Q128JVSIQ connected to a Nucleo-U545RE-Q board through it's octo SPI interface configured in quad SPI mode. The release build config is set up to drop the resulting stldr file in the project root directory. It has been tested via the board's built-in STLINK/V3 debugger against CubeProgrammer v2.19.0.

*Since this Winbond NOR flash is the Q variant it comes with QE = 1 (fixed) in Status register-2. It's already set to be used in quad mode. As this has a limited number of pins the /HOLD function is disabled to support Standard, Dual and Quad I/O without user setting.

The octo SPI interface clock output prescaler is deliberately set to 159 to slow down the flash to 1MHz (@160MHz interface clock) to avoid signal integrity issues while using jumper wires.  
OCTOSPI1 GPIO Configuration:  

| Nucleo Pin | MCU Pin | Flash Pin    |
| ---------: | :-----: | :----------: |
| CN7 - 12   | N/A     | VCC          |
| CN7 - 20   | N/A     | GND          | 
| CN7 - 28   | PA0     | OCTOSPI1_NCS |
| CN10 - 37  | PA3     | OCTOSPI1_CLK |
| CN10 - 13  | PA6     | OCTOSPI1_IO3 |
| CN10 - 15  | PA7     | OCTOSPI1_IO2 |
| CN7 - 34   | PB0     | OCTOSPI1_IO1 |
| CN10 - 24  | PB1     | OCTOSPI1_IO0 |

## CLI scripting to automate in production
The upload.sh script is made to call CubeProg CLI to erase, write then verify the flash contents from a binary file. It reads back the flash in chunks and does a comparison since CubeProg couldn't handle large amounts of data at once. There are variables to change this file's path (`SOURCE_FILE`), loader's path (`EXT_LOADER`) and the CLI's path (`CUBEPROG_CLI`).

Usage:
```
./upload.sh
```
Calling for a mass erase directly via CubeProg CLI if needed:
```
./STM32_Programmer.sh -vb 3 -c port=swd -e all -el ~/W25Q128JV_STM32U545REQ-NUCLEO_EXTLOADER.stldr
```

## Notes
- I left the map file in the release directory so that the final linking of the loader can be studied.
- The M variant of the flash would need extra functionality to set the QE bit in SR2 but it seems like the JEDEC ID verification would also need to change. According to the data sheet that variant received a new device ID.
- The loader basically gets uploaded and resides in SRAM and has a section allocated for an input/output buffer to exchange data. CubeProg calls Init() and then the desired command's function. It all works like a dynamic library with multiple entry points. Before/after calls CubeProg checks the function return values in R0 register and also the contents of the program status register (xPSR).
- STM32CubeIDE Version: 1.12.0 Build: 14980_20230301_1550 (UTC)
- STM32 Toolchain: GNU Tools for STM32 (7-2018-q2-update) Version: 2.0.100.202301271415
- Firmware package: STM32Cube_FW_U5_V1.2.0
