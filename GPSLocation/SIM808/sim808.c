#include "sim808.h"
#include "usart.h"		
#include "delay.h"	
#include "led.h"   	 
#include "key.h"	 	 	 	 	 
#include "string.h"    
#include "usart2.h" 

//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//ATK-sim808C GSM/GPRSģ������	  
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2016/4/1
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved	
//********************************************************************************
//��

u8 Scan_Wtime = 0;//����ɨ����Ҫ��ʱ��
u8 BT_Scan_mode=0;//����ɨ���豸ģʽ��־

u8 SIM900_CSQ[3];
u8 dtbuf[50];   								//��ӡ������	

//usmart֧�ֲ��� 
//���յ���ATָ��Ӧ�����ݷ��ظ����Դ���
//mode:0,������USART2_RX_STA;
//     1,����USART2_RX_STA;
void sim_at_response(u8 mode)
{
	if(USART2_RX_STA&0X8000)		//���յ�һ��������
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//��ӽ�����
		printf("%s",USART2_RX_BUF);	//���͵�����
		if(mode)USART2_RX_STA=0;
		
	} 
}
//////////////////////////////////////////////////////////////////////////////////
//ATK-sim808C �������(���Ų��ԡ����Ų��ԡ�GPRS���ԡ���������)���ô���

//sim808C���������,�����յ���Ӧ��
//str:�ڴ���Ӧ����
//����ֵ:0,û�еõ��ڴ���Ӧ����
//����,�ڴ�Ӧ������λ��(str��λ��)
u8* sim808c_check_cmd(u8 *str)
{
	char *strx=0;
	if(USART2_RX_STA&0X8000)		//���յ�һ��������
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//��ӽ�����
		strx=strstr((const char*)USART2_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}
//��sim808C��������
//cmd:���͵������ַ���(����Ҫ��ӻس���),��cmd<0XFF��ʱ��,��������(���緢��0X1A),���ڵ�ʱ�����ַ���.
//ack:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       1,����ʧ��
u8 sim808_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART2_RX_STA=0;
	if((u32)cmd<=0XFF)
	{
		while(DMA1_Channel7->CNDTR!=0);	//�ȴ�ͨ��7�������   
		USART2->DR=(u32)cmd;
	}else u2_printf("%s\r\n",cmd);//��������
	
	if(waittime==1100)//11s����ش�������(����ɨ��ģʽ)
	{
		 Scan_Wtime = 11;  //��Ҫ��ʱ��ʱ��
		 TIM4_SetARR(9999);//����1S��ʱ�ж�
	}
	
	
	if(ack&&waittime)		//��Ҫ�ȴ�Ӧ��
	{ 
	   while(--waittime)	//�ȴ�����ʱ
	   {
		   if(BT_Scan_mode)//����ɨ��ģʽ
		   {
			   res=KEY_Scan(0);//������һ��
			   if(res==WKUP_PRES)return 2;
		   }
		   delay_ms(10);
		   if(USART2_RX_STA&0X8000)//���յ��ڴ���Ӧ����
		   {
			   if(sim808c_check_cmd(ack))break;//�õ���Ч���� 
			   USART2_RX_STA=0;
		   } 
	   }
	   if(waittime==0)res=1; 
	}
	return res;
} 

//����sim808C�������ݣ���������ģʽ��ʹ�ã�
//request:�ڴ����������ַ���
//waittimg:�ȴ�ʱ��(��λ��10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       1,����ʧ��
u8 sim808c_wait_request(u8 *request ,u16 waittime)
{
	 u8 res = 1;
	 u8 key;
	 if(request && waittime)
	 {
		while(--waittime)
		{   
		   key=KEY_Scan(0);
		   if(key==WKUP_PRES) return 2;//������һ��
		   delay_ms(10);
		   if(USART2_RX_STA &0x8000)//���յ��ڴ���Ӧ����
		   {
			  if(sim808c_check_cmd(request)) break;//�õ���Ч����
			  USART2_RX_STA=0;
		   }
		}
		if(waittime==0)res=0;
	 }
	 return res;
}

//��1���ַ�ת��Ϊ16��������
//chr:�ַ�,0~9/A~F/a~F
//����ֵ:chr��Ӧ��16������ֵ
u8 sim808c_chr2hex(u8 chr)
{
	if(chr>='0'&&chr<='9')return chr-'0';
	if(chr>='A'&&chr<='F')return (chr-'A'+10);
	if(chr>='a'&&chr<='f')return (chr-'a'+10); 
	return 0;
}
//��1��16��������ת��Ϊ�ַ�
//hex:16��������,0~15;
//����ֵ:�ַ�
u8 sim808c_hex2chr(u8 hex)
{
	if(hex<=9)return hex+'0';
	if(hex>=10&&hex<=15)return (hex-10+'A'); 
	return '0';
}


