#include "ld3320.h"
#include "main.h"
#include "spi.h"
#include "usart.h"

void ld3320_reset(void);
void spi_send_byte(unsigned char byte);
unsigned char spi_receive_byte(void);
void ld3320_write_reg(unsigned char reg_addr, unsigned char data);
unsigned char ld3320_read_reg(unsigned char reg_addr);

unsigned int nMp3Size = 0;			   //mp3文件的大小
unsigned int nMp3Pos = 0;			   //mp3文件的偏移(最后的偏移就是文件大小)
unsigned char nLD_Mode = LD_MODE_IDLE; //用来记录当前是在进行ASR识别还是在播放MP3
unsigned char bMp3Play = 0;			   //用来记录播放MP3的状态

void ld3320_reset()
{
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
	HAL_Delay(100);

	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
	HAL_Delay(100);
}

void spi_send_byte(unsigned char byte)
{
	HAL_SPI_Transmit(&hspi1, &byte, 1, 0xffff);
}

unsigned char spi_receive_byte()
{
	unsigned char data;
	HAL_SPI_Receive(&hspi1, &data, 1, 0xffff);
	return data;
}

void ld3320_write_reg(unsigned char reg_addr, unsigned char data)
{
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);

	spi_send_byte(0x04);

	spi_send_byte(reg_addr);

	spi_send_byte(data);

	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

unsigned char ld3320_read_reg(unsigned char reg_addr)
{
	unsigned char data = 0;
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);

	spi_send_byte(0x05);

	spi_send_byte(reg_addr);

	data = spi_receive_byte();

	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);

	return data;
}

int ld3320_check()
{
	unsigned char data = 0;
	data = ld3320_read_reg(0x06);
	if ((data != 0x00) && (data != 0x87)) /*第一次读0x06，相当于激活芯片，值为0x00或0x87*/
	{
		return -1;
	}

	data = ld3320_read_reg(0x06);
	if (data != 0x87) /*第二次读0x06，值为0x87*/
	{
		return -1;
	}

	data = ld3320_read_reg(0x35);
	if (data != 0x80) /*读0x35，值为0x80*/
	{
		return -1;
	}

	data = ld3320_read_reg(0xb3);
	if (data != 0xff) /*读0xb3，值为0xff*/
	{
		return -1;
	}

	return 0;
}

void LD_Init_Common()
{
	ld3320_read_reg(0x06);
	ld3320_write_reg(0x17, 0x35);
	HAL_Delay(5);
	ld3320_read_reg(0x06);

	ld3320_write_reg(0x89, 0x03);
	HAL_Delay(5);
	ld3320_write_reg(0xCF, 0x43);
	HAL_Delay(5);
	ld3320_write_reg(0xCB, 0x02);

	/*PLL setting*/
	ld3320_write_reg(0x11, LD_PLL_11);
	if (nLD_Mode == LD_MODE_MP3)
	{
		ld3320_write_reg(0x1E, 0x00);
		ld3320_write_reg(0x19, LD_PLL_MP3_19);
		ld3320_write_reg(0x1B, LD_PLL_MP3_1B);
		ld3320_write_reg(0x1D, LD_PLL_MP3_1D);
	}
	else
	{
		ld3320_write_reg(0x1E, 0x00);
		ld3320_write_reg(0x19, LD_PLL_ASR_19);
		ld3320_write_reg(0x1B, LD_PLL_ASR_1B);
		ld3320_write_reg(0x1D, LD_PLL_ASR_1D);
	}
	HAL_Delay(10);

	ld3320_write_reg(0xCD, 0x04);
	ld3320_write_reg(0x17, 0x4c);
	HAL_Delay(5);
	ld3320_write_reg(0xB9, 0x00);
	ld3320_write_reg(0xCF, 0x4F);
	ld3320_write_reg(0x6F, 0xFF);
}

/*语音识别初始化*/
void LD_Init_ASR(void)
{
	nLD_Mode = LD_MODE_ASR_RUN; /*当前进行语音识别*/
	LD_Init_Common();			/*通用初始化*/

	ld3320_write_reg(0xBD, 0x00);
	ld3320_write_reg(0x17, 0x48); /*写48H可以激活DSP*/
	HAL_Delay(100);

	ld3320_write_reg(0x3C, 0x80);
	ld3320_write_reg(0x3E, 0x07);
	ld3320_write_reg(0x38, 0xff);
	ld3320_write_reg(0x3A, 0x07);
	HAL_Delay(100);

	ld3320_write_reg(0x40, 0);
	ld3320_write_reg(0x42, 8);
	ld3320_write_reg(0x44, 0);
	ld3320_write_reg(0x46, 8);
	HAL_Delay(100);
}

