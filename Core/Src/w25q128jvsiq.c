/*
  ******************************************************************************
  * @file    w25q128jvsiq.c
  * @headerfile w25q128jvsiq.h "Inc/w25q128jvsiq.h"
  * @brief   Simple OSPI driver source for W25Q128JVSIQ NOR Flash in indirect/autopoll mode
  * @details NOR Flash spiFlash, 3V, 128M-bit, 4Kb Uniform Sector,
  *          with QE = 1 (fixed) in Status register-2
  ******************************************************************************
  * @author christal
  */

#include "main.h" //Includes types, macros and functions from the HAL
#include "octospi.h" //Includes the HAL generated OSPI init function
#include "w25q128jvsiq.h" //Driver macros and API functions

/* Private functions exposed to local calls */
static OSPI_RegularCmdTypeDef OSPI_GetDefaultParams(void);
static HAL_StatusTypeDef W25Q128JV_AutoPollMemReady(OSPI_HandleTypeDef* ospiHandle);
static HAL_StatusTypeDef W25Q128JV_WriteEna(OSPI_HandleTypeDef* ospiHandle);
static HAL_StatusTypeDef W25Q128JV_ResetSeq(OSPI_HandleTypeDef* ospiHandle);
static HAL_StatusTypeDef W25Q128JV_PollMemBusy(OSPI_HandleTypeDef* ospiHandle);
static HAL_StatusTypeDef W25Q128JV_ReadSR1(OSPI_HandleTypeDef* ospiHandle, uint8_t* register_data);
static HAL_StatusTypeDef W25Q128JV_VerifyJEDEC_ID(OSPI_HandleTypeDef* ospiHandle);

/*
 * @brief   Keep common OCTOSPI settings organized adhering to the DRY principle.
 * @param   None
 * @retval  OSPI_RegularCmdTypeDef struct
 */
static OSPI_RegularCmdTypeDef OSPI_GetDefaultParams(void) {

	OSPI_RegularCmdTypeDef cmd = {0}; //Start with an empty struct for safety

	cmd.OperationType          = HAL_OSPI_OPTYPE_COMMON_CFG; 			//Common configuration for indirect or auto-polling mode
	cmd.FlashId            	   = HAL_OSPI_FLASH_ID_1; 					//Set the OCTOSPI flash ID
	cmd.InstructionDtrMode 	   = HAL_OSPI_INSTRUCTION_DTR_DISABLE; 		//Disable instruction double xfer rate
	cmd.AddressDtrMode     	   = HAL_OSPI_ADDRESS_DTR_DISABLE; 			//Disable address double xfer rate
	cmd.DataDtrMode			   = HAL_OSPI_DATA_DTR_DISABLE; 			//Disable data double xfer rate
	cmd.DQSMode            	   = HAL_OSPI_DQS_DISABLE; 					//Disable data strobing
	cmd.SIOOMode          	   = HAL_OSPI_SIOO_INST_EVERY_CMD; 			//SIOO mode: Send instruction on every transaction
	cmd.AlternateBytesMode 	   = HAL_OSPI_ALTERNATE_BYTES_NONE; 		//No alternate bytes
	cmd.AlternateBytes		   = HAL_OSPI_ALTERNATE_BYTES_NONE; 		//No alternate bytes
	cmd.AlternateBytesSize	   = HAL_OSPI_ALTERNATE_BYTES_NONE; 		//No alternate bytes
	cmd.AlternateBytesDtrMode  = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE; 	//Disable alternate bytes double xfer rate
	cmd.InstructionMode   	   = HAL_OSPI_INSTRUCTION_1_LINE;			//Instruction on a single line
	cmd.InstructionSize    	   = HAL_OSPI_INSTRUCTION_8_BITS;			//8-bit instructions
	cmd.AddressSize 		   = HAL_OSPI_ADDRESS_24_BITS;				//24-bit addressing
	cmd.DummyCycles            = 0;                                     //Assume no dummy cycles

    return cmd;
}

