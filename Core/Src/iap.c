/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "MLX90640_API.h"
#include "usbd_cdc_if.h"

static uint32_t GetSector(uint32_t Address);
extern unsigned int xcrc32 (const unsigned char *buf, int len, unsigned int init);



/* Base address of the Flash sectors Bank 1 */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */


#define IAP_RXBUFFER_SIZE		(200)
#define IAP_TXBUFFER_SIZE		(16)

#define FLASH_IMAGE_ADDRESS			(ADDR_FLASH_SECTOR_6)

static uint8_t rx_buffer[IAP_RXBUFFER_SIZE];
static uint32_t rx_leng;
static uint8_t tx_buffer[IAP_TXBUFFER_SIZE];
static uint8_t rx_flag = 0;
static uint32_t length;
static uint32_t crc32;
static uint32_t write_address;
static uint8_t *p_temp = rx_buffer;

/*Variable used for Erase procedure*/
static FLASH_EraseInitTypeDef EraseInitStruct;

void iap_init(void)
{
	uint32_t Sector;
	uint32_t SECTORError = 0;
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();
	
  /* Erase the user Flash area
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

  /* Get the 1st sector to erase */
  Sector = GetSector(FLASH_IMAGE_ADDRESS);
  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector        = Sector;
  EraseInitStruct.NbSectors     = 1;


  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
  {
    while (1);
  }
}


void iap_handler(void)
{
	if (rx_flag)
	{
		rx_flag = 0;
		
		switch (rx_leng)
		{
			case 10:		/* fire ware info */
			{
				length = ((uint32_t)rx_buffer[2] << 24) | ((uint32_t)rx_buffer[3] << 16) | ((uint32_t)rx_buffer[4] << 8) | ((uint32_t)rx_buffer[5] << 0);
				crc32 = ((uint32_t)rx_buffer[6] << 24) | ((uint32_t)rx_buffer[7] << 16) | ((uint32_t)rx_buffer[8] << 8) | ((uint32_t)rx_buffer[9] << 0);
				write_address = FLASH_IMAGE_ADDRESS;
				p_temp = rx_buffer;
				tx_buffer[0] = tx_buffer[1] = tx_buffer[2] = tx_buffer[3] = 0xAA;
				CDC_Transmit_FS(tx_buffer, 4);
				break;
			}
			case 128:
			{
				while (rx_leng)
				{
					if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, write_address, *p_temp) == HAL_OK)
					{
						write_address = write_address + 4;
						p_temp = p_temp + 4;
						rx_leng = rx_leng - 4;
						tx_buffer[0] = tx_buffer[1] = tx_buffer[2] = tx_buffer[3] = 0xAA;
						CDC_Transmit_FS(tx_buffer, 4);
					}
					else
					{
						while (1);
					}
				}
			}
			case 4:
			{
				if (xcrc32((const unsigned char *)FLASH_IMAGE_ADDRESS, length, 0xffffffff) == crc32)
				{
					HAL_FLASH_Lock();
					
					
				}
				else
				{
					while (1);
				}
			}
		}
	}
}





void iap_receive_callback(uint8_t *Buf, uint32_t Len)
{
	uint8_t *p_buf = rx_buffer;
	uint8_t *p_buf_ = Buf;
	rx_leng = Len;
	
	while (Len--)
	{
		*p_buf++ = *p_buf_++;
	}
	rx_flag = 1;
}




/**
  * @brief  Gets the sector of a given address
  * @param  None
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;

  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;
  }
  return sector;
}