//检测芯片内部有无出错
// Return 1: success.
unsigned char LD_Check_ASRBusyFlag_b2()
{
	unsigned char j;
	unsigned char flag = 0;
	for (j = 0; j < 10; j++)
	{
		if (ld3320_read_reg(0xb2) == 0x21)
		{
			flag = 1;
			break;
		}
		HAL_Delay(10);
	}
	return flag;
}

// Return 1: success.
//	添加识别关键词语，开发者可以学习"语音识别芯片LD3320高阶秘籍.pdf"中关于垃圾词语吸收错误的用法
unsigned char LD_AsrAddFixed(void) //添加关键词语到LD3320芯片中
{
	//	return add_item_from_ini(ini_file_name);
	unsigned char k, flag;
	unsigned char nAsrAddLength;
#define DATE_A 11 //识别的语句数量
#define DATE_B 25 //识别的拼音长度，最长72个字母

	// 以下为识别语句的拼音，例如“开灯”对应写入“kai deng”
	unsigned char sRecog[DATE_A][DATE_B] = {
		"kai deng",
		"guan deng",
		"quan bu da kai",
		"quan bu guan bi",
		"liu shui deng",
		"shan shuo deng",
		"da kai ji dian qi",
		"ji dian qi dian dong",
		"bo fang ge qu",
		"ni jiao shen me ming zi",
		"ni hui zuo shen me",
	};																   /*添加关键词，用户修改*/
																	   //以下为识别码的宏定义无特别意义，0-ff可自行修改值。
	unsigned char pCode[DATE_A] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; /*添加识别码，用户修改*/

	flag = 1;
	for (k = 0; k < DATE_A; k++)
	{
		if (LD_Check_ASRBusyFlag_b2() == 0)
		{
			flag = 0;
			break;
		}

		ld3320_write_reg(0xc1, pCode[k]);
		ld3320_write_reg(0xc3, 0);
		ld3320_write_reg(0x08, 0x04);
		ld3320_write_reg(0x08, 0x00);

		for (nAsrAddLength = 0; nAsrAddLength < DATE_B; nAsrAddLength++)
		{
			if (sRecog[k][nAsrAddLength] == 0)
				break;
			ld3320_write_reg(0x5, sRecog[k][nAsrAddLength]);
		}
		ld3320_write_reg(0xb9, nAsrAddLength);
		ld3320_write_reg(0xb2, 0xff);
		ld3320_write_reg(0x37, 0x04);
	}

	return flag;
}

void LD_AsrStart()
{
	LD_Init_ASR();
}

// Return 1: success.
unsigned char LD_AsrRun() //开始识别
{
	ld3320_write_reg(0x35, 0x4c); //麦克风音量 范围：00-7fH---建议范围：40-55H  ,,,值越大越灵敏，也越容易误识别
	ld3320_write_reg(0xB3, 0xf);

	ld3320_write_reg(0x1C, 0x09); //ADC开关控制 写09H Reserve保留命令字
	ld3320_write_reg(0xBD, 0x20); //初始化控制寄存器 写入20H；Reserve保留命令字
	ld3320_write_reg(0x08, 0x01); //清除FIFO内容第0位：写入1→清除FIFO_DATA 第2位：写入1→清除FIFO_EXT
	HAL_Delay(1);
	ld3320_write_reg(0x08, 0x00); //清除FIFO内容第0位（清除指定FIFO后再写入一次00H）
	HAL_Delay(1);

	if (LD_Check_ASRBusyFlag_b2() == 0) //芯片内部出错
	{
		return 0;
	}

	ld3320_write_reg(0xB2, 0xff); //ASR：DSP忙闲状态 0x21 表示闲，查询到为闲状态可以进行下一步 ??? 为什么不是read??
	ld3320_write_reg(0x37, 0x06); //识别控制命令下发寄存器 写06H 通知DSP开始识别语音 下发前，需要检查B2寄存器
	HAL_Delay(5);
	ld3320_write_reg(0x1C, 0x0b); // ADC开关控制  写0BH 麦克风输入ADC通道可用
	ld3320_write_reg(0x29, 0x10); //中断允许 第2位：FIFO 中断允许 第4位：同步中断允许 1允许；0不允许

	ld3320_write_reg(0xBD, 0x00); //初始化控制寄存器 写入00 然后启动 为ASR模块

	return 1;
}

