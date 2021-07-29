#ifndef __LD3320_H
#define __LD3320_H

/*以下三个状态定义用来记录程序是在运行ASR识别还是在运行MP3播放*/
#define LD_MODE_IDLE 0x00    /*空闲状态*/
#define LD_MODE_ASR_RUN 0x08 /*语音识别*/
#define LD_MODE_MP3 0x40     /*mp3播放*/

/*以下五个状态定义用来记录程序是在运行ASR识别过程中的哪个状态*/
#define LD_ASR_NONE 0x00      /*表示没有在作ASR识别*/
#define LD_ASR_RUNING 0x01    /*表示LD3320正在作ASR识别中*/
#define LD_ASR_FOUNDOK 0x10   /*表示一次识别流程结束后，有一个识别结果*/
#define LD_ASR_FOUNDZERO 0x11 /*表示一次识别流程结束后，没有识别结果*/
#define LD_ASR_ERROR 0x31     /*表示一次识别流程中LD3320芯片内部出现不正确的状态*/

#define CLK_IN 24 /* user need modify this value according to clock in */
#define LD_PLL_11 (unsigned char)((CLK_IN / 2.0) - 1)
#define LD_PLL_MP3_19 0x0f
#define LD_PLL_MP3_1B 0x18
#define LD_PLL_MP3_1D (unsigned char)(((90.0 * ((LD_PLL_11) + 1)) / (CLK_IN)) - 1)

#define LD_PLL_ASR_19 (unsigned char)(CLK_IN * 32.0 / (LD_PLL_11 + 1) - 0.51)
#define LD_PLL_ASR_1B 0x48
#define LD_PLL_ASR_1D 0x1f

// LD chip fixed values.
#define RESUM_OF_MUSIC 0x01
#define CAUSE_MP3_SONG_END 0x20

#define MASK_INT_SYNC 0x10
#define MASK_INT_FIFO 0x04
#define MASK_AFIFO_INT 0x01
#define MASK_FIFO_STATUS_AFULL 0x08

void ld332_main(void);

#endif