/*
 * @brief   Initialialize the OCTOSPI peripheral and the flash for external loader operations.
 * @param   ospiHandle :pointer to HAL OCTOSPI handle
 * @retval  HAL_OK = 0    : Operation succeeded
 * @retval  HAL_ERROR = 1 : Operation failed
 */
HAL_StatusTypeDef W25Q128JV_Init(OSPI_HandleTypeDef* ospiHandle) {

	MX_OCTOSPI1_Init();

	if (W25Q128JV_ResetSeq(ospiHandle) != HAL_OK) {
	    return HAL_ERROR;
	}

	if (W25Q128JV_VerifyJEDEC_ID(ospiHandle) != HAL_OK) {
		return HAL_ERROR;
	}

	//HAL_Delay(1);

	if (W25Q128JV_AutoPollMemReady(ospiHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    if (W25Q128JV_WriteEna(ospiHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/*
 * @brief   Erase the whole flash in one go (which takes some time).
 * @param   ospiHandle : pointer to HAL OCTOSPI handle
 * @retval  HAL_OK = 0    : Operation succeeded
 * @retval  HAL_ERROR = 1 : Operation failed
 */
HAL_StatusTypeDef W25Q128JV_MassEraseSeq(OSPI_HandleTypeDef* ospiHandle) {

	//check if the flash is up for the task
    if (W25Q128JV_AutoPollMemReady(ospiHandle) != HAL_OK) {
        return HAL_ERROR;
    }

	OSPI_RegularCmdTypeDef sCommand = OSPI_GetDefaultParams();

	sCommand.Instruction 				= MASS_ERASE_CMD;

	sCommand.AddressMode       			= HAL_OSPI_ADDRESS_NONE;
	sCommand.Address					= 0;

	sCommand.DummyCycles       			= 0;
	sCommand.DataMode          			= HAL_OSPI_DATA_NONE;
	sCommand.NbData            			= 1;

    if (W25Q128JV_WriteEna(ospiHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    if (HAL_OSPI_Command(ospiHandle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    while (W25Q128JV_PollMemBusy(ospiHandle) == HAL_ERROR) {
    	HAL_Delay(1);
    }

    if (W25Q128JV_AutoPollMemReady(ospiHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/*
 * @brief   Check if the flash is in a ready state yet.
 * @details Uses the OSPI peripheral's autopoll hw to wait for MemReady
 * @param   ospiHandle : pointer to HAL OCTOSPI handle
 * @retval  HAL_OK = 0    : Operation succeeded
 * @retval  HAL_ERROR = 1 : Operation failed
 */
static HAL_StatusTypeDef W25Q128JV_AutoPollMemReady(OSPI_HandleTypeDef* ospiHandle) {

	OSPI_RegularCmdTypeDef sCommand = OSPI_GetDefaultParams();
    OSPI_AutoPollingTypeDef sConfig = {0};

	sCommand.Instruction 				= READ_SR1_CMD;

	sCommand.AddressMode       			= HAL_OSPI_ADDRESS_NONE;
	sCommand.Address					= 0;

	sCommand.DummyCycles       			= 0;

	sCommand.DataMode          			= HAL_OSPI_DATA_1_LINE;
	sCommand.NbData            			= 1;

    if (HAL_OSPI_Command(ospiHandle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Block and poll the status register until the busy bit is either cleared or a timeout has been reached */
    sConfig.Mask            			= 0x01U; //Mask all but the 1st bit of STATUS_REG1 (BUSY)
    sConfig.Match           			= 0x00U; //The first, BUSY bit should be set to 0
    sConfig.MatchMode       			= HAL_OSPI_MATCH_MODE_AND;
    sConfig.Interval        			= MEMORY_AUTOPOLL_INTVL;
    sConfig.AutomaticStop   			= HAL_OSPI_AUTOMATIC_STOP_ENABLE;

    if (HAL_OSPI_AutoPolling(ospiHandle, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}


/*
 * @brief   Enable a flash write operation.
 * @details Needed after every page write, sector erase
 * @param   ospiHandle : pointer to HAL OCTOSPI handle
 * @retval  HAL_OK = 0    : Operation succeeded
 * @retval  HAL_ERROR = 1 : Operation failed
 */
static HAL_StatusTypeDef W25Q128JV_WriteEna(OSPI_HandleTypeDef* ospiHandle) {

	OSPI_RegularCmdTypeDef sCommand = OSPI_GetDefaultParams();
    OSPI_AutoPollingTypeDef sConfig={0};

	sCommand.Instruction 				= WRITE_ENABLE_CMD;

	sCommand.AddressMode       			= HAL_OSPI_ADDRESS_NONE;
	sCommand.Address					= 0;

	sCommand.DummyCycles       			= 0;

	sCommand.DataMode          			= HAL_OSPI_DATA_NONE;
	sCommand.NbData            			= 0;

    if (HAL_OSPI_Command(ospiHandle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

	sCommand.Instruction 				= READ_SR1_CMD;

	sCommand.AddressMode       			= HAL_OSPI_ADDRESS_NONE;
	sCommand.Address					= 0;

	sCommand.DummyCycles       			= 0;

	sCommand.DataMode          			= HAL_OSPI_DATA_1_LINE;
	sCommand.NbData            			= 1;

    if (HAL_OSPI_Command(ospiHandle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Block and poll the status register until the write bit is either set or a timeout has been reached */
    sConfig.Mask 					= 0x02U; //Mask all but the 2nd bit of STATUS_REG1 (WEL)
    sConfig.Match 					= 0x02U; //The second, WEL bit should be set to 1
    sConfig.MatchMode 				= HAL_OSPI_MATCH_MODE_AND;
    sConfig.Interval 				= MEMORY_AUTOPOLL_INTVL;
    sConfig.AutomaticStop 			= HAL_OSPI_AUTOMATIC_STOP_ENABLE;


    if (HAL_OSPI_AutoPolling(ospiHandle, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/*
 * @brief   Erase a range of flash aligned in basic 4k sector sized units.
 * @param   ospiHandle        : pointer to HAL OCTOSPI handle
 * @param   EraseStartAddress : First memory address
 * @param   EraseEndAddress   : Last memory address
 * @retval  HAL_OK = 0    : Operation succeeded
 * @retval  HAL_ERROR = 1 : Operation failed
 */
HAL_StatusTypeDef W25Q128JV_EraseSect4k(OSPI_HandleTypeDef* ospiHandle, uint32_t EraseStartAddress, uint32_t EraseEndAddress) {

	//check if the flash is up for the task
    if (W25Q128JV_AutoPollMemReady(ospiHandle) != HAL_OK) {
        return HAL_ERROR;
    }

	OSPI_RegularCmdTypeDef sCommand = OSPI_GetDefaultParams();
    uint32_t StartAddress = 0;

    StartAddress = EraseStartAddress - (EraseStartAddress % MEMORY_SECTOR_SIZE);

    while (EraseEndAddress >= StartAddress) {
    	sCommand.Instruction 				= SECTOR4_ERASE_CMD;

    	sCommand.AddressMode       			= HAL_OSPI_ADDRESS_1_LINE;
    	sCommand.Address					= (StartAddress & MEMORY_ADDR_MSK);

    	sCommand.DataMode          			= HAL_OSPI_DATA_NONE;
    	sCommand.NbData            			= 0;

        if (W25Q128JV_WriteEna(ospiHandle) != HAL_OK) {
            return HAL_ERROR;
        }

        if (HAL_OSPI_Command(ospiHandle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
            return HAL_ERROR;
        }

        if (W25Q128JV_AutoPollMemReady(ospiHandle) != HAL_OK) {
            return HAL_ERROR;
        }

        StartAddress += MEMORY_SECTOR_SIZE;
    }

    return HAL_OK;
}

/*
 * @brief   Write an arbitrary length of data to the flash in basic 256 byte page sized units.
 * @param   ospiHandle : pointer to HAL OCTOSPI handle
 * @param   pData      : pointer to data array to write
 * @param   WriteAddr  : start address
 * @param   Size       : data array length
 * @retval  HAL_OK = 0    : Operation succeeded
 * @retval  HAL_ERROR = 1 : Operation failed
 */
HAL_StatusTypeDef W25Q128JV_WritePage(OSPI_HandleTypeDef* ospiHandle, uint8_t* pData, uint32_t WriteAddr, uint32_t Size) {

  //check if the flash is up for the task
  if (W25Q128JV_AutoPollMemReady(ospiHandle) != HAL_OK) {
	  return HAL_ERROR;
  }

  OSPI_RegularCmdTypeDef sCommand = OSPI_GetDefaultParams();
  uint32_t end_addr = 0, current_size = 0, current_addr = 0, data_addr = 0;

  while (current_addr <= WriteAddr) {
      current_addr += MEMORY_PAGE_SIZE;
  }

  current_size = current_addr - WriteAddr;

  if (current_size > Size) {
      current_size = Size;
  }

  current_addr = WriteAddr;
  end_addr = WriteAddr + Size;
  data_addr = (uint32_t)pData;

  do {
	sCommand.Instruction = QUAD_IN_PAGE_PROG_CMD;

	sCommand.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
	sCommand.Address     = current_addr;

	sCommand.DummyCycles = 0;

	sCommand.DataMode    = HAL_OSPI_DATA_4_LINES;
	sCommand.NbData      = current_size;

    if (current_size == 0) {
        return HAL_OK;
    }

    if (W25Q128JV_WriteEna(ospiHandle) != HAL_OK) {
      return HAL_ERROR;
    }

    if (HAL_OSPI_Command(ospiHandle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      return HAL_ERROR;
    }

    if (HAL_OSPI_Transmit(ospiHandle, (uint8_t*)data_addr, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      return HAL_ERROR;
    }

    if (W25Q128JV_AutoPollMemReady(ospiHandle) != HAL_OK) {
      return HAL_ERROR;
    }

    current_addr += current_size;
    data_addr += current_size;
    current_size = ((current_addr + MEMORY_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : MEMORY_PAGE_SIZE;
  } while (current_addr <= end_addr);

  return HAL_OK;
}

/*
 * @brief   Non memory mapped (byte level) fast read  of a predetermined size.
 * @param   ospiHandle : pointer to HAL OCTOSPI handle
 * @param   pData      : pointer to read data array to fill and return
 * @param   ReadAddr   : start address
 * @param   Size       : data array length
 * @retval  HAL_OK = 0    : Operation succeeded
 * @retval  HAL_ERROR = 1 : Operation failed
 */
HAL_StatusTypeDef W25Q128JV_ReadBytes(OSPI_HandleTypeDef* ospiHandle,uint8_t* pData, uint32_t ReadAddr, uint32_t Size) {

  //check if the flash is up for the task
  if (W25Q128JV_AutoPollMemReady(ospiHandle) != HAL_OK) {
	  return HAL_ERROR;
  }

  OSPI_RegularCmdTypeDef sCommand = OSPI_GetDefaultParams();

  sCommand.Instruction        = QUAD_FAST_READ_CMD;

  sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
  sCommand.Address            = ReadAddr;

  sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;

  sCommand.DummyCycles        = QUAD_FAST_READ_DUMMY_CYC;

  sCommand.NbData             = Size;

  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
  sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
  sCommand.AlternateBytes     = 0xFF;

  if (HAL_OSPI_Command(ospiHandle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Receive(ospiHandle, pData, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}


/*
 * @brief   Reset the whole flash using enable and execute in sequence.
 * @param   ospiHandle : pointer to HAL OCTOSPI handle
 * @retval  HAL_OK = 0    : Operation succeeded
 * @retval  HAL_ERROR = 1 : Operation failed
 */
static HAL_StatusTypeDef W25Q128JV_ResetSeq(OSPI_HandleTypeDef* ospiHandle) {

	OSPI_RegularCmdTypeDef sCommand = OSPI_GetDefaultParams();

	sCommand.Instruction = RESET_ENABLE_CMD;

	sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
	sCommand.Address     = 0;

	sCommand.DummyCycles = 0;

	sCommand.DataMode    = HAL_OSPI_DATA_NONE;
	sCommand.NbData      = 0;


	if (HAL_OSPI_Command(ospiHandle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return HAL_ERROR;
	}

	sCommand.Instruction = RESET_DEVICE_CMD;

	sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
	sCommand.Address     = 0;

	sCommand.DummyCycles = 0;

	sCommand.DataMode    = HAL_OSPI_DATA_NONE;
	sCommand.NbData      = 0;

	if (HAL_OSPI_Command(ospiHandle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return HAL_ERROR;
	}

	return HAL_OK;
}

/*
 * @brief   Flash busy wait after a full erase operation.
 * @param   ospiHandle : pointer to HAL OCTOSPI handle
 * @retval  HAL_OK = 0    : Operation succeeded
 * @retval  HAL_ERROR = 1 : Operation failed
 */
static HAL_StatusTypeDef W25Q128JV_PollMemBusy(OSPI_HandleTypeDef* ospiHandle)
{
	uint8_t sreg1 = {0};

	if (W25Q128JV_ReadSR1(ospiHandle, &sreg1) != HAL_OK) {
		return HAL_ERROR;
	}

	if ((sreg1 & 0b1) != HAL_OK) {
		return HAL_ERROR;
	}
	return HAL_OK;
}

/*
 * @brief   Read status register 1 and return it's value.
 * @param   ospiHandle    : pointer to HAL OCTOSPI handle
 * @param   register_data : pointer to SR1 data array to fill and return
 * @retval  HAL_OK = 0    : Operation succeeded
 * @retval  HAL_ERROR = 1 : Operation failed
 */
static HAL_StatusTypeDef W25Q128JV_ReadSR1(OSPI_HandleTypeDef* ospiHandle, uint8_t* register_data)
{
	OSPI_RegularCmdTypeDef sCommand = OSPI_GetDefaultParams();

    sCommand.Instruction = READ_SR1_CMD;

    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.Address     = 0;

    sCommand.DummyCycles = 0;

    sCommand.DataMode    = HAL_OSPI_DATA_1_LINE;
    sCommand.NbData      = 1;

    if (HAL_OSPI_Command(ospiHandle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    if (HAL_OSPI_Receive(ospiHandle, register_data, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

	return HAL_OK;
}

/*
 * @brief   Makes sure the flash responds to commands and the correct make/type/size is being used.
 * @param   ospiHandle    : pointer to HAL OCTOSPI handle
 * @retval  HAL_OK = 0    : Operation succeeded
 * @retval  HAL_ERROR = 1 : Operation failed
 */
static HAL_StatusTypeDef W25Q128JV_VerifyJEDEC_ID(OSPI_HandleTypeDef* ospiHandle) {

  OSPI_RegularCmdTypeDef sCommand = OSPI_GetDefaultParams();
  uint8_t id[3] = {0};

  sCommand.Instruction = READ_JEDEC_ID_CMD;

  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.Address     = 0;

  sCommand.DummyCycles = 0;

  sCommand.DataMode    = HAL_OSPI_DATA_1_LINE;
  sCommand.NbData      = 3;

  if (HAL_OSPI_GetState(ospiHandle) != HAL_OSPI_STATE_READY) {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Command(ospiHandle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Receive(ospiHandle, id, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  if (id[0] != JEDEC_ID_MFG || id[1] != JEDEC_ID_TYPE || id[2] != JEDEC_ID_CAP) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

