/**
  ******************************************************************************
  * @file    stm32_it.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
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

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "stm32_it.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "trans_buffers.h"
#include "miner.h"
#include "debug.h"

volatile u32 IRQ_FLAG;
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/******************************************************************************/
/*            Cortex-M Processor Exceptions Handlers                         */
/******************************************************************************/

/*******************************************************************************
* Function Name  : NMI_Handler
* Description    : This function handles NMI exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NMI_Handler(void)
{
}

/*******************************************************************************
* Function Name  : HardFault_Handler
* Description    : This function handles Hard Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : MemManage_Handler
* Description    : This function handles Memory Manage exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : BusFault_Handler
* Description    : This function handles Bus Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : UsageFault_Handler
* Description    : This function handles Usage Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : SVC_Handler
* Description    : This function handles SVCall exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVC_Handler(void)
{
}

/*******************************************************************************
* Function Name  : DebugMon_Handler
* Description    : This function handles Debug Monitor exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DebugMon_Handler(void)
{
}

/*******************************************************************************
* Function Name  : PendSV_Handler
* Description    : This function handles PendSVC exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void PendSV_Handler(void)
{
}

/*******************************************************************************
* Function Name  : SysTick_Handler
* Description    : This function handles SysTick Handler.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SysTick_Handler(void)
{
}

/*******************************************************************************
* Function Name  : USB_IRQHandler
* Description    : This function handles USB Low Priority interrupts
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)|| defined (STM32F37X)
void USB_LP_IRQHandler(void)
#else
void USB_LP_CAN1_RX0_IRQHandler(void)
#endif
{
  USB_Istr();
}

void EXTI9_5_IRQHandler(void)
{
  if (EXTI_GetITStatus(KEY_BUTTON_EXTI_LINE) != RESET)
  {
    /* Clear the EXTI line pending bit */
	EXTI_ClearITPendingBit(KEY_BUTTON_EXTI_LINE);
	led0_revert();
	SET_IRQ_FLAG(BTC_IRQ_FLAG);
  }
}

/*******************************************************************************
* Function Name  : EVAL_COM1_IRQHandler
* Description    : This function handles EVAL_COM1 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u32 com1_frame_count = 0;
void EVAL_COM1_IRQHandler(void)
{
  SET_IRQ_FLAG(COM1_IRQ_FLAG);
  if (USART_GetITStatus(EVAL_COM1, USART_IT_RXNE) != RESET)
  {
    /* Send the received data to the PC Host*/
    //USART_To_USB_Send_Data(EVAL_COM1);
    com1_frame_count = 0;
//#if defined(USE_UART1_AS_HOST_IF)
//    USART_to_trans_buffer(EVAL_COM1,&usb_trans_buffer);
//#else
    USART_to_trans_buffer(EVAL_COM1,&uart_trans_buffer[COM1]);
//#endif
  }

  /* If overrun condition occurs, clear the ORE flag and recover communication */
  if (USART_GetFlagStatus(EVAL_COM1, USART_FLAG_ORE) != RESET)
  {
    (void)USART_ReceiveData(EVAL_COM1);
  }
}

void EVAL_COM2_IRQHandler(void)
{
  SET_IRQ_FLAG(COM2_IRQ_FLAG);
  if (USART_GetITStatus(EVAL_COM2, USART_IT_RXNE) != RESET)
  {
    /* Send the received data to the PC Host*/
    //USART_To_USB_Send_Data(EVAL_COM2);
    USART_to_trans_buffer(EVAL_COM2,&uart_trans_buffer[COM2]);
  }

  /* If overrun condition occurs, clear the ORE flag and recover communication */
  if (USART_GetFlagStatus(EVAL_COM2, USART_FLAG_ORE) != RESET)
  {
    (void)USART_ReceiveData(EVAL_COM2);
  }
}

void EVAL_COM3_IRQHandler(void)
{
  SET_IRQ_FLAG(COM3_IRQ_FLAG);
  if (USART_GetITStatus(EVAL_COM3, USART_IT_RXNE) != RESET)
  {
    /* Send the received data to the PC Host*/
    //USART_To_USB_Send_Data(EVAL_COM3);
    USART_to_trans_buffer(EVAL_COM3,&uart_trans_buffer[COM3]);
  }

  /* If overrun condition occurs, clear the ORE flag and recover communication */
  if (USART_GetFlagStatus(EVAL_COM3, USART_FLAG_ORE) != RESET)
  {
    (void)USART_ReceiveData(EVAL_COM3);
  }
}
#if defined(USE_STM3210E_EVAL)
void EVAL_COM4_IRQHandler(void)
{
  SET_IRQ_FLAG(COM4_IRQ_FLAG);
  if (USART_GetITStatus(EVAL_COM4, USART_IT_RXNE) != RESET)
  {
    /* Send the received data to the PC Host*/
    //USART_To_USB_Send_Data(EVAL_COM4);
    USART_to_trans_buffer(EVAL_COM4,&uart_trans_buffer[COM4]);
  }

  /* If overrun condition occurs, clear the ORE flag and recover communication */
  if (USART_GetFlagStatus(EVAL_COM4, USART_FLAG_ORE) != RESET)
  {
    (void)USART_ReceiveData(EVAL_COM4);
  }
}

void EVAL_COM5_IRQHandler(void)
{
  SET_IRQ_FLAG(COM5_IRQ_FLAG);
  if (USART_GetITStatus(EVAL_COM5, USART_IT_RXNE) != RESET)
  {
    /* Send the received data to the PC Host*/
    //USART_To_USB_Send_Data(EVAL_COM5);
    USART_to_trans_buffer(EVAL_COM5,&uart_trans_buffer[COM5]);
  }

  /* If overrun condition occurs, clear the ORE flag and recover communication */
  if (USART_GetFlagStatus(EVAL_COM5, USART_FLAG_ORE) != RESET)
  {
    (void)USART_ReceiveData(EVAL_COM5);
  }
}
#endif
/*******************************************************************************
* Function Name  : USB_FS_WKUP_IRQHandler
* Description    : This function handles USB WakeUp interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)
void USB_FS_WKUP_IRQHandler(void)
#else
void USBWakeUp_IRQHandler(void)
#endif
{
  EXTI_ClearITPendingBit(EXTI_Line18);
}

/******************************************************************************/
/*                 STM32 Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32xxx.s).                                            */
/******************************************************************************/

/*******************************************************************************
* Function Name  : PPP_IRQHandler
* Description    : This function handles PPP interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
/*void PPP_IRQHandler(void)
{
}*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

