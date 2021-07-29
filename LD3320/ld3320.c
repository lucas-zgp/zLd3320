#include "ld3320.h"
#include "main.h"
#include "spi.h"
#include "usart.h"


void ld3320_reset(void);
void spi_send_byte(unsigned char byte);
unsigned char spi_receive_byte(void);
void ld3320_write_reg(unsigned char reg_addr,unsigned char data);
unsigned char ld3320_read_reg(unsigned char reg_addr);
	


unsigned char glo_data[10] = {0};

void ld3320_reset()
{
	HAL_GPIO_WritePin(RST_GPIO_Port,RST_Pin,GPIO_PIN_SET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(RST_GPIO_Port,RST_Pin,GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(RST_GPIO_Port,RST_Pin,GPIO_PIN_SET);
	HAL_Delay(100);
	
	HAL_GPIO_WritePin(CS_GPIO_Port,CS_Pin,GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(CS_GPIO_Port,CS_Pin,GPIO_PIN_SET);
	HAL_Delay(100);
}

void spi_send_byte(unsigned char byte)
{
	HAL_SPI_Transmit(&hspi1,&byte,1,0xffff);
}


unsigned char spi_receive_byte()
{
	unsigned char data;
	HAL_SPI_Receive(&hspi1,&data,1,0xffff);
	return data;
}

void ld3320_write_reg(unsigned char reg_addr,unsigned char data)
{
	HAL_GPIO_WritePin(CS_GPIO_Port,CS_Pin,GPIO_PIN_RESET);
	
	spi_send_byte(0x04);
	
	spi_send_byte(reg_addr);
	
	spi_send_byte(data);
	
	HAL_GPIO_WritePin(CS_GPIO_Port,CS_Pin,GPIO_PIN_SET);
}


unsigned char ld3320_read_reg(unsigned char reg_addr)
{
	unsigned char data = 0;
	HAL_GPIO_WritePin(CS_GPIO_Port,CS_Pin,GPIO_PIN_RESET);
	
	spi_send_byte(0x05);
	
	spi_send_byte(reg_addr);
	
	data = spi_receive_byte();
	
	HAL_GPIO_WritePin(CS_GPIO_Port,CS_Pin,GPIO_PIN_SET);
	
	return data;
}


int ld3320_check()
{
	unsigned char data = 0;
	data = ld3320_read_reg(0x06);
	if((data != 0x00) && (data != 0x87)) /*第一次读0x06，相当于激活芯片，值为0x00或0x87*/
	{
		return -1;
	}
	
	data = ld3320_read_reg(0x06);
	if(data != 0x87) /*第二次读0x06，值为0x87*/
	{
		return -1;
	}
	
	data = ld3320_read_reg(0x35);
	if(data != 0x80) /*读0x35，值为0x80*/
	{
		return -1;
	}
	
	data = ld3320_read_reg(0xb3);
	if(data != 0xff) /*读0xb3，值为0xff*/
	{
		return -1;
	}
	
	return 0;
}




void ld332_main()
{
	ld3320_reset();	
	if(ld3320_check()!=0)
	{
		while(1);
	}
	
	ld3320_reset();	
	if(ld3320_check()!=0)
	{
		while(1);
	}
	
	HAL_UART_Transmit(&huart1,(unsigned char*)"ok\n",sizeof("ok"),0xffff);
}










