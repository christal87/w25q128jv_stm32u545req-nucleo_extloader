/*
 * Dev_Inf.c
 *
 *  Created on: May 26, 2025
 *      Author: ST, christal
 */

#include "Dev_Inf.h"
#include "octospi.h"

/*
 * This structure contains information used by ST-LINK/CubeProgrammer Utility to program and erase the device.
 * At the time of writing CubeProgrammer's loader list doesn't take the Device Name string from this struct
 * like before. It falls back to parsing the *.stldr file name separated by an underscore: <flashPN>_<boardPN>.
 */
struct StorageInfo const StorageInfo = {
    "W25Q128JV_STM32U545REQ-NUCLEO_EXTLOADER",         // Device Name + version number
	NOR_FLASH,                                         // Device Type
    0x90000000,                                        // Device Start Address
    MEMORY_FLASH_SIZE,                                 // Device Size in Bytes
    MEMORY_PAGE_SIZE,                                  // Programming Page Size
    0xFF,                                              // Initial Content of Erased Memory

    /* Specify Size and Address of Sectors */
    {
    	{ (MEMORY_FLASH_SIZE / MEMORY_SECTOR_SIZE), (uint32_t)MEMORY_SECTOR_SIZE }, // 4096 count of 4096byte sized sectors
        { 0x00000000                              , 0x00000000                   }  // End of size list
    }
};