unsigned char RunASR(void)
{
	unsigned char i = 0;
	unsigned char j;
	unsigned char asrflag = 0;
	for (i = 0; i < 5; i++) //	防止由于硬件原因导致LD3320芯片工作不正常，所以一共尝试5次启动ASR识别流程
	{
		LD_AsrStart();
		HAL_Delay(100);
		if (LD_AsrAddFixed() == 0)
		{
			ld3320_reset(); //	LD3320芯片内部出现不正常，立即重启LD3320芯片
			HAL_Delay(100); //	并从初始化开始重新ASR识别流程
			continue;
		}
		HAL_Delay(100);
		j = LD_AsrRun();
		if (j == 0)
		{
			ld3320_reset(); //	LD3320芯片内部出现不正常，立即重启LD3320芯片
			HAL_Delay(100); //	并从初始化开始重新ASR识别流程
			continue;
		}

		asrflag = 1;
		break; //	ASR流程启动成功，退出当前for循环。开始等待LD3320送出的中断信号
	}

	return asrflag;
}

unsigned char nAsrStatus = 0;

unsigned char LD_GetResult()
{
	unsigned char res;

	res = ld3320_read_reg(0xc5);

	return res;
}

//用户根据自己的配置文件自行修改
void deal_the_index(char nIndex)
{
	switch (nIndex)
	{
	case 0:
		break;
	case 1: /*开灯*/
		HAL_UART_Transmit(&huart1, (unsigned char *)"1", sizeof("1"), 0xffff);
		break;
	case 2: /*关灯*/
		HAL_UART_Transmit(&huart1, (unsigned char *)"2", sizeof("2"), 0xffff);
		break;
	case 3: /*全部打开*/
		HAL_UART_Transmit(&huart1, (unsigned char *)"3", sizeof("3"), 0xffff);
		break;
	case 4: /*全部关闭*/
		break;
	case 5: /*流水灯*/

		break;
	case 6: /*闪烁灯*/

		break;
	case 7: /*打开继电器*/

		break;
	case 8: /*继电器点动*/

		break;
	case 9: /*播放歌曲*/

		break;
	case 10: /*你叫什么名字*/

		break;
	case 11: /*你会做什么*/

		break;
	case 12:

		break;
	case 13:

		break;
	case 14:

		break;
	case 15:

		break;
	case 16:

		break;
	case 17:

		break;
	case 18:

		break;
	case 19:

		break;
	case 20:

		break;
	case 21:

		break;
	case 22:

		break;
	case 23:

		break;
	case 24:

		break;
	case 25:

		break;
	case 26:

		break;
	case 27:

		break;
	case 28:

		break;
	case 29:

		break;
	case 30:

		break;
	case 31:

		break;
	case 32:

		break;
	case 33:

		break;
	case 34:

		break;
	case 35:

		break;
	case 36:

		break;
	case 37:

		break;
	case 38:

		break;
	case 39:

		break;
	case 40:

		break;
	case 41:

		break;
	case 42:

		break;
	case 43:

		break;
	case 44:

		break;
	case 45:

		break;
	case 46:

		break;
	case 47:

		break;
	case 48:

		break;
	case 49:

		break;
	}
}

unsigned char ucRegVal;	 //寄存器备份变量
unsigned char ucHighInt; //寄存器备份变量
unsigned char ucLowInt;	 //寄存器备份变量