u8 sim900a_work_test(void)
{
	if(sim808_send_cmd((u8 *)"AT",(u8 *)"OK",100))
	{
		if(sim808_send_cmd((u8 *)"AT",(u8 *)"OK",100))return SIM_COMMUNTION_ERR;	//ͨ�Ų���
	}		
	if(sim808_send_cmd((u8 *)"AT+CPIN?",(u8 *)"READY",400))return SIM_CPIN_ERR;	//û��SIM��
	if(sim808_send_cmd((u8 *)"AT+CREG?",(u8 *)"0,1",400))
	{
		if(strstr((const char*)USART2_RX_BUF,"0,5")==NULL)
		{
			 if(!sim808_send_cmd((u8 *)"AT+CSQ",(u8 *)"OK",200))	
			 {
					memcpy(SIM900_CSQ,USART2_RX_BUF+15,2);
			 }
			 return SIM_CREG_FAIL;	//�ȴ����ŵ�����
		}
	}
	printf("OKKKKKK\r\n");
	return SIM_OK;
}

u8 GSM_DectStatus(void)
{
	u8 res;
	res=sim900a_work_test();	
	switch(res)
	{
		case SIM_OK:
			printf("GSMģ���Լ�ɹ�\r\n");
			break;
		case SIM_COMMUNTION_ERR:
			printf("��GSMģ��δͨѶ�ɹ�����ȴ�\r\n");
			break;
		case SIM_CPIN_ERR:
			printf("û��⵽SIM��\r\n");	
			break;
		case SIM_CREG_FAIL:
			printf("ע�������С�����\r\n");	
			printf("��ǰ�ź�ֵ��%s\r\n", SIM900_CSQ);	
			break;		
		default:
			break;
	}
	return res;
}
/**
* ����绰
*/
u8 SIM_MAKE_CALL(u8 *number)
{
	u8 cmd[20];
	sprintf((char*)cmd,"ATD%s;",number);
	if(sim808_send_cmd(cmd,(u8 *)"OK",200))	return SIM_MAKE_CALL_ERR;
	return SIM_OK;
}

/**
* GPS����
*/
u8 GPS_Enable(void)
{
	if(sim808_send_cmd("AT+CGPSPWR=1","OK",200)) //��GPS��Դ
	{
		printf("open GPS Power Fail\n");
		return GPS_FAIL;
	}
	if(sim808_send_cmd("AT+CGNSTST=1","OK",200)) //��NMEA�������
	{
		printf("Open NMEA Data Output Fail\n");
		return GPS_FAIL;
	}
	return GPS_OK;
}

u8 SIM808_CONNECT_SERVER(u8 *IP_ADD,u8 *COM)
{		
		if(sim808_send_cmd((u8 *)"AT+CGATT?",(u8 *)": 1",100))	 return 1;
		//if(sim900a_send_cmd((u8 *)"AT+CIPHEAD=1",(u8 *)"OK",100))	 return 7;
	  if(sim808_send_cmd((u8 *)"AT+CIPSHUT",(u8 *)"OK",500))	return 2;
		if(sim808_send_cmd((u8 *)"AT+CSTT",(u8 *)"OK",200))	return 3;
		if(sim808_send_cmd((u8 *)"AT+CIICR",(u8 *)"OK",600))	return 4;
		if(!sim808_send_cmd((u8 *)"AT+CIFSR",(u8 *)"ERROR",200))	return 5;		
		sprintf((char*)dtbuf,"AT+CIPSTART=\"TCP\",\"%s\",\"%s\"",IP_ADD,COM);
	  if(sim808_send_cmd((u8 *)dtbuf,(u8 *)"CONNECT OK",200))	return 6;		
	  return 0;
}	
u8 SIM808_CONNECT_SERVER_SEND_INFOR(u8 *IP_ADD,u8 *COM)
{
	u8 res;	
	res=SIM808_CONNECT_SERVER(IP_ADD,COM);
	switch(res)
	{
		case 0:
			printf("���������ӳɹ�\n");
			break;
		case 1:
			printf("�ȴ�GSMģ�鸽������\n");
		  break;
		case 2:
			printf("�����ر�ʧ��\n");	
			break;
		case 3:
			printf("CSTTʧ��\n");	
			break;
		case 4:
			printf("CIICRʧ��\n");	
			break;
		case 5:
			printf("CIFSRʧ��\n");	
			break;
		case 6:
			printf("���ӷ�����ʧ��\n");	
			break;
		default:
			break;
	}
	return res;
}
u8 SIM808_GPRS_SEND_DATA(u8 *temp_data)
{		
	 //UART3SendString("�������ݷ��ͣ��������ݣ�",strlen("�������ݷ��ͣ��������ݣ�"));	
	 if(sim808_send_cmd("AT+CIPSEND",">",100))	 return 1;
	 //UART3SendString((u8*)temp_data,strlen((u8*)temp_data));	UART3SendString("\r\n",2);
	 if(sim808_send_cmd(temp_data,NULL,0))	return 2;
	 if(sim808_send_cmd((u8 *)0x1a,(u8 *)"SEND OK",1500))	return 3;		
	 //UART3SendString("���ݷ��ͳɹ�",strlen("���ݷ��ͳɹ�"));	UART3SendString("\r\n",2);
	 return 0;
}	

