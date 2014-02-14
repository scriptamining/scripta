/**
  ******************************************************************************
  * @file    hw_config.h
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Hardware Configuration & Setup
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "usb_type.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/
#define MASS_MEMORY_START     0x04002000
#define BULK_MAX_PACKET_SIZE  0x00000040
#define LED_ON                0xF0
#define LED_OFF               0xFF

#define USART_RX_DATA_SIZE   2048
//#define USART_RX_DATA_SIZE   64

#define COM1_IRQ_FLAG (1<<0)
#define COM2_IRQ_FLAG (1<<1)
#define COM3_IRQ_FLAG (1<<2)
#define COM4_IRQ_FLAG (1<<3)
#define COM5_IRQ_FLAG (1<<4)
#define USBIN_IRQ_FLAG (1<<5)
#define USBOUT_IRQ_FLAG (1<<6)
#define BTC_IRQ_FLAG (1<<7)

#define SET_IRQ_FLAG(irq) \
	IRQ_FLAG &= ~irq; \
	IRQ_FLAG |= irq
#define CLEAR_IRQ_FLAG(irq) \
	IRQ_FLAG &= ~irq
#define IS_IRQ_FLAG(irq) \
	(IRQ_FLAG & irq)
extern volatile u32 IRQ_FLAG;

/* Exported functions ------------------------------------------------------- */
void Set_System(void);
void Set_USBClock(void);
void Enter_LowPowerMode(void);
void Leave_LowPowerMode(void);
void USB_Interrupts_Config(void);
void USB_Cable_Config (FunctionalState NewState);
void USART_Config_Default(void);
bool USART_Config(void);
void btc_ltc_uart_config(u32 bd);
void USB_To_USART_Send_Data(uint8_t* data_buffer, uint8_t Nb_bytes);
void USART_To_USB_Send_Data(USART_TypeDef* USARTx);
void Handle_USBAsynchXfer (void);
void Get_SerialNum(void);
void Reset_GC3355(void);
void gpio_init(void);
void led0(int on);
int led0_revert(void);
int led1_revert(void);
void led1(int on);
void reset_btc_hw(void);
void reset_ltc_hw(void);
void reset_gc3355_chip(void);

/* External variables --------------------------------------------------------*/

#endif  /*__HW_CONFIG_H*/
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
