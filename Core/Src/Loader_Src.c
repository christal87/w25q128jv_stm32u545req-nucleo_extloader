/*
 * Loader_Src.c
 *
 *  Created on: May 26, 2025
 *      Author: ST, christal
 */

#include "octospi.h"
#include "main.h"
#include "gpio.h"

#define LOADER_OK   0x1
#define LOADER_FAIL 0x0

extern void SystemClock_Config(void); //use the generated HAL clock config function

/*
 * @brief   Simple delay function for the loader
 * @details Since version 2.7.0, the use of interrupts (including SysTick) is not allowed anymore
 * in custom external loaders. The solution is to overload the default HAL Tick or implement
 * your own custom Delay function (if needed) by adding the following code to your project.
 * https://community.st.com/t5/stm32-mcus/custom-external-loader-quot-failed-to-download-segment-0-quot/ta-p/49307
 * @param   Delay : Block for f(MHz) * MULTIPLIER
 * @retval  None
 */
/*void Loader_Delay(uint32_t Delay) {
#define MULTIPLIER 100
//uint32_t clock; 		//MCU system clock in MHz

	uint32_t clock=HAL_RCC_GetSysClockFreq()/1000000; //MCU system clock in MHz

	for (uint32_t j=0; j<Delay; j++) { //Blocking
		for (uint32_t k=0; (k<(clock*MULTIPLIER)); k++) {};
	}
}*/

/*
 * @brief   Overloaded __weak HAL_Delay
 * @details Since version 2.7.0, the use of interrupts (including SysTick) is not allowed anymore
 * in custom external loaders. The solution is to overload the default HAL Tick or implement
 * your own custom Delay function (if needed) by adding the following code to your project.
 * https://community.st.com/t5/stm32-mcus/custom-external-loader-quot-failed-to-download-segment-0-quot/ta-p/49307
 * @param   Delay : Block for f(MHz) * MULTIPLIER
 * @retval  None
 */
/*void HAL_Delay(uint32_t Delay) {
	Loader_Delay(Delay);
}*/

/*
 * @brief   System initialization.
 * @param   None
 * @retval  LOADER_OK = R0=1   : Operation succeeded
 * @retval  LOADER_FAIL = R0=0 : Operation failed
 */
int Init(void) {

    /*
     *  DHCSR@0xE000EDF0 register write time bit assignments to enable interrupts in debug mode
     *
     *  [31:16] DBGKEY  - 0xA05F = enable write access to bits [15:0]
     *  [15:7]	-	    - Reserved
     *  [6] C_PMOV      - 0 = No action
     *  [5] C_SNAPSTALL - 0 = No action
     *  [4]       -     - Reserved
     *  [3] C_MASKINTS  - 0 = Do not mask
     *  [2] C_STEP      - 0 = No effect
     *  [1] C_HALT      - 0 = No effect
     *  [0] C_DEBUGEN   - 0 = Disabled
     */
    //*(uint32_t*)0xE000EDF0 = 0xA05F0000;

    SystemInit();

    SCB->VTOR = 0x20000000 | 0x200; //Relocate vector table from flash to main SRAM@0x20000200

    HAL_DeInit();
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();

    __set_PRIMASK(0); //enable interrupts

    if (W25Q128JV_Init(&hospi1) != HAL_OK) {
    	__set_PRIMASK(1); //disable interrupts
        return LOADER_FAIL;
    }

    __set_PRIMASK(1); //disable interrupts
    return LOADER_OK;
}

/*
 * @brief   Program memory.
 * @param   Address: page address
 * @param   Size   : size of data
 * @param   buffer : pointer to data buffer
 * @retval  LOADER_OK = R0=1   : Operation succeeded
 * @retval  LOADER_FAIL = R0=0 : Operation failed
 */
int Write(uint32_t Address, uint32_t Size, uint8_t* buffer) {
	__set_PRIMASK(0); //enable interrupts

    if (W25Q128JV_WritePage(&hospi1, (uint8_t*) buffer, (Address & (MEMORY_ADDR_MSK)), Size) != HAL_OK) {
    	__set_PRIMASK(1); //disable interrupts
        return LOADER_FAIL;
    }

    __set_PRIMASK(1); //disable interrupts
    return LOADER_OK;
}

/*
 * @brief   Sector erase.
 * @param   EraseStartAddress :  erase start address
 * @param   EraseEndAddress   :  erase end address
 * @retval  LOADER_OK = R0=1   : Operation succeeded
 * @retval  LOADER_FAIL = R0=0 : Operation failed
 */
int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress) {
	__set_PRIMASK(0); //enable interrupts

    if (W25Q128JV_EraseSect4k(&hospi1, (EraseStartAddress & (MEMORY_ADDR_MSK)), (EraseEndAddress & (MEMORY_ADDR_MSK))) != HAL_OK) {
    	__set_PRIMASK(1); //disable interrupts
        return LOADER_FAIL;
    }

    __set_PRIMASK(1); //disable interrupts
    return LOADER_OK;
}

/*
 * @brief   Mass erase of external flash area
 * @details Optional command for all types of devices
 * @param   None
 * @retval  LOADER_OK = R0=1   : Operation succeeded
 * @retval  LOADER_FAIL = R0=0 : Operation failed
 */
int MassErase(void) {
	__set_PRIMASK(0); //enable interrupts

    if (W25Q128JV_MassEraseSeq(&hospi1) != HAL_OK) {
    	__set_PRIMASK(1); //disable interrupts
        return LOADER_FAIL;
    }

    __set_PRIMASK(1); //disable interrupts
    return LOADER_OK;
}

/*
 * @brief   Read data from the device using indirect mode
 * @details Optional command for all types of devices.
 * Either use this read function or implement and initialize memory mapping.
 * "UM2237: For Quad-/Octo-SPI memories, the memory mapped mode can be defined in
 * the Init function; in that case, the Read function is useless,
 * as data can be read directly from JTAG/SWD interface."
 * @param   Address       : Write location
 * @param   Size          : Length in bytes
 * @param   buffer        : Address where to get the data to write
 * @retval  LOADER_OK = R0=1   : Operation succeeded
 * @retval  LOADER_FAIL = R0=0 : Operation failed
 */
int Read (uint32_t Address, uint32_t Size, uint8_t* buffer) {
    __set_PRIMASK(0); //enable interrupts

    if (W25Q128JV_ReadBytes(&hospi1, buffer, (Address & (MEMORY_ADDR_MSK)), Size) != HAL_OK) { //do indirect read
    	__set_PRIMASK(1); //disable interrupts
		return LOADER_FAIL;
    }

	__set_PRIMASK(1); //disable interrupts
	return LOADER_OK;
}

