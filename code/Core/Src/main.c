/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/**
  ******************************************************************************
  * ����:�üһ�VCC         
  * ��ϵ��ʽ:1930299709@qq.com
  * �����汾:V3.3.0
  *	ʹ��Ӳ���汾:V3.3.0
  *
  ******************************************************************************
  * ������־:
  *
  * 
  *
  ******************************************************************************
  */  
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "stdio.h"
#include "motor.h"
#include "niming.h"
#include "pid.h"

#include "cJSON.h"
#include <string.h>
#include "HC_SR04.h"



#include "mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
extern float Motor1Speed ;//�������1�ٶ�
extern float Motor2Speed ;//�������2�ٶ�

extern tPid pidMotor1Speed;//�������1PID�ٶȿ��ƽṹ�����ͱ���
extern tPid pidMotor2Speed;
extern tPid pidFollow;    //���������PID
extern tPid pidMPU6050YawMovement;  //����6050ƫ���� ������̬���Ƶ�PID
extern uint8_t Usart1_ReadBuf[255];	//����1 ��������
float p,i,d,a,b;//ʹ��JSONʱ��ʹ�õı���
uint8_t OledString[50];//OLED��ʾʹ�õ��ַ�������
extern float Mileage;//�����

extern tPid pidHW_Tracking;//����ѭ����PID
uint8_t g_ucaHW_Read[4] = {0};//�������Թܵ�ƽ������
int8_t g_cThisState = 0;//���״̬
int8_t g_cLastState = 0; //�ϴ�״̬
float g_fHW_PID_Out;//����Թ�PID��������ٶ�
float g_fHW_PID_Out1;//���1�����ѭ��PID�����ٶ�
float g_fHW_PID_Out2;//���2�����ѭ��PID�����ٶ�

uint8_t g_ucUsart3ReceiveData;  //���洮�������յ�����

uint8_t Usart3String[50];//����������ַ���ʹ�õ��ַ�������
float g_fHC_SR04_Read;//��������������ȡ�ϰ�������
float g_fFollow_PID_Out;//���������PID��������ٶ�


float pitch,roll,yaw; //������ ����� �����

float  g_fMPU6050YawMovePidOut = 0.00f; //��̬PID�������
float  g_fMPU6050YawMovePidOut1 = 0.00f; //��һ������������
float  g_fMPU6050YawMovePidOut2 = 0.00f; //��һ������������

uint8_t g_ucMode = 0; 
//С���˶�ģʽ��־λ 0:��ʾ���ܡ�1:PIDѭ��ģʽ��2:�ֻ�ң����ͨ�˶�ģʽ��3.����������ģʽ��4:PID����ģʽ��5:ң�ؽǶȱջ�
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_ADC2_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();			//��ʼ��OLED  
  OLED_Clear(); 
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);//������ʱ��1 ͨ��1 PWM���
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_4);//������ʱ��1 ͨ��4 PWM���
  HAL_TIM_Encoder_Start(&htim2,TIM_CHANNEL_ALL);//������ʱ��2
  HAL_TIM_Encoder_Start(&htim4,TIM_CHANNEL_ALL);//������ʱ��4
  HAL_TIM_Base_Start_IT(&htim2);				//������ʱ��2 �ж�
  HAL_TIM_Base_Start_IT(&htim4);                //������ʱ��4 �ж�
  
  HAL_TIM_Base_Start_IT(&htim1);                //������ʱ��1 �ж�
  __HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE);	//��������1�����ж�
  PID_init();//PID������ʼ��
  HAL_UART_Receive_IT(&huart3,&g_ucUsart3ReceiveData,1);  //��������������
  
  HAL_Delay(500);//��ʱ0.5�� 6050�ϵ��ȶ����ʼ��
  MPU_Init(); //��ʼ��MPU6050
  while(MPU_Init()!=0);//��ʼ��MPU6050ģ���MPU ע���ʼ���׶β�Ҫ�ƶ�С��
  while(mpu_dmp_init()!=0);//mpu6050,dmp��ʼ��

