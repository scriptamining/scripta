/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Virtual Com Port Demo main file
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
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "trans_buffers.h"
#include "timer.h"
#include "debug.h"
#include "miner.h"
#include "delay.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
//extern struct TRANS_BUFFER usb_trans_buffer;
//extern struct TRANS_BUFFER uart_trans_buffer[COMn];

extern uint8_t USART_Rx_Buffer[USART_RX_DATA_SIZE];
uint8_t crx_buffer[UART_TRANS_BUFFER_SIZE][COMn];
uint8_t urx_buffer[USB_TRANS_BUFFER_SIZE];
uint32_t timer_count;

void rx_cx_buffers_init(void)
{
  int i;
  init_trans_buffer(&usb_trans_buffer,urx_buffer, USB_TRANS_BUFFER_SIZE);
	init_trans_buffer(&uart_trans_buffer[COM1],USART_Rx_Buffer, USART_RX_DATA_SIZE);
  for (i=1; i <= COMn; i++){
      init_trans_buffer(&uart_trans_buffer[i],crx_buffer[i],UART_TRANS_BUFFER_SIZE);
  }
}

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : main.
* Description    : Main routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
int main(void)
{
    int i;
    Reset_GC3355();
    Set_System();
    Set_USBClock();
    USB_Interrupts_Config();
#ifdef USE_USB_AS_HOST_IF	
    USB_Init();
#endif	
	  USART_Config_Default();
    btc_ltc_uart_config(115200);
    delay_init(72);
    //gpio_init();
    Timerx_Init(100,7200-1);
    USART_Config_Default();
    rx_cx_buffers_init();
    while (1)
    {
        dispatch_host_cmd();
        report_nonce_to_host();
        if(timer_count > 100) {
		timer_count = 0;
		//PR_DEBUG("GPIOB8 = %d\n",GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8));
		led1_revert();
		//PR_DEBUG("%02X\n",get_next_n_out(&usb_trans_buffer,152));
		//PR_DEBUG("has cmd length = %d\n",trans_buffer_has_cmd(&usb_trans_buffer));
        }
        //delay_ms(1000);
        //PR_DEBUG("delay 1000ms \n");
    }
}
#ifdef USE_FULL_ASSERT
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