void ProcessInt0(void) //播放 语音识别中断
{
	unsigned char nAsrResCount = 0;

	ucRegVal = ld3320_read_reg(0x2B);

	if (nLD_Mode == LD_MODE_ASR_RUN) //当前进行语音识别
	{
		// 语音识别产生的中断
		// （有声音输入，不论识别成功或失败都有中断）
		ld3320_write_reg(0x29, 0);			 //中断允许 FIFO 中断允许 0表示不允许
		ld3320_write_reg(0x02, 0);			 // FIFO中断允许	 FIFO_DATA FIFO_EXT中断   不允许
		if ((ucRegVal & 0x10) &&			 //2b第四位为1 芯片内部FIFO中断发生 MP3播放时会产生中断标志请求外部MCU向FIFO_DATA中Reload数
			ld3320_read_reg(0xb2) == 0x21 && //读b2得到0x21表示闲可以进行下一步ASR动作
			ld3320_read_reg(0xbf) == 0x35)	 //读到数值为0x35，可以确定是一次语音识别流程正常结束
		{
			nAsrResCount = ld3320_read_reg(0xba); //ASR：中断时，判断语音识别有几个识别候选
			if (nAsrResCount == 1)
			{
				ld3320_read_reg(0xc5);
			}
			if (nAsrResCount > 0 && nAsrResCount <= 4) //1 – 4: N个识别候选 0或者大于4：没有识别候选
			{
				nAsrStatus = LD_ASR_FOUNDOK; //表示一次识别流程结束后，有一个识别结果
			}
			else
			{
				nAsrStatus = LD_ASR_FOUNDZERO; //表示一次识别流程结束后，没有识别结果
			}
		}
		else
		{
			nAsrStatus = LD_ASR_FOUNDZERO; //表示一次识别流程结束后，没有识别结果
		}

		ld3320_write_reg(0x2b, 0);
		ld3320_write_reg(0x1C, 0); //ADC开关控制 写00H ADC不可用
		return;
	}

	// 声音播放产生的中断，有三种：
	// A. 声音数据已全部播放完。
	// B. 声音数据已发送完毕。
	// C. 声音数据暂时将要用完，需要放入新的数据。
	ucHighInt = ld3320_read_reg(0x29);
	ucLowInt = ld3320_read_reg(0x02);
	ld3320_write_reg(0x29, 0);
	ld3320_write_reg(0x02, 0);
	if (ld3320_read_reg(0xBA) & CAUSE_MP3_SONG_END)
	{
		// A. 声音数据已全部播放完。

		ld3320_write_reg(0x2B, 0);
		ld3320_write_reg(0xBA, 0);
		ld3320_write_reg(0xBC, 0x0);
		bMp3Play = 0; // 声音数据全部播放完后，修改bMp3Play的变量
		ld3320_write_reg(0x08, 1);

		ld3320_write_reg(0x08, 0);
		ld3320_write_reg(0x33, 0);
		return;
	}

	if (nMp3Pos >= nMp3Size)
	{
		// B. 声音数据已发送完毕。

		ld3320_write_reg(0xBC, 0x01);
		ld3320_write_reg(0x29, 0x10);
		return;
	}

	// C. 声音数据暂时将要用完，需要放入新的数据。

	ld3320_write_reg(0x29, ucHighInt);
	ld3320_write_reg(0x02, ucLowInt);
}

void ld332_main()
{
	ld3320_reset();
	if (ld3320_check() != 0)
	{
		while (1)
		{
		}
	}

	ld3320_reset();
	if (ld3320_check() != 0)
	{
		while (1)
		{
		}
	}

	HAL_UART_Transmit(&huart1, (unsigned char *)"ok\n", sizeof("ok"), 0xffff);

	while (1)
	{
		if (HAL_GPIO_ReadPin(IRQ_GPIO_Port, IRQ_Pin) == GPIO_PIN_RESET)
		{
			ProcessInt0();
		}

		switch (nAsrStatus)
		{
		case LD_ASR_RUNING: //  LD3320芯片正在语音识别中
			break;

		case LD_ASR_ERROR: //  LD3320芯片发生错误
			break;

		case LD_ASR_NONE: //  LD3320芯片无动作
		{

			nAsrStatus = LD_ASR_RUNING;
			if (RunASR() == 0) //	启动一次ASR识别流程：ASR初始化，ASR添加关键词语，启动ASR运算
			{
				nAsrStatus = LD_ASR_ERROR;
			}
			break;
		}
		case LD_ASR_FOUNDOK: //  LD3320芯片识别到语音
		{
			int nAsrRes = LD_GetResult(); //一次ASR识别流程结束，去取ASR识别结果 （获取识别码）
			deal_the_index(nAsrRes);	  //在此添加播放函数
			nAsrStatus = LD_ASR_NONE;
			break;
		}
		case LD_ASR_FOUNDZERO: //无识别结果
			nAsrStatus = LD_ASR_NONE;
			break;
		default:
		{
			nAsrStatus = LD_ASR_NONE;
			break;
		}
		}
	}
}