//  cJSON *cJsonData ,*cJsonVlaue;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		sprintf((char *)OledString," g_ucMode:%d",g_ucMode);//��ʾg_ucMode ��ǰģʽ
		OLED_ShowString(0,6,OledString,12);	//��ʾ��OLED��
		
		sprintf((char *)Usart3String," g_ucMode:%d",g_ucMode);//����APP��ʾ
		HAL_UART_Transmit(&huart3,( uint8_t *)Usart3String,strlen(( const  char  *)Usart3String),50);//����ʽ����ͨ������������ַ� strlen:�����ַ�����С
		
		if(g_ucMode == 0)
		{
		//0LED��ʾ����
			sprintf((char*)OledString, "V1:%.2fV2:%.2f", Motor1Speed,Motor2Speed);//��ʾ�ٶ�
			OLED_ShowString(0,0,OledString,12);//�����oled��������ģ�����ʾλ�õ�һ��������
			
			sprintf((char*)OledString, "Mileage:%.2f", Mileage);//��ʾ���
			OLED_ShowString(0,1,OledString,12);//�����oled��������ģ�����ʾλ�õ�һ��������
			
			sprintf((char*)OledString, "U:%.2fV", adcGetBatteryVoltage());//��ʾ��ص�ѹ
			OLED_ShowString(0,2,OledString,12);//�����oled��������ģ�����ʾλ�õ�һ��������
			
			sprintf((char *)OledString,"HC_SR04:%.2fcm\r\n",HC_SR04_Read());//��ʾ����������
			OLED_ShowString(0,3,OledString,12);//�����oled��������ģ�����ʾλ�õ�һ��������
			
			sprintf((char *)OledString,"p:%.2f r:%.2f \r\n",pitch,roll);//��ʾ6050���� ������ �����
			OLED_ShowString(0,4,OledString,12);//�����oled��������ģ�����ʾλ�õ�һ��������
			
			sprintf((char *)OledString,"y:%.2f  \r\n",yaw);//��ʾ6050����  �����
			OLED_ShowString(0,5,OledString,12);//�����oled��������ģ�����ʾλ�õ�һ��������
			
		//����APP��ʾ
			sprintf((char*)Usart3String, "V1:%.2fV2:%.2f", Motor1Speed,Motor2Speed);//��ʾ�ٶ�
			HAL_UART_Transmit(&huart3,( uint8_t *)Usart3String,strlen(( const  char  *)Usart3String),50);//����ʽ����ͨ������������ַ� strlen:�����ַ�����С
			//������ʽ���Ϳ��Ա�֤���ݷ�����ϣ��жϷ��Ͳ�һ�����Ա�֤�����Ѿ�������ϲ�������һ�η���
			sprintf((char*)Usart3String, "Mileage:%.2f", Mileage);//��ʾ���
			HAL_UART_Transmit(&huart3,( uint8_t *)Usart3String,strlen(( const  char  *)Usart3String),50);//����ʽ����ͨ������������ַ� strlen:�����ַ�����С
			
			sprintf((char*)Usart3String, "U:%.2fV", adcGetBatteryVoltage());//��ʾ��ص�ѹ
			HAL_UART_Transmit(&huart3,( uint8_t *)Usart3String,strlen(( const  char  *)Usart3String),50);//����ʽ����ͨ������������ַ� strlen:�����ַ�����С
			
			sprintf((char *)Usart3String,"HC_SR04:%.2fcm\r\n",HC_SR04_Read());//��ʾ����������
			HAL_UART_Transmit(&huart3,( uint8_t *)Usart3String,strlen(( const  char  *)Usart3String),50);//����ʽ����ͨ������������ַ� strlen:�����ַ�����С
			
			sprintf((char *)Usart3String,"p:%.2f r:%.2f \r\n",pitch,roll);//��ʾ6050���� ������ �����
			HAL_UART_Transmit(&huart3,( uint8_t *)Usart3String,strlen(( const  char  *)Usart3String),50);//����ʽ����ͨ������������ַ� strlen:�����ַ�����С
			
			sprintf((char *)Usart3String,"y:%.2f  \r\n",yaw);//��ʾ6050����  �����
			HAL_UART_Transmit(&huart3,( uint8_t *)Usart3String,strlen(( const  char  *)Usart3String),50);//����ʽ����ͨ������������ַ� strlen:�����ַ�����С
		
			//���6050����
			while(mpu_dmp_get_data(&pitch,&roll,&yaw)!=0){}  //������Խ���������������ݵ�����
			
			//����ʾģʽ���ͣת ����С���ٶ�Ϊ0
			motorPidSetSpeed(0,0);
		}
		if(g_ucMode == 1)
		{
			///****    ����PIDѭ������******************/
			g_ucaHW_Read[0] = READ_HW_OUT_1;//��ȡ����Թ�״̬�����������д��if�������Ч
			g_ucaHW_Read[1] = READ_HW_OUT_2;
			g_ucaHW_Read[2] = READ_HW_OUT_3;
			g_ucaHW_Read[3] = READ_HW_OUT_4;

			if(g_ucaHW_Read[0] == 0&&g_ucaHW_Read[1] == 0&&g_ucaHW_Read[2] == 0&&g_ucaHW_Read[3] == 0 )
			{
		//		printf("Ӧ��ǰ��\r\n");//ע�͵����Ӹ�Ч�������ޱ�Ҫ����ִ��
				g_cThisState = 0;//ǰ��
			}
			else if(g_ucaHW_Read[0] == 0&&g_ucaHW_Read[1] == 1&&g_ucaHW_Read[2] == 0&&g_ucaHW_Read[3] == 0 )//ʹ��else if���Ӻ�����Ч
			{
		//		printf("Ӧ����ת\r\n");
				g_cThisState = -1;//Ӧ����ת
			}
			else if(g_ucaHW_Read[0] == 1&&g_ucaHW_Read[1] == 0&&g_ucaHW_Read[2] == 0&&g_ucaHW_Read[3] == 0 )
			{
		//		printf("������ת\r\n");
				g_cThisState = -2;//������ת
			}
			else if(g_ucaHW_Read[0] == 1&&g_ucaHW_Read[1] == 1&&g_ucaHW_Read[2] == 0&&g_ucaHW_Read[3] == 0)
			{
		//		printf("������ת\r\n");
				g_cThisState = -3;//������ת
			}
			else if(g_ucaHW_Read[0] == 0&&g_ucaHW_Read[1] == 0&&g_ucaHW_Read[2] == 1&&g_ucaHW_Read[3] == 0 )
			{
		//		printf("Ӧ����ת\r\n");
				g_cThisState = 1;//Ӧ����ת	
			}
			else if(g_ucaHW_Read[0] == 0&&g_ucaHW_Read[1] == 0&&g_ucaHW_Read[2] == 0&&g_ucaHW_Read[3] == 1 )
			{
		//		printf("������ת\r\n");
				g_cThisState = 2;//������ת
			}
			else if(g_ucaHW_Read[0] == 0&&g_ucaHW_Read[1] == 0&&g_ucaHW_Read[2] == 1&&g_ucaHW_Read[3] == 1)
			{
		//	    printf("������ת\r\n");
				g_cThisState = 3;//������ת
			}
			g_fHW_PID_Out = PID_realize(&pidHW_Tracking,g_cThisState);//PID�������Ŀ���ٶ� ����ٶȣ���ͻ����ٶȼӼ�

			g_fHW_PID_Out1 = 3 + g_fHW_PID_Out;//���1�ٶ�=�����ٶ�+ѭ��PID����ٶ�
			g_fHW_PID_Out2 = 3 - g_fHW_PID_Out;//���1�ٶ�=�����ٶ�-ѭ��PID����ٶ�
			if(g_fHW_PID_Out1 >5) g_fHW_PID_Out1 =5;//�����޷� �޷��ٶ���0-5֮��
			if(g_fHW_PID_Out1 <0) g_fHW_PID_Out1 =0;
			if(g_fHW_PID_Out2 >5) g_fHW_PID_Out2 =5;//�����޷� �޷��ٶ���0-5֮��
			if(g_fHW_PID_Out2 <0) g_fHW_PID_Out2 =0;
			if(g_cThisState != g_cLastState)//������״̬�������ϴ�״̬���ͽ��иı�Ŀ���ٶȺͿ��Ƶ�����ڶ�ʱ�������ɶ�ʱ���Ƶ��
			{
				motorPidSetSpeed(g_fHW_PID_Out1,g_fHW_PID_Out2);//ͨ��������ٶȿ��Ƶ��
			}
			
			g_cLastState = g_cThisState;//�����ϴκ���Թ�״̬	

		}
		if(g_ucMode == 2)
		{
			//***************ң��ģʽ***********************//
			//ң��ģʽ�Ŀ����ڴ��������ж�����
		}
		if(g_ucMode == 3)
		{
			//******����������ģʽ*********************//
	////�����߼�
			if(HC_SR04_Read() > 25)//ǰ�����ϰ���
			{
				motorPidSetSpeed(1,1);//ǰ�˶�
				HAL_Delay(100);
			}
			else{	//ǰ�����ϰ���
				motorPidSetSpeed(-1,1);//�ұ��˶� ԭ��	
				HAL_Delay(500);
				if(HC_SR04_Read() > 25)//�ұ����ϰ���
				{
					motorPidSetSpeed(1,1);//ǰ�˶�
					HAL_Delay(100);
				}
				else{//�ұ����ϰ���
					motorPidSetSpeed(1,-1);//����˶� ԭ��
					HAL_Delay(1000);
					if(HC_SR04_Read() >25)//������ϰ���
					{
						 motorPidSetSpeed(1,1);//ǰ�˶�
						HAL_Delay(100);
					}
					else{
						motorPidSetSpeed(-1,-1);//���˶�
						HAL_Delay(1000);
						motorPidSetSpeed(-1,1);//�ұ��˶�
						HAL_Delay(50);
					}
				}
			}
		}
		if(g_ucMode == 4)
		{
		//**********PID���湦��***********//
			g_fHC_SR04_Read=HC_SR04_Read();//��ȡǰ���ϰ������
			if(g_fHC_SR04_Read < 60){  //���ǰ60cm �ж�������������
				g_fFollow_PID_Out = PID_realize(&pidFollow,g_fHC_SR04_Read);//PID�������Ŀ���ٶ� ����ٶȣ���ͻ����ٶȼӼ�
				if(g_fFollow_PID_Out > 6) g_fFollow_PID_Out = 6;//������ٶ��޷�
				if(g_fFollow_PID_Out < -6) g_fFollow_PID_Out = -6;
				motorPidSetSpeed(g_fFollow_PID_Out,g_fFollow_PID_Out);//�ٶ�����������
			}
			else motorPidSetSpeed(0,0);//���ǰ��60cm û�ж�����ֹͣ
			HAL_Delay(10);//��ȡ���������������ܹ���
		}
		if(g_ucMode == 5)
		{
		//*************MPU6050����� PIDת�����*****************//

			sprintf((char *)Usart3String,"pitch:%.2f roll:%.2f yaw:%.2f\r\n",pitch,roll,yaw);//��ʾ6050���� ������ ����� �����
			HAL_UART_Transmit(&huart3,( uint8_t *)Usart3String,strlen(( const  char  *)Usart3String),0xFFFF);//ͨ������������ַ� strlen:�����ַ�����С	
			 
			 //mpu_dmp_get_data(&pitch,&roll,&yaw);//����ֵ:0,DMP�ɹ����ŷ����
			while(mpu_dmp_get_data(&pitch,&roll,&yaw)!=0){}  //������Խ���������������ݵ�����
			
			
			g_fMPU6050YawMovePidOut = PID_realize(&pidMPU6050YawMovement,yaw);//PID�������Ŀ���ٶ� ����ٶȣ���ͻ����ٶȼӼ�

			g_fMPU6050YawMovePidOut1 = 1.5 + g_fMPU6050YawMovePidOut;//�����ٶȼӼ�PID����ٶ�
			g_fMPU6050YawMovePidOut2 = 1.5 - g_fMPU6050YawMovePidOut;
			if(g_fMPU6050YawMovePidOut1 >3.5) g_fMPU6050YawMovePidOut1 =3.5;//�����޷�
			if(g_fMPU6050YawMovePidOut1 <0) g_fMPU6050YawMovePidOut1 =0;
			if(g_fMPU6050YawMovePidOut2 >3.5) g_fMPU6050YawMovePidOut2 =3.5;//�����޷�
			if(g_fMPU6050YawMovePidOut2 <0) g_fMPU6050YawMovePidOut2 =0;
			motorPidSetSpeed(g_fMPU6050YawMovePidOut1,g_fMPU6050YawMovePidOut2);//���������Ŀ���ٶ� ͨ��motorPidSetSpeed���Ƶ��
		
		}
		
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */