#include "delay.h"
#include "sys.h"
#include "led.h"
#include "key.h"
#include "usart.h"	
#include "sim808.h"
#include "string.h"    
#include "usart2.h" 
#include "gps.h"

u8 USART1_TX_BUF[USART2_MAX_RECV_LEN]; 
nmea_msg gpsx; 
__align(4) u8 gpsbuf1[50];  
__align(4) u8 gpsbuf2[50];

void sim800c_init(void);
int Gps_Msg_Show(void);

int main(void)
{
	u8 res;
	int i = 0, ret = 0;
	u16 rxlen;
	u8 p[20],*p1,*p2;
	int isConnect = 0;

	NVIC_Configuration(); 	 //����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	delay_init();	    	 //��ʱ������ʼ��	  
	uart_init(9600);	 	//���ڳ�ʼ��Ϊ9600
	LED_Init();		  		//��ʼ����LED���ӵ�Ӳ���ӿ�
	USART2_Init(115200);	//��GSMģ��ͨ��

	printf("system init OK\n");
	
	// 1.GPS Init
	if(GPS_Enable())
	{
		printf("GPS OK\n");
	}

	// 2.��ʼ�� ���״̬
	res=GSM_DectStatus();//���״̬����ʾ
	delay_ms(2000);
	while(1)
	{		
		// 4.GPS
		delay_ms(100);
		if(USART2_RX_STA & 0X8000)		//���յ�����
		{
			rxlen=USART2_RX_STA & 0X7FFF;	//�õ����ݳ���
			for(i = 0; i < rxlen; i++)
			{
				USART1_TX_BUF[i]=USART2_RX_BUF[i];	   
 			}
			USART2_RX_STA = 0;		   	//׼����һ�εĽ���
			USART1_TX_BUF[i] = 0;			//
			GPS_Analysis(&gpsx,(u8*)USART1_TX_BUF);//
			printf("\r\nGPS Data:%s\r\n",USART1_TX_BUF);
			ret = Gps_Msg_Show();
		}
		if(ret == 0)
		{
			continue;
		}
		res = 0;
		if(sim808_send_cmd((u8 *)"AT+CIFSR",(u8 *)"ERROR",200))	//���ӶϿ�
		{
			printf("connect disable \n");
			res = 1;
		}
				
		while(res)
		{
			res=SIM808_CONNECT_SERVER_SEND_INFOR((u8*)"wn20858756.iask.in",(u8*)"2317");
			delay_ms(200);
		}
		SIM808_GPRS_SEND_DATA(gpsbuf1);
		SIM808_GPRS_SEND_DATA(gpsbuf2);
	}
}

int Gps_Msg_Show(void)
{
 	float tp;		   
	tp=gpsx.longitude;
	if(tp == 0)
	{
		return 0;
	}
	
	sprintf((char *)gpsbuf1,"%1c%.5f ", gpsx.ewhemi, tp/=100000);
	printf("Buf: %s \n",gpsbuf1);	 	

	tp=gpsx.latitude;	   
	sprintf((char *)gpsbuf2,"%1c%.5f ", gpsx.nshemi, tp/=100000);
	printf("Buf: %s \n",gpsbuf2);
	
	return 1;
}


