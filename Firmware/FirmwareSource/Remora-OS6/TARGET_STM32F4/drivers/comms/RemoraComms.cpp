#include "mbed.h"
#include "RemoraComms.h"

#include "stm32f4xx_hal.h"

RemoraComms::RemoraComms(volatile rxData_t* ptrRxData, volatile txData_t* ptrTxData, SPI_TypeDef* spiType, PinName interruptPin) :
    ptrRxData(ptrRxData),
    ptrTxData(ptrTxData),
    spiType(spiType),
    interruptPin(interruptPin), 
    slaveSelect(interruptPin)
{
    this->spiHandle.Instance = this->spiType;

    if (this->interruptPin == PA_4)
    {
        // interrupt pin is the NSS pin
        sharedSPI = false;
        HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
    }
    else if (this->interruptPin == PB_12)
    {
        // interrupt pin is the NSS pin
        sharedSPI = false;
        HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    }
    else if (this->interruptPin == PE_10)
    {
        // interrupt pin is not the NSS pin, ie the board shares the SPI bus with the SD card
        // configure the SPI in software NSS mode and always on
        sharedSPI = true;
        HAL_NVIC_SetPriority(EXTI15_10_IRQn , 5, 0);
    }
    else if (this->interruptPin == PC_6)
    {
        // interrupt pin is not the NSS pin, ie the board shares the SPI bus with the SD card
        // configure the SPI in software NSS mode and always on
        sharedSPI = true;
        HAL_NVIC_SetPriority(EXTI9_5_IRQn , 5, 0);
    }


    //slaveSelect.rise(callback(this, &RemoraComms::processPacket));
    slaveSelect.rise(callback(this, &RemoraComms::NSSinterrupt));
}

void RemoraComms:: update()
{
	if (this->data)
	{
		this->noDataCount = 0;
		this->CommsStatus = true;
	}
	else
	{
		this->noDataCount++;
	}

	if (this->noDataCount > DATA_ERR_MAX)
	{
		this->noDataCount = 0;
		this->CommsStatus = false;
	}

	this->data = false;    
}


void RemoraComms::init()
{
    if(this->spiHandle.Instance == SPI1)
    {
        printf("Initialising SPI1 slave\n");

        GPIO_InitTypeDef GPIO_InitStruct;

        /**SPI1 GPIO Configuration
        PA4     ------> SPI1_NSS (YELLOW)
        PA5     ------> SPI1_SCK (GREEN)
        PA6     ------> SPI1_MISO (ORANGE)
        PA7     ------> SPI1_MOSI (RED)
        */
        GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        __HAL_RCC_SPI1_CLK_ENABLE();

        this->spiHandle.Init.Mode           = SPI_MODE_SLAVE;
        this->spiHandle.Init.Direction      = SPI_DIRECTION_2LINES;
        this->spiHandle.Init.DataSize       = SPI_DATASIZE_8BIT;
        this->spiHandle.Init.CLKPolarity    = SPI_POLARITY_LOW;
        this->spiHandle.Init.CLKPhase       = SPI_PHASE_1EDGE;
        if (sharedSPI)
        {
            this->spiHandle.Init.NSS            = SPI_NSS_SOFT;
            printf("SPI is shared with SD card\n");
        }
        else
        {
            this->spiHandle.Init.NSS            = SPI_NSS_HARD_INPUT;
        } 
        this->spiHandle.Init.FirstBit       = SPI_FIRSTBIT_MSB;
        this->spiHandle.Init.TIMode         = SPI_TIMODE_DISABLE;
        this->spiHandle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
        this->spiHandle.Init.CRCPolynomial  = 10;

        HAL_SPI_Init(&this->spiHandle);

        if (sharedSPI)
        {
            // set SSI (Slave Select Internal) low, ie same as NSS going low
             CLEAR_BIT(this->spiHandle.Instance->CR1, SPI_CR1_SSI);
        }
    }
    else if(this->spiHandle.Instance == SPI2)
    {
        printf("Initialising SPI2 slave\n");

        GPIO_InitTypeDef GPIO_InitStruct;

        /**SPI2 GPIO Configuration
        PB12     ------> SPI2_NSS (YELLOW)
        PB13     ------> SPI2_SCK (GREEN)
        PB14     ------> SPI2_MISO (ORANGE)
        PB15     ------> SPI2_MOSI (RED)
        */
        GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        __HAL_RCC_SPI2_CLK_ENABLE();

        this->spiHandle.Init.Mode           = SPI_MODE_SLAVE;
        this->spiHandle.Init.Direction      = SPI_DIRECTION_2LINES;
        this->spiHandle.Init.DataSize       = SPI_DATASIZE_8BIT;
        this->spiHandle.Init.CLKPolarity    = SPI_POLARITY_LOW;
        this->spiHandle.Init.CLKPhase       = SPI_PHASE_1EDGE;
        if (sharedSPI)
        {
            this->spiHandle.Init.NSS            = SPI_NSS_SOFT;
            printf("SPI is shared with SD card\n");
        }
        else
        {
            this->spiHandle.Init.NSS            = SPI_NSS_HARD_INPUT;
        } 
        this->spiHandle.Init.FirstBit       = SPI_FIRSTBIT_MSB;
        this->spiHandle.Init.TIMode         = SPI_TIMODE_DISABLE;
        this->spiHandle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
        this->spiHandle.Init.CRCPolynomial  = 10;

        HAL_SPI_Init(&this->spiHandle);

        if (sharedSPI)
        {
            // set SSI (Slave Select Internal) low, ie same as NSS going low
             CLEAR_BIT(this->spiHandle.Instance->CR1, SPI_CR1_SSI);
        }
    }

    if(this->spiHandle.Instance == SPI1)
    {
        printf("Initialising DMA for SPI\n");

        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_DMA2_CLK_ENABLE();

        this->hdma_spi_tx.Instance                   = DMA2_Stream3;
        this->hdma_spi_tx.Init.Channel               = DMA_CHANNEL_3;
        this->hdma_spi_tx.Init.Direction             = DMA_MEMORY_TO_PERIPH;
        this->hdma_spi_tx.Init.PeriphInc             = DMA_PINC_DISABLE;
        this->hdma_spi_tx.Init.MemInc                = DMA_MINC_ENABLE;
        this->hdma_spi_tx.Init.PeriphDataAlignment   = DMA_PDATAALIGN_BYTE;
        this->hdma_spi_tx.Init.MemDataAlignment      = DMA_MDATAALIGN_BYTE;
        this->hdma_spi_tx.Init.Mode                  = DMA_CIRCULAR;
        //this->hdma_spi_tx.Init.Mode                  = DMA_NORMAL;
        this->hdma_spi_tx.Init.Priority              = DMA_PRIORITY_VERY_HIGH;
        this->hdma_spi_tx.Init.FIFOMode              = DMA_FIFOMODE_DISABLE;
        
        HAL_DMA_Init(&this->hdma_spi_tx);

        __HAL_LINKDMA(&this->spiHandle, hdmatx, this->hdma_spi_tx);

        //HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
        ///NVIC_SetVector(DMA2_Stream3_IRQn, (uint32_t)&DMA2_Stream3_IRQHandler);
        //HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

        this->hdma_spi_rx.Instance                   = DMA2_Stream0;
        this->hdma_spi_rx.Init.Channel               = DMA_CHANNEL_3;
        this->hdma_spi_rx.Init.Direction             = DMA_PERIPH_TO_MEMORY;
        this->hdma_spi_rx.Init.PeriphInc             = DMA_PINC_DISABLE;
        this->hdma_spi_rx.Init.MemInc                = DMA_MINC_ENABLE;
        this->hdma_spi_rx.Init.PeriphDataAlignment   = DMA_PDATAALIGN_BYTE;
        this->hdma_spi_rx.Init.MemDataAlignment      = DMA_MDATAALIGN_BYTE;
        this->hdma_spi_rx.Init.Mode                  = DMA_CIRCULAR;
        //this->hdma_spi_rx.Init.Mode                  = DMA_NORMAL;
        this->hdma_spi_rx.Init.Priority              = DMA_PRIORITY_VERY_HIGH;
        this->hdma_spi_rx.Init.FIFOMode              = DMA_FIFOMODE_DISABLE;

        HAL_DMA_Init(&this->hdma_spi_rx);

        __HAL_LINKDMA(&this->spiHandle,hdmarx,this->hdma_spi_rx);

        //HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
        //NVIC_SetVector(DMA2_Stream0_IRQn, (uint32_t)&DMA2_Stream0_IRQHandler);
        //HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
        
        this->hdma_memtomem_dma2_stream1.Instance                 = DMA2_Stream1;
        this->hdma_memtomem_dma2_stream1.Init.Channel             = DMA_CHANNEL_0;
        this->hdma_memtomem_dma2_stream1.Init.Direction           = DMA_MEMORY_TO_MEMORY;
        this->hdma_memtomem_dma2_stream1.Init.PeriphInc           = DMA_PINC_ENABLE;
        this->hdma_memtomem_dma2_stream1.Init.MemInc              = DMA_MINC_ENABLE;
        this->hdma_memtomem_dma2_stream1.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        this->hdma_memtomem_dma2_stream1.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        this->hdma_memtomem_dma2_stream1.Init.Mode                = DMA_NORMAL;
        this->hdma_memtomem_dma2_stream1.Init.Priority            = DMA_PRIORITY_LOW;
        this->hdma_memtomem_dma2_stream1.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
        this->hdma_memtomem_dma2_stream1.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        this->hdma_memtomem_dma2_stream1.Init.MemBurst            = DMA_MBURST_SINGLE;
        this->hdma_memtomem_dma2_stream1.Init.PeriphBurst         = DMA_PBURST_SINGLE;

        HAL_DMA_Init(&this->hdma_memtomem_dma2_stream1);

    }
    else if(this->spiHandle.Instance == SPI2)
    {
        printf("Initialising DMA for SPI2\n");

        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();

        this->hdma_spi_tx.Instance                   = DMA1_Stream4;
        this->hdma_spi_tx.Init.Channel               = DMA_CHANNEL_0;
        this->hdma_spi_tx.Init.Direction             = DMA_MEMORY_TO_PERIPH;
        this->hdma_spi_tx.Init.PeriphInc             = DMA_PINC_DISABLE;
        this->hdma_spi_tx.Init.MemInc                = DMA_MINC_ENABLE;
        this->hdma_spi_tx.Init.PeriphDataAlignment   = DMA_PDATAALIGN_BYTE;
        this->hdma_spi_tx.Init.MemDataAlignment      = DMA_MDATAALIGN_BYTE;
        this->hdma_spi_tx.Init.Mode                  = DMA_CIRCULAR;
        //this->hdma_spi_tx.Init.Mode                  = DMA_NORMAL;
        this->hdma_spi_tx.Init.Priority              = DMA_PRIORITY_VERY_HIGH;
        this->hdma_spi_tx.Init.FIFOMode              = DMA_FIFOMODE_DISABLE;
        
        HAL_DMA_Init(&this->hdma_spi_tx);

        __HAL_LINKDMA(&this->spiHandle, hdmatx, this->hdma_spi_tx);

        //HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
        ///NVIC_SetVector(DMA2_Stream3_IRQn, (uint32_t)&DMA2_Stream3_IRQHandler);
        //HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

        this->hdma_spi_rx.Instance                   = DMA1_Stream3;
        this->hdma_spi_rx.Init.Channel               = DMA_CHANNEL_0;
        this->hdma_spi_rx.Init.Direction             = DMA_PERIPH_TO_MEMORY;
        this->hdma_spi_rx.Init.PeriphInc             = DMA_PINC_DISABLE;
        this->hdma_spi_rx.Init.MemInc                = DMA_MINC_ENABLE;
        this->hdma_spi_rx.Init.PeriphDataAlignment   = DMA_PDATAALIGN_BYTE;
        this->hdma_spi_rx.Init.MemDataAlignment      = DMA_MDATAALIGN_BYTE;
        this->hdma_spi_rx.Init.Mode                  = DMA_CIRCULAR;
        //this->hdma_spi_rx.Init.Mode                  = DMA_NORMAL;
        this->hdma_spi_rx.Init.Priority              = DMA_PRIORITY_VERY_HIGH;
        this->hdma_spi_rx.Init.FIFOMode              = DMA_FIFOMODE_DISABLE;

        HAL_DMA_Init(&this->hdma_spi_rx);

        __HAL_LINKDMA(&this->spiHandle,hdmarx,this->hdma_spi_rx);

        //HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
        //NVIC_SetVector(DMA2_Stream0_IRQn, (uint32_t)&DMA2_Stream0_IRQHandler);
        //HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
        
        this->hdma_memtomem_dma2_stream1.Instance                 = DMA2_Stream1;
        this->hdma_memtomem_dma2_stream1.Init.Channel             = DMA_CHANNEL_0;
        this->hdma_memtomem_dma2_stream1.Init.Direction           = DMA_MEMORY_TO_MEMORY;
        this->hdma_memtomem_dma2_stream1.Init.PeriphInc           = DMA_PINC_ENABLE;
        this->hdma_memtomem_dma2_stream1.Init.MemInc              = DMA_MINC_ENABLE;
        this->hdma_memtomem_dma2_stream1.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        this->hdma_memtomem_dma2_stream1.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        this->hdma_memtomem_dma2_stream1.Init.Mode                = DMA_NORMAL;
        this->hdma_memtomem_dma2_stream1.Init.Priority            = DMA_PRIORITY_LOW;
        this->hdma_memtomem_dma2_stream1.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
        this->hdma_memtomem_dma2_stream1.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        this->hdma_memtomem_dma2_stream1.Init.MemBurst            = DMA_MBURST_SINGLE;
        this->hdma_memtomem_dma2_stream1.Init.PeriphBurst         = DMA_PBURST_SINGLE;

        HAL_DMA_Init(&this->hdma_memtomem_dma2_stream1);
    }
}

void RemoraComms::start()
{
    this->ptrTxData->header = PRU_DATA;
    HAL_SPI_TransmitReceive_DMA(&this->spiHandle, (uint8_t *)this->ptrTxData->txBuffer, (uint8_t *)this->spiRxBuffer.rxBuffer, SPI_BUFF_SIZE);
}

void RemoraComms::NSSinterrupt()
{
    // NSS / CS has gone high, packet recieved
    this->NSS = true;
}

void RemoraComms::SPItasks()
{
    if (this->NSS)
    {
        this->NSS = false;

        this->DMArxCnt = 0;
        this->ticksStart = HAL_GetTick();

        // wait for DMA to complete and break if DMA is not complete in time
        while (this->DMArxCnt != SPI_BUFF_SIZE)
        {
            this->DMArxCnt = __HAL_DMA_GET_COUNTER(&this->hdma_spi_rx);
            this->ticks = HAL_GetTick() - this->ticksStart;

            if (this->ticks > 2)
            {
                this->resetSPI = true;
                break;
            }
        }

        if (this->resetSPI)
        {
            // for testing, not needed / implemented
            printf("  Reset SPI now\n");
            this->resetSPI = false;
        }

        switch (this->spiRxBuffer.header)
        {
            case PRU_READ:
                this->data = true;
                this->rejectCnt = 0;
                ++this->dataCnt;
                // READ so do nothing with the received data
                break;

            case PRU_WRITE:
                this->data = true;
                this->rejectCnt = 0;
                ++this->dataCnt;
                // we've got a good WRITE header, move the data to rxData

                // ensure an atomic access to the rxBuffer
                // disable thread interrupts
                __disable_irq(); 

                for (int i = 0; i < SPI_BUFF_SIZE; i++)
                {
                    this->ptrRxData->rxBuffer[i] = this->spiRxBuffer.rxBuffer[i];
                }

                // re-enable thread interrupts
                __enable_irq();
                
                break;

            default:
                this->rejectCnt++;
                if (this->rejectCnt > 5)
                {
                    this->SPIdataError = true;
                }
        }
    }
}


bool RemoraComms::getStatus(void)
{
    return this->CommsStatus;
}

void RemoraComms::setStatus(bool status)
{
    this->CommsStatus = status;
}

bool RemoraComms::getError(void)
{
    return this->SPIdataError;
}

void RemoraComms::setError(bool error)
{
    this->SPIdataError = error;
}