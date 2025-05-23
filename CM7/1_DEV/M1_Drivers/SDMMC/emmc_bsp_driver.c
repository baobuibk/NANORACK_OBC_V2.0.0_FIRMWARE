/*
 * emmc_stm32h745zi.c
 *
 *  Created on: Nov 22, 2024
 *      Author: CAO HIEU
 */

#include "emmc_bsp_driver.h"
//extern MMC_HandleTypeDef hmmc2;
extern MMC_HandleTypeDef hmmc1;

/**
  * @}
  */

/** @defgroup STM32H745I_DISCO_MMC_Private_FunctionsPrototypes Private Functions Prototypes
  * @{
  */
static void MMC_MspInit(MMC_HandleTypeDef *hmmc);
static void MMC_MspDeInit(MMC_HandleTypeDef *hmmc);
#if (USE_HAL_MMC_REGISTER_CALLBACKS == 1)
static void MMC_AbortCallback(MMC_HandleTypeDef *hmmc);
static void MMC_TxCpltCallback(MMC_HandleTypeDef *hmmc);
static void MMC_RxCpltCallback(MMC_HandleTypeDef *hmmc);
#endif
/**
  * @}
  */

/** @defgroup STM32H745I_DISCO_MMC_Exported_Functions Exported Functions
  * @{
  */

/**
  * @brief  Initializes the MMC card device.
  * @param  Instance      SDMMC Instance
  * @retval BSP status
  */
