/*
  ******************************************************************************
  * @file    w25q128jvsiq.h
  * @brief   Simple OSPI driver header for W25Q128JVSIQ NOR Flash in indirect/autopoll mode
  * @details NOR Flash spiFlash, 3V, 128M-bit, 4Kb Uniform Sector,
  *          with QE = 1 (fixed) in Status register-2
  ******************************************************************************
  * @author christal
  ******************************************************************************
  * Recommended OCTOSPI peripheral setup example /w 160MHz OSPI clock and QSPI mode:
  * hospi1.Instance = OCTOSPI1;
  * hospi1.Init.FifoThreshold = 1;
  * hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
  * hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
  * hospi1.Init.DeviceSize = 24;
  * hospi1.Init.ChipSelectHighTime = 1;
  * hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
  * hospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
  * hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
  * hospi1.Init.ClockPrescaler = 159;
  * hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  * hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
  * hospi1.Init.ChipSelectBoundary = 0;
  * hospi1.Init.MaxTran = 0;
  * hospi1.Init.Refresh = 0;
  */

#ifndef INC_W25Q128JVSIQ_H_
#define INC_W25Q128JVSIQ_H_

/* Public functions exposed to loader calls */
HAL_StatusTypeDef W25Q128JV_Init(OSPI_HandleTypeDef* ospiHandle);
HAL_StatusTypeDef W25Q128JV_MassEraseSeq(OSPI_HandleTypeDef* ospiHandle);
HAL_StatusTypeDef W25Q128JV_EraseSect4k(OSPI_HandleTypeDef* ospiHandle, uint32_t EraseStartAddress, uint32_t EraseEndAddress);
HAL_StatusTypeDef W25Q128JV_WritePage(OSPI_HandleTypeDef* ospiHandle, uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
HAL_StatusTypeDef W25Q128JV_ReadBytes(OSPI_HandleTypeDef* ospiHandle,uint8_t* pData, uint32_t ReadAddr, uint32_t Size);

/* OSPI Peripheral Parameters */
#define MEMORY_AUTOPOLL_INTVL 0x10      //OSPI autopolling hw repeat rate

/*Winbond W25Q128JV Parameters*/
#define MEMORY_FLASH_SIZE  0x1000000  //128 MBits = 16MBytes
#define MEMORY_SECTOR_SIZE 0x1000     //4096 total sectors of 4kBytes
#define MEMORY_PAGE_SIZE   0x100      //65536 total pages of 256 bytes
#define MEMORY_ADDR_MSK    0xFFFFFFUL //Mask the memory mapped address MSB passed from CubeProg to 24bit flash addr

/* Winbond W25Q128JV Commands */
#define READ_JEDEC_ID_CMD 0x9F
#define JEDEC_ID_MFG 0xEF
#define JEDEC_ID_TYPE 0x40
#define JEDEC_ID_CAP 0x18

#define RESET_ENABLE_CMD 0x66
#define RESET_DEVICE_CMD 0x99

#define WRITE_ENABLE_CMD 0x06
#define READ_SR1_CMD 0x05

#define MASS_ERASE_CMD 0xC7
#define SECTOR4_ERASE_CMD 0x20

#define QUAD_FAST_READ_CMD 0xEB
#define QUAD_IN_PAGE_PROG_CMD 0x32
#define QUAD_FAST_READ_DUMMY_CYC 4

#endif /* INC_W25Q128JVSIQ_H_ */