int32_t BSP_MMC_Init(MMC_HandleTypeDef *hmmc)
{
  int32_t ret = BSP_ERROR_NONE;
#if (USE_HAL_MMC_REGISTER_CALLBACKS == 0)
    /* Msp MMC initialization */

    MMC_MspInit(hmmc);

#else
    /* Register the MMC MSP Callbacks */
    if(IsMspCallbacksValid[Instance] == 0UL)
    {
      if(BSP_MMC_RegisterDefaultMspCallbacks(Instance) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
    }
    if(ret == BSP_ERROR_NONE)
    {
#endif
      /* HAL MMC initialization */

      if(MX_MMC_SD_Init(hmmc) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }

#if (USE_HAL_MMC_REGISTER_CALLBACKS == 1)
      /* Register MMC TC, HT and Abort callbacks */
      else if(HAL_MMC_RegisterCallback(&hsd_sdmmc[Instance], HAL_MMC_TX_CPLT_CB_ID, MMC_TxCpltCallback) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else if(HAL_MMC_RegisterCallback(&hsd_sdmmc[Instance], HAL_MMC_RX_CPLT_CB_ID, MMC_RxCpltCallback) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else
      {
        if(HAL_MMC_RegisterCallback(&hsd_sdmmc[Instance], HAL_MMC_ABORT_CB_ID, MMC_AbortCallback) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
      }
    }
#endif /* USE_HAL_MMC_REGISTER_CALLBACKS */


  return  ret;
}

/**
  * @brief  DeInitializes the MMC card device.
  * @param  Instance      SDMMC Instance
  * @retval BSP status
  */
int32_t BSP_MMC_DeInit(MMC_HandleTypeDef *hmmc)
{
  int32_t ret = BSP_ERROR_NONE;

  if(HAL_MMC_DeInit(hmmc) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* Msp MMC de-initialization */
#if (USE_HAL_MMC_REGISTER_CALLBACKS == 0)
    MMC_MspDeInit(hmmc);
#endif /* (USE_HAL_MMC_REGISTER_CALLBACKS == 0) */
  }

  return  ret;
}

/**
  * @brief  Initializes the SDMMC1 peripheral.
  * @param  hmmc SD handle
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_MMC_SD_Init(MMC_HandleTypeDef *hmmc)
{
  HAL_StatusTypeDef ret = HAL_OK;

//  hmmc->Instance                 = hmmc->Instance;
//  hmmc->Init.ClockDiv            = hmmc->Init.ClockDiv;
//  hmmc->Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
//  hmmc->Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
//  hmmc->Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
//  hmmc->Init.BusWide             = SDMMC_BUS_WIDE_4B;

  /* HAL SD initialization */
  if(HAL_MMC_Init(hmmc) != HAL_OK)
  {
    ret = HAL_ERROR;
  }

  return ret;
}

#if (USE_HAL_MMC_REGISTER_CALLBACKS == 1)
/**
  * @brief Default BSP MMC Msp Callbacks
  * @param  Instance      SDMMC Instance
  * @retval BSP status
  */
int32_t BSP_MMC_RegisterDefaultMspCallbacks(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= MMC_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if(HAL_MMC_RegisterCallback(&hsd_sdmmc[Instance], HAL_MMC_MSP_INIT_CB_ID, MMC_MspInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_MMC_RegisterCallback(&hsd_sdmmc[Instance], HAL_MMC_MSP_DEINIT_CB_ID, MMC_MspDeInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsMspCallbacksValid[Instance] = 1U;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief BSP MMC Msp Callback registering
  * @param  Instance   SDMMC Instance
  * @param  CallBacks  pointer to MspInit/MspDeInit callbacks functions
  * @retval BSP status
  */
int32_t BSP_MMC_RegisterMspCallbacks(uint32_t Instance, BSP_MMC_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= MMC_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if(HAL_MMC_RegisterCallback(&hsd_sdmmc[Instance], HAL_MMC_MSP_INIT_CB_ID, CallBacks->pMspInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_MMC_RegisterCallback(&hsd_sdmmc[Instance], HAL_MMC_MSP_DEINIT_CB_ID, CallBacks->pMspDeInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsMspCallbacksValid[Instance] = 1U;
    }
  }

  /* Return BSP status */
  return ret;
}
#endif /* (USE_HAL_MMC_REGISTER_CALLBACKS == 1) */

/**
  * @brief  Reads block(s) from a specified address in an SD card, in polling mode.
  * @param  Instance   MMC Instance
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be read
  * @param  BlocksNbr  Number of MMC blocks to read
  * @retval BSP status
  */
int32_t BSP_MMC_ReadBlocks(MMC_HandleTypeDef *hmmc, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
  uint32_t timeout = MMC_READ_TIMEOUT*BlocksNbr;
  int32_t ret;

  if(HAL_MMC_ReadBlocks(hmmc, (uint8_t *)pData, BlockIdx, BlocksNbr, timeout) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Writes block(s) to a specified address in an SD card, in polling mode.
  * @param  Instance   MMC Instance
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be written
  * @param  BlocksNbr  Number of MMC blocks to write
  * @retval BSP status
  */
int32_t BSP_MMC_WriteBlocks(MMC_HandleTypeDef *hmmc, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
  uint32_t timeout = MMC_READ_TIMEOUT*BlocksNbr;
  int32_t ret;

  if(HAL_MMC_WriteBlocks(hmmc, (uint8_t *)pData, BlockIdx, BlocksNbr, timeout) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Reads block(s) from a specified address in an SD card, in DMA mode.
  * @param  Instance   MMC Instance
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be read
  * @param  BlocksNbr  Number of MMC blocks to read
  * @retval BSP status
  */
int32_t BSP_MMC_ReadBlocks_DMA(MMC_HandleTypeDef *hmmc, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
  int32_t ret;

  if(HAL_MMC_ReadBlocks_DMA(hmmc, (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Writes block(s) to a specified address in an SD card, in DMA mode.
  * @param  Instance   MMC Instance
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be written
  * @param  BlocksNbr  Number of MMC blocks to write
  * @retval BSP status
  */
int32_t BSP_MMC_WriteBlocks_DMA(MMC_HandleTypeDef *hmmc, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
  int32_t ret;

  if(HAL_MMC_WriteBlocks_DMA(hmmc, (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Erases the specified memory area of the given MMC card.
  * @param  Instance   MMC Instance
  * @param  StartAddr : Start byte address
  * @param  EndAddr : End byte address
  * @retval BSP status
  */
int32_t BSP_MMC_Erase(MMC_HandleTypeDef *hmmc, uint32_t StartAddr, uint32_t EndAddr)
{
  int32_t ret;

  if( HAL_MMC_Erase(hmmc, StartAddr, EndAddr) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  return ret ;
}

/**
  * @brief  Handles MMC card interrupt request.
  * @param  Instance   MMC Instance
  * @retval None
  */
void BSP_MMC_IRQHandler(MMC_HandleTypeDef *hmmc)
{
  HAL_MMC_IRQHandler(hmmc);
}

/**
  * @brief  Gets the current MMC card data status.
  * @param  Instance   MMC Instance
  * @retval Data transfer state.
  *          This value can be one of the following values:
  *            @arg  MMC_TRANSFER_OK: No data transfer is acting
  *            @arg  MMC_TRANSFER_BUSY: Data transfer is acting
  *            @arg  MMC_TRANSFER_ERROR: Data transfer error
  */
int32_t BSP_MMC_GetCardState(MMC_HandleTypeDef *hmmc)
{
  return((HAL_MMC_GetCardState(hmmc) == HAL_MMC_CARD_TRANSFER ) ? MMC_TRANSFER_OK : MMC_TRANSFER_BUSY);
}

/**
  * @brief  Get MMC information about specific MMC card.
  * @param  Instance   MMC Instance
  * @param  CardInfo : Pointer to HAL_MMC_CardInfoTypedef structure
  * @retval None
  */
int32_t BSP_MMC_GetCardInfo(MMC_HandleTypeDef *hmmc, BSP_MMC_CardInfo *CardInfo)
{
  int32_t ret;

  if(HAL_MMC_GetCardInfo(hmmc, CardInfo) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief BSP MMC Abort callbacks
  * @param  Instance   MMC Instance
  * @retval None
  */
__weak void BSP_MMC_AbortCallback(MMC_HandleTypeDef *hmmc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hmmc);
}

/**
  * @brief BSP Tx Transfer completed callbacks
  * @param  Instance   MMC Instance
  * @retval None
  */
//__weak void BSP_MMC_WriteCpltCallback(MMC_HandleTypeDef *hmmc)
//{
//  /* Prevent unused argument(s) compilation warning */
//  UNUSED(hmmc);
//}

/**
  * @brief BSP Rx Transfer completed callbacks
  * @param  Instance   MMC Instance
  * @retval None
  */
__weak void BSP_MMC_ReadCpltCallback(MMC_HandleTypeDef *hmmc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hmmc);
}

#if (USE_HAL_MMC_REGISTER_CALLBACKS == 0)
/**
  * @brief MMC Abort callbacks
  * @param hmmc : MMC handle
  * @retval None
  */
void HAL_MMC_AbortCallback(MMC_HandleTypeDef *hmmc)
{
  BSP_MMC_AbortCallback(hmmc);
}

/**
  * @brief Tx Transfer completed callbacks
  * @param hmmc: MMC handle
  * @retval None
  */
void HAL_MMC_TxCpltCallback(MMC_HandleTypeDef *hmmc)
{
  BSP_MMC_WriteCpltCallback(hmmc);
}

/**
  * @brief Rx Transfer completed callbacks
  * @param hmmc: MMC handle
  * @retval None
  */
void HAL_MMC_RxCpltCallback(MMC_HandleTypeDef *hmmc)
{
  BSP_MMC_ReadCpltCallback(hmmc);
}
#endif
/**
  * @}
  */

/** @defgroup STM32H745I_DISCO_MMC_Private_Functions Private Functions
  * @{
  */

/**
  * @brief  Initializes the MMC MSP.
  * @param  hmmc  MMC handle
  * @retval None
  */
static void MMC_MspInit(MMC_HandleTypeDef *hmmc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hmmc);

  /* __weak function can be modified by the application */
  HAL_MMC_MspInit(hmmc);
}

/**
  * @brief  DeInitializes the MMC MSP.
  * @param  hmmc : MMC handle
  * @retval None
  */
static void MMC_MspDeInit(MMC_HandleTypeDef *hmmc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hmmc);

  /* Disable NVIC for SDIO interrupts */
  HAL_NVIC_DisableIRQ(SDMMC1_IRQn);

  /* DeInit GPIO pins can be done in the application
  (by surcharging this __weak function) */

  /* Disable SDMMC1 clock */
  __HAL_RCC_SDMMC1_CLK_DISABLE();

  /* GPIO pins clock and DMA clocks can be shut down in the application
  by surcharging this __weak function */
}

#if (USE_HAL_MMC_REGISTER_CALLBACKS == 1)
/**
  * @brief MMC Abort callbacks
  * @param hmmc : MMC handle
  * @retval None
  */
static void MMC_AbortCallback(MMC_HandleTypeDef *hmmc)
{
  BSP_MMC_AbortCallback(0);
}

/**
  * @brief Tx Transfer completed callbacks
  * @param hmmc : MMC handle
  * @retval None
  */
static void MMC_TxCpltCallback(MMC_HandleTypeDef *hmmc)
{
  BSP_MMC_WriteCpltCallback(0);
}

/**
  * @brief Rx Transfer completed callbacks
  * @param hmmc : MMC handle
  * @retval None
  */
static void MMC_RxCpltCallback(MMC_HandleTypeDef *hmmc)
{
  BSP_MMC_ReadCpltCallback(0);
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */


