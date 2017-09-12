#include "ade7753.h"
#include "spi.h"
#include "delay.h"

#include "freertos.h"
#include "task.h"

#include <math.h>


//vTaskSuspendAll
//xTaskResumeAll


uint8_t ade7753_wr_buf[3] = {0, 0, 0};
uint8_t ade7753_rd_buf[3] = {0, 0, 0};

_SRT_ADE7753_REGS_ ade7753_regs[3];
uint8_t ade7753_test = 0;

// vrms reading
#define VOLTAGE_FILTER	100
//irms reading
#define CURRENT_FILTER	100


_TYPE_CAL_ vrms_cal[3];
_TYPE_CAL_ irms_cal[3];


uint16_t test_counter = 0;

void write_ade7753_mode_register(uint8_t phase);

void ade7753_reset_control(uint8_t phase, uint8_t enable)
{
	if(phase == PHASE_A)
	{
		PEout(0) = enable;
	}
	else if(phase == PHASE_B)
	{
		PEout(4) = enable;
	}
	else if(phase == PHASE_C)
	{
		PEout(8) = enable;
	}
}

void ade7753_cs_control(uint8_t phase, uint8_t enable)
{
	if(phase == PHASE_A)
	{
		PAout(4) = enable;
	}
	else if(phase == PHASE_B)
	{
		PAout(3) = enable;
	}
	else if(phase == PHASE_C)
	{
		PAout(2) = enable;
	}
}

void ade7753_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// init cs: phase A -- PA4, phase B -- PA3, phase C -- PA2 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_3 | GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure); 
	GPIO_SetBits(GPIOA, GPIO_InitStructure.GPIO_Pin);
	
	// init reset, irq, sag, zx pins
	// reset: A - PE0, B - PE4, C - PE8
	// irq:   A - PE1, B - PE5, C - PE9
	// sag:   A - PE2, B - PE6, C - PE10
	// zx:    A - PE3, B - PE7, C - PE11
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_4 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure); 
	GPIO_SetBits(GPIOE, GPIO_InitStructure.GPIO_Pin);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 \
								| GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 \
								| GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOE, &GPIO_InitStructure); 
	GPIO_SetBits(GPIOE, GPIO_InitStructure.GPIO_Pin);
}

// there are some problems using interrupt mode, so it uses poll mode now
void ade7753_interrupt_init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

// ade7753 irq interrupt	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource1/* | GPIO_PinSource5 | GPIO_PinSource9*/); 
	EXTI_InitStructure.EXTI_Line = EXTI_Line1/* | EXTI_Line5 | EXTI_Line9*/; 
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	
	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	
//	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
//	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_Init(&NVIC_InitStructure);
	
// ade7753 zero corss interrupt
	// ade7753 irq interrupt	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource3/* | GPIO_PinSource7 | GPIO_PinSource11*/); 
	EXTI_InitStructure.EXTI_Line = EXTI_Line3/* | EXTI_Line7 | EXTI_Line11*/; 
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//EXTI_Trigger_Falling, EXTI_Trigger_Rising
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	
	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
	NVIC_Init(&NVIC_InitStructure);
}

void write_ade7753(uint8_t cmd, uint8_t *pbuf, uint8_t length)
{
	uint8_t i;
	SPI1_ReadWriteByte(cmd | 0x80);
	for(i = 0; i < length; i++)
	{
		SPI1_ReadWriteByte(pbuf[i]);
	}
}

void read_ade7753(uint8_t cmd, uint8_t *pbuf, uint8_t length)
{
	uint8_t i;
	SPI1_ReadWriteByte(cmd);
	for(i = 0; i < length; i++)
	{
		pbuf[i] = SPI1_ReadWriteByte(0xfe);	
	}
}

void write_ade7753_interrupt(uint8_t phase)
{
	ade7753_wr_buf[0] = ade7753_regs[phase].rw_interrupt_enable.flags >> 8;
	ade7753_wr_buf[1] = ade7753_regs[phase].rw_interrupt_enable.flags & 0xff;
	
	ade7753_cs_control(phase, CLR);
	write_ade7753(ADE7753_REG_IRQEN_RW, ade7753_wr_buf, 2);
	ade7753_cs_control(phase, SET);
}

void read_ade7753_interrupt(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_IRQEN_RW, ade7753_rd_buf, 2);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_interrupt_enable.flags = (ade7753_rd_buf[0] << 8) | ade7753_rd_buf[1];
}

void ade7753_init(void)
{
	ade7753_gpio_init();
//	ade7753_interrupt_init();
	SPI1_Init();
	
	ade7753_reset_control(PHASE_A, CLR);
	ade7753_reset_control(PHASE_B, CLR);
	ade7753_reset_control(PHASE_C, CLR);
	delay_ms(10);
	ade7753_reset_control(PHASE_A, SET);
	ade7753_reset_control(PHASE_B, SET);
	ade7753_reset_control(PHASE_C, SET);
	delay_ms(10);

	// enable CF output
	ade7753_regs[PHASE_A].rw_ade7753_mode.mode.discf = 0;
	ade7753_regs[PHASE_A].rw_ade7753_mode.mode.wavsel = 3;
	
	write_ade7753_mode_register(PHASE_A);
	delay_ms(1);
	
//	write_ade7753_interrupt(PHASE_A);
}

uint8_t set_ade7753_waveform_source(uint8_t phase, uint8_t waveform_source)
{
	if((waveform_source > 3) || (waveform_source == 1)) // invalid value
		return 1;
	
	ade7753_regs[phase].rw_ade7753_mode.mode.wavsel = waveform_source;
	write_ade7753_mode_register(phase);
	
//	if((waveform_source == 2) || (waveform_source == 3))
//	{
//		ade7753_regs[phase].rw_interrupt_enable.flag.wsmp = 1;
////		ade7753_regs[phase].rw_interrupt_enable.flag.zx = 1;
//		
//		write_ade7753_interrupt(phase);
////		delay_ms(1000);
//	}
	
	return 0;
}

void read_ade7753_waveform(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_WAVEFORM_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].ro_waveform = (int32_t)((ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2]);
}

void read_ade7753_active_energy(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_AENERGY_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].ro_active_energy = (int32_t)((ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2]);
}

void read_ade7753_active_energy_reset(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_RAENERGY_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].ro_active_energy_reset = (int32_t)((ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2]);
}

int32_t read_ade7753_line_accumulation_active_energy(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_LENERGY_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	return (int32_t)((ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2]);
}

void read_ade7753_apparent_energy(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_VAENERGY_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
//	ade7753_regs[phase].ro_apparent_energy = (uint32_t)((ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2]);
}

void read_ade7753_apparent_energy_reset(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_RVAENERGY_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].ro_apparent_energy_reset = (uint32_t)((ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2]);
}

uint32_t read_ade7753_line_accumulation_apparent_energy(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_LVAENERGY_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
//	ade7753_regs[phase].ro_line_accumulation_apparent_energy = (uint32_t)((ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2]);
	return (uint32_t)((ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2]);
}

void read_ade7753_line_accumulation_reactive_energy(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_LVARENERGY_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].ro_line_accumulation_reactive_energy = (int32_t)((ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2]);
}

void read_ade7753_mode_register(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_MODE_RW, ade7753_rd_buf, 2);
	ade7753_cs_control(phase, SET);
	
//	ade7753_regs[phase].rw_ade7753_mode.modes = (ade7753_rd_buf[0] << 8) | ade7753_rd_buf[1];
}

void write_ade7753_mode_register(uint8_t phase)
{
	ade7753_wr_buf[0] = ade7753_regs[PHASE_A].rw_ade7753_mode.modes >> 8;
	ade7753_wr_buf[1] = ade7753_regs[PHASE_A].rw_ade7753_mode.modes & 0xff;
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_MODE_RW, ade7753_wr_buf, 2);
	ade7753_cs_control(phase, SET);
	
//	ade7753_regs[phase].rw_ade7753_mode.modes = mode;
}

void read_ade7753_offset(uint8_t phase, uint8_t channel)
{
	ade7753_cs_control(phase, CLR);
	if(channel == CHANNEL1)
	{
		read_ade7753(ADE7753_REG_CH1OS_RW, ade7753_rd_buf, 1);
	}
	else
	{
		read_ade7753(ADE7753_REG_CH2OS_RW, ade7753_rd_buf, 1);
	}
	ade7753_cs_control(phase, SET);
	
	if(channel == CHANNEL1)
	{
		ade7753_regs[phase].rw_offset_channel1 = (int8_t)ade7753_rd_buf[0];
	}
	else
	{
		ade7753_regs[phase].rw_offset_channel2 = (int8_t)ade7753_rd_buf[0];
	}
}

void write_ade7753_offset(uint8_t phase, uint8_t channel, int8_t offset)
{
	ade7753_wr_buf[0] = offset;
	
	ade7753_cs_control(phase, CLR);
	if(channel == CHANNEL1)
	{
		write_ade7753(ADE7753_REG_CH1OS_RW, ade7753_wr_buf, 1);
	}
	else
	{
		write_ade7753(ADE7753_REG_CH2OS_RW, ade7753_wr_buf, 1);
	}
	ade7753_cs_control(phase, SET);
	
	//电压offset与vrms是1:1的关系，所以可以直接加上
	//电流offset与irms是1:32768的关系，所以需要平方和再开方计算
}

void read_pga_gain(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_GAIN_RW, ade7753_rd_buf, 1);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_pga_gain.pca_gain = ade7753_rd_buf[0];
}

void write_pga_gain(uint8_t phase)
{
	ade7753_wr_buf[0] = ade7753_regs[phase].rw_pga_gain.pca_gain;
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_GAIN_RW, ade7753_wr_buf, 1);
	ade7753_cs_control(phase, SET);
}

//通过调整电压相位延时时间来使之与电流同相，用于校正功率计算
//对于3.579545MHz的晶振，每个刻度时间为2.22us
//6位有符号数，0x21~0x1F (-31 ~ +31) == (-102.12us ~ +39.96us), 默认值为0X0D代表0延时
#define NS_PER_BIT	2220
void read_phase_calibration(uint8_t phase)
{
	
}

void write_phase_calibration(uint8_t phase)
{
	ade7753_wr_buf[0] = ade7753_regs[phase].rw_phase_calibration;
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_PHCAL_RW, ade7753_wr_buf, 1);
	ade7753_cs_control(phase, SET);
}

//void write_phase_calibration(uint8_t phase, int32_t phase_delay_ns_adjust)
//{
//	int8_t bit_counter;
//	if((phase_delay_ns_adjust < -102120) || (phase_delay_ns_adjust > 39960)) // 非法的值
//		return;
//	
//	bit_counter = (int8_t)(1.0 * phase_delay_ns_adjust / NS_PER_BIT);

//	ade7753_regs[phase].rw_phase_calibration = bit_counter + 0x0D;
//	ade7753_regs[phase].rw_phase_calibration &= 0xc0;	//为负数时，最高两位仍然为0
//	
//	ade7753_wr_buf[0] = ade7753_regs[phase].rw_phase_calibration;
//	  
//	ade7753_cs_control(phase, CLR);
//	read_ade7753(ADE7753_REG_PHCAL_RW, ade7753_wr_buf, 1);
//	ade7753_cs_control(phase, SET);
//}

void read_ade7753_temperature(uint8_t phase)
{
	ade7753_wr_buf[0] = MODE_WAVE_CH2;
	ade7753_wr_buf[1] = MODE_TEMPSEL | MODE_DISCF | MODE_DISSAG;
	
	ade7753_cs_control(phase, CLR);
	write_ade7753(ADE7753_REG_MODE_RW, ade7753_wr_buf, 2);
	ade7753_cs_control(phase, SET);
	
	do
	{
		delay_ms(1);
		read_ade7753_mode_register(phase);
	} while(ade7753_regs[phase].rw_ade7753_mode.mode.tempsel == 1);
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_TEMP_RO, ade7753_rd_buf, 1);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].ro_temperature = ade7753_rd_buf[0];
}

// eg. this function returns 511 if the frequency is 51.1 HZ
void read_frequency(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_PERIOD_RO, ade7753_rd_buf, 2);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].ro_voltage_period = (ade7753_rd_buf[0] << 8) | ade7753_rd_buf[1];
	
	ade7753_regs[phase].voltage_frequency = (ADE7753_CLOCK * 10) / (ade7753_regs[phase].ro_voltage_period * 8);
}

// read the interrupt status
void read_status(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_STATUS_RO, ade7753_rd_buf, 2);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].ro_interrupt_status.flags = (ade7753_rd_buf[0] << 8) | ade7753_rd_buf[1];
}

// read the interrupt status, and then clear the flags
void read_reset_status(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_RSTSTATUS_RO, ade7753_rd_buf, 2);
	ade7753_cs_control(phase, SET);
	
//	ade7753_regs[phase].ro_interrupt_status.flags = (ade7753_rd_buf[0] << 8) | ade7753_rd_buf[1];
}

//the zero crossing timeout, 12-bit register
//the default value is the maximum 0xFFF, time = 128/CLKIN * 2^12 = 0.15s
void read_zx_timeout(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_ZXTOUT_RW, ade7753_rd_buf, 2);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_zx_timeout = (ade7753_rd_buf[0] << 8) | ade7753_rd_buf[1];
}

void write_zx_timeout(uint8_t phase, uint16_t timeout)
{
	ade7753_wr_buf[0] = timeout >> 8;
	ade7753_wr_buf[1] = timeout & 0xff;
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_ZXTOUT_RW, ade7753_wr_buf, 2);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_zx_timeout = timeout;
}

void write_line_cycle(uint8_t phase)
{
	ade7753_wr_buf[0] = ade7753_regs[phase].rw_line_cycle >> 8;
	ade7753_wr_buf[1] = ade7753_regs[phase].rw_line_cycle & 0xff;
	
	ade7753_cs_control(phase, CLR);
	write_ade7753(ADE7753_REG_LINECYC_RW, ade7753_wr_buf, 2);
	ade7753_cs_control(phase, SET);
}

void read_line_cycle(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_LINECYC_RW, ade7753_rd_buf, 2);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_line_cycle = (ade7753_rd_buf[0] << 8) | ade7753_rd_buf[1];
}

void write_sag_line_cycle(uint8_t phase, uint8_t cycle)
{
	if(cycle < 2) cycle = 2;
	ade7753_wr_buf[0] = cycle;
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_SAGCYC_RW, ade7753_wr_buf, 1);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_sag_cycle = cycle;
}

void read_sag_line_cycle(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_SAGCYC_RW, ade7753_rd_buf, 1);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_sag_cycle = ade7753_rd_buf[0];
}

// the sag pin goes active low if the line voltage is less than the 'level' 'cycle' times. 
void write_sag_voltage_level(uint8_t phase, uint8_t level)
{
	ade7753_wr_buf[0] = level;
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_SAGLVL_RW, ade7753_wr_buf, 1);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_sag_level = level;
}

void read_sag_voltage_level(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_SAGLVL_RW, ade7753_rd_buf, 1);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_sag_level = ade7753_rd_buf[0];
}

void read_voltage_peak(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_VPEAK_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].ro_voltage_peak = (ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2];
}

void read_voltage_peak_reset(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_RSTVPEAK_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
//	ade7753_regs[phase].ro_voltage_peak = (ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2];
}

void write_voltage_peak_level(uint8_t phase, uint8_t level)
{
	ade7753_wr_buf[0] = level;
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_VPKLVL_RW, ade7753_wr_buf, 1);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_voltage_peak_level_thershold = level;
}

void read_voltage_rms(uint8_t phase)
{
	uint32_t read_tmp;
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_VRMS_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	read_tmp = (ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2];
	ade7753_regs[phase].ro_voltage_rms = ade7753_regs[phase].ro_voltage_rms + (read_tmp - ade7753_regs[phase].ro_voltage_rms) / VOLTAGE_FILTER;

	ade7753_regs[phase].ro_vrms_value = vrms_cal[phase].k_slope.f * ade7753_regs[phase].ro_voltage_rms;
}

void read_current_rms(uint8_t phase)
{
	uint32_t read_tmp;
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_IRMS_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	read_tmp = (ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2];
	ade7753_regs[phase].ro_current_rms = ade7753_regs[phase].ro_current_rms + (read_tmp - ade7753_regs[phase].ro_current_rms) / CURRENT_FILTER;
}

void read_current_peak(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_IPEAK_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].ro_current_peak = (ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2];
}

//phase A interrupt
void EXTI1_IRQHandler(void)
{
	if(EXTI->PR & EXTI_Line1)	//line1 interrupt source
	{    
		EXTI->PR = EXTI_Line1;	//clear the interrupt flag on line1
		
		read_reset_status(PHASE_A);
		if(ade7753_regs[PHASE_A].ro_interrupt_status.flag.wsmp == 1)
		{
			read_ade7753_waveform(PHASE_A);
//			if(ade7753_regs[PHASE_A].rw_ade7753_mode.mode.wavsel == 2)		// waveform direct to channel 1 -- current
//			{
//				
//			}
//			else if(ade7753_regs[PHASE_A].rw_ade7753_mode.mode.wavsel == 3)	// waveform_direct to channel 2 -- voltage
//			{
//				
//			}
		}
		
//		if(ade7753_regs[PHASE_A].ro_interrupt_status.flag.zx == 1)
//		{
//			read_voltage_rms(PHASE_A);
//			read_current_rms(PHASE_A);
//		}
		
		test_counter++;
		
//		read_reset_status(PHASE_A);
	}
}

//phase B or C interrupt
void EXTI9_5_IRQHandler(void)
{
	if(EXTI->PR & EXTI_Line5)	//是5线的中断
	{    
		EXTI->PR = EXTI_Line5;	//清除LINE5上的中断标志位
	}
	
	if(EXTI->PR & EXTI_Line9)	//是9线的中断
	{    
		EXTI->PR = EXTI_Line9;	//清除LINE9上的中断标志位
	}
}

void EXTI3_IRQHandler(void)
{
	if(EXTI->PR & EXTI_Line3)	//是3线的中断
	{    
		EXTI->PR = EXTI_Line3;	//清除LINE3上的中断标志位
		
		read_voltage_rms(PHASE_A);
		read_current_rms(PHASE_A);
	}
}

uint32_t poll_vrms(uint8_t phase)
{
	uint32_t read_tmp;
	uint32_t lastupdate;
	
	read_reset_status(phase);
	lastupdate = xTaskGetTickCount();
	do
	{
		if((xTaskGetTickCount() - lastupdate) > 100)
		{
			return 0;
//			break;
		}
		
		read_status(phase);
	} while(ade7753_regs[phase].ro_interrupt_status.flag.zx == 0);
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_VRMS_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	read_tmp = (ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2];
	
	return read_tmp;
}

void get_vrms(uint8_t phase)
{
	uint32_t checksum = 0;
	uint32_t tmp;
	uint8_t ctr = 0;
	
	while(1)
	{
		tmp = poll_vrms(phase);
		if(tmp)
		{
			checksum += tmp;
			ctr++;
		}
		
		if(ctr >= VOLTAGE_FILTER)
		{
			break;
		}
	}
	
	ade7753_regs[phase].ro_voltage_rms = checksum / VOLTAGE_FILTER;
	
	ade7753_regs[phase].ro_vrms_value = vrms_cal[phase].k_slope.f * ade7753_regs[phase].ro_voltage_rms;
}

uint32_t poll_irms(uint8_t phase)
{
	uint32_t read_tmp;
	uint32_t lastupdate;
	
	read_reset_status(phase);
	lastupdate = xTaskGetTickCount();
	do
	{
		if((xTaskGetTickCount() - lastupdate) > 100)
		{
			return 0;
//			break;
		}
		
		read_status(phase);
	} while(ade7753_regs[phase].ro_interrupt_status.flag.zx == 0);
	
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_IRMS_RO, ade7753_rd_buf, 3);
	ade7753_cs_control(phase, SET);
	
	read_tmp = (ade7753_rd_buf[0] << 16) | (ade7753_rd_buf[1] << 8) | ade7753_rd_buf[2];
	
	return read_tmp;
}

void get_irms(uint8_t phase)
{
	uint32_t checksum = 0;
	uint32_t tmp;
	uint8_t ctr = 0;
	
	while(1)
	{
		tmp = poll_irms(phase);
		if(tmp)
		{
			checksum += tmp;
			ctr++;
		}
		
		if(ctr >= CURRENT_FILTER)
		{
			break;
		}
	}
	
	ade7753_regs[phase].ro_current_rms = checksum / CURRENT_FILTER;
	
	ade7753_regs[phase].ro_irms_value = irms_cal[phase].k_slope.f * ade7753_regs[phase].ro_current_rms;
}

void read_ade7753_wgain(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_WGAIN_RW, ade7753_rd_buf, 2);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_power_gain = (ade7753_rd_buf[0] << 8) | ade7753_rd_buf[1];
}

void write_ade7753_wgain(uint8_t phase)
{
	ade7753_wr_buf[0] = ade7753_regs[phase].rw_power_gain >> 8;
	ade7753_wr_buf[1] = ade7753_regs[phase].rw_power_gain & 0xff;
	
	ade7753_cs_control(phase, CLR);
	write_ade7753(ADE7753_REG_WGAIN_RW, ade7753_wr_buf, 2);
	ade7753_cs_control(phase, SET);
}

void read_ade7753_wdiv(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_WDIV_RW, ade7753_rd_buf, 1);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_active_energy_divider = ade7753_rd_buf[0];
}

void write_ade7753_wdiv(uint8_t phase)
{
	ade7753_wr_buf[0] = ade7753_regs[phase].rw_active_energy_divider;
	
	ade7753_cs_control(phase, CLR);
	write_ade7753(ADE7753_REG_WDIV_RW, ade7753_wr_buf, 1);
	ade7753_cs_control(phase, SET);
}

void read_ade7753_apos(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_APOS_RW, ade7753_rd_buf, 2);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_active_power_offset = (ade7753_rd_buf[0] << 8) | ade7753_rd_buf[1];
}

void write_ade7753_apos(uint8_t phase)
{
	ade7753_wr_buf[0] = ade7753_regs[phase].rw_active_power_offset >> 8;
	ade7753_wr_buf[1] = ade7753_regs[phase].rw_active_power_offset & 0xff;
	
	ade7753_cs_control(phase, CLR);
	write_ade7753(ADE7753_REG_APOS_RW, ade7753_wr_buf, 2);
	ade7753_cs_control(phase, SET);
}

void read_ade7753_vagain(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_VAGAIN_RW, ade7753_rd_buf, 2);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_apparent_gain = (ade7753_rd_buf[0] << 8) | ade7753_rd_buf[1];
}

void write_ade7753_vagain(uint8_t phase)
{
	ade7753_wr_buf[0] = ade7753_regs[phase].rw_apparent_gain >> 8;
	ade7753_wr_buf[1] = ade7753_regs[phase].rw_apparent_gain & 0xff;
	
	ade7753_cs_control(phase, CLR);
	write_ade7753(ADE7753_REG_VAGAIN_RW, ade7753_wr_buf, 2);
	ade7753_cs_control(phase, SET);
}

void read_ade7753_vadiv(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_VADIV_RW, ade7753_rd_buf, 1);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_apparent_energy_divider = ade7753_rd_buf[0];
}

void write_ade7753_vadiv(uint8_t phase)
{
	ade7753_wr_buf[0] = ade7753_regs[phase].rw_apparent_energy_divider;
	
	ade7753_cs_control(phase, CLR);
	write_ade7753(ADE7753_REG_VADIV_RW, ade7753_wr_buf, 1);
	ade7753_cs_control(phase, SET);
}

void read_ade7753_phcal(uint8_t phase)
{
	ade7753_cs_control(phase, CLR);
	read_ade7753(ADE7753_REG_PHCAL_RW, ade7753_rd_buf, 1);
	ade7753_cs_control(phase, SET);
	
	ade7753_regs[phase].rw_phase_calibration = ade7753_rd_buf[0];
}

void write_ade7753_phcal(uint8_t phase)
{
	ade7753_wr_buf[0] = ade7753_regs[phase].rw_phase_calibration;
	
	ade7753_cs_control(phase, CLR);
	write_ade7753(ADE7753_REG_PHCAL_RW, ade7753_wr_buf, 1);
	ade7753_cs_control(phase, SET);
}

void ade7753_energy_setup(uint8_t phase, uint16_t wgain, uint8_t wdiv, int16_t apos, int16_t vagain, uint8_t vadiv, uint8_t phcal)
{
	ade7753_regs[phase].rw_power_gain = wgain;
	write_ade7753_wgain(phase);
	
	ade7753_regs[phase].rw_active_energy_divider = wdiv;
	write_ade7753_wdiv(phase);
	
	ade7753_regs[phase].rw_active_power_offset = apos;
	write_ade7753_apos(phase);
	
	ade7753_regs[phase].rw_apparent_gain = vagain;
	write_ade7753_vagain(phase);
	
	ade7753_regs[phase].rw_apparent_energy_divider = vadiv;
	write_ade7753_vadiv(phase);
	
	ade7753_regs[phase].rw_phase_calibration = phcal;
	write_ade7753_phcal(phase);
}

/**************************************************\
Calibrations:
1. Watt-hour calibration
	a. Signal path and functionality
	b. Gain calibration
	c. Offset calibration
	d. Phase calibration
2. RMS calibration
	a. Signal path and functionality
	b. Offset calibration
3. VA-hour calibration
	a. Signal path and functionality
	b. Gain calibration
4. Reactive energy
	a. Theory of operation
	b. ADE7753 implementation
\**************************************************/

/***************************************\
Watt-hour gain calibration
\***************************************/
_STR_WATT_HOUR_CALIBRATION wh_calibration[3];

typedef struct 
{
	int32_t Wh; 		//active energy
	uint32_t VAh;	//apparent energy
} _STR_ENERTY_;
_STR_ENERTY_ energy[3];

_STR_VAH_CALIBRATION vah_calibration[3];

uint8_t do_lincyc(uint8_t phase)
{
	uint32_t lastupdate;
	
	ade7753_regs[phase].rw_ade7753_mode.mode.cycmode = 1;
	write_ade7753_mode_register(phase);
	
	read_reset_status(phase);
	
	lastupdate = xTaskGetTickCount();
	do
	{
		if((xTaskGetTickCount() - lastupdate) > 546000)
		{
			return 0;
//			break;
		}
		
		read_status(phase);
		
	} while(ade7753_regs[phase].ro_interrupt_status.flag.cycend == 0);
	
	read_reset_status(phase);
	
	lastupdate = xTaskGetTickCount();
	do
	{
		if((xTaskGetTickCount() - lastupdate) > 546000)
		{
			return 0;
//			break;
		}
		
		read_status(phase);
		
	} while(ade7753_regs[phase].ro_interrupt_status.flag.cycend == 0);
	
	energy[phase].Wh = read_ade7753_line_accumulation_active_energy(phase);
	energy[phase].VAh = read_ade7753_line_accumulation_apparent_energy(phase);
	
	return 1;
}

uint8_t watt_hour_calibration_cf(uint8_t phase)
{
	float tmp;
	float cf_caculation;
	
	read_line_cycle(phase); //////////////////////////////
	
	if(do_lincyc(phase) == 0) return 0;
	
	wh_calibration[phase].cf_expected.f = wh_calibration[phase].cf_imp_per_kwh * wh_calibration[phase].Vtest * wh_calibration[phase].Itest / 1000 / 3600;
	tmp = 1.0 * ADE7753_CLOCK / ade7753_regs[phase].rw_line_cycle / ade7753_regs[phase].ro_voltage_period / 4;
	cf_caculation = energy[phase].Wh * tmp;
//	cf_caculation = 1.0 * laenergy / ade7753_regs[phase].rw_line_cycle / ade7753_regs[phase].ro_voltage_period / 4 * ADE7753_CLOCK;
	ade7753_regs[phase].rw_cf_frequency_divider_denominator = (uint16_t)(cf_caculation / wh_calibration[phase].cf_expected.f) - 1;
	
	wh_calibration[phase].Laenergy_test = (int32_t)(wh_calibration[phase].cf_expected.f * tmp * (ade7753_regs[phase].rw_cf_frequency_divider_denominator + 1));
//	ade7753_regs[phase].rw_power_gain = (int16_t)(((wh_calibration[phase].cf_expected / (cf_caculation / ade7753_regs[phase].rw_cf_frequency_divider_denominator)) - 1) * 4096);
	ade7753_regs[phase].rw_power_gain = (int16_t)((wh_calibration[phase].Laenergy_test / energy[phase].Wh - 1) * 4096);
	//write these two parameters to the registers
	write_ade7753_wgain(phase);//???????????????
	
//	wh_calibration[phase].const_w_lsb = wh_calibration[phase].cf_expected * 1000 / wh_calibration[phase].cf_imp_per_kwh / cf_caculation * 4096 / 4094;
	wh_calibration[phase].const_w_lsb.f = 1000.0 / wh_calibration[phase].cf_imp_per_kwh / (ade7753_regs[phase].rw_cf_frequency_divider_denominator + 1);
	//write it to eeprom ???????????????????
	
//	vah_lsb = VA * Accumulation_times / 3600 / LVAENEGRY
//			= VA * LINCYC * VOL_PERIOD * 4 / 3600 / CLKIN / LVAENERGY
//			= VA * LINCYC * VOL_PERIOD / 900 / CLKIN / LVAENERGY
//	vah_calibration[phase].const_vah_lsb = 1.0 * wh_calibration[phase].Vtest * wh_calibration[phase].Itest / energy[phase].VAh
//										   * ade7753_regs[phase].ro_voltage_period * wh_calibration[phase].LineCyc_ib
//										   / 900 / ADE7753_CLOCK;
	
	
	return 1;
}

// Iselect = 0: read the Laenergy for the Itest
// Iselect = 1: read the Laenergy for the Icorrection
uint8_t watt_hour_calibration_offset_pro(uint8_t phase, uint8_t Iselect)
{
	int32_t laenergy;
	
	laenergy = read_ade7753_line_accumulation_active_energy(phase);
	if(laenergy == 0) return 0;
	
	if(Iselect == 0)
	{
		wh_calibration[phase].Laenergy_test = laenergy;
	}
	else
	{
		float accumulation_time;
		float cf_constant;
		
		wh_calibration[phase].Laenergy_correction = laenergy;
	
		accumulation_time = 8.0 / 2 / ade7753_regs[phase].ro_voltage_period * ade7753_regs[phase].rw_line_cycle;
		cf_constant = 1.0 * (ade7753_regs[phase].rw_cf_frequency_divider_numerator + 1) 
						  * ade7753_regs[phase].rw_active_energy_divider
						  * (4096 + ade7753_regs[phase].rw_power_gain)
						  / (ade7753_regs[phase].rw_cf_frequency_divider_denominator + 1)
						  / 4096;
		
		wh_calibration[phase].lsb_variation.f = wh_calibration[phase].Laenergy_correction * cf_constant / accumulation_time;
	}
	
	return 1;
}

void watt_hour_calibration_offset(uint8_t phase)
{
	int32_t laenergy_correction_expected;
	int32_t laenergy_absolute_error;
	
	laenergy_correction_expected = (int32_t)(1.0 * wh_calibration[phase].Laenergy_test * wh_calibration[phase].I_correction.f
									/ wh_calibration[phase].Itest * wh_calibration[phase].LineCyc_imin / wh_calibration[phase].LineCyc_ib);

	laenergy_absolute_error = wh_calibration[phase].Laenergy_correction - laenergy_correction_expected;
	
//	
//	APOS = - error_rate * 2^35 / CLKIN
//		 = - (laenergy_absolute_error * CLKIN / LINCYC / PERIOD / 4) * 2^35 / CLKCIN
//		 = - (laenergy_absolute_error / LINCYC / PERIOD * 2^33)
	ade7753_regs[phase].rw_active_power_offset = - (int16_t)(1.0 * laenergy_absolute_error * (2^16) / wh_calibration[phase].LineCyc_imin 
													/ ade7753_regs[phase].ro_voltage_period * (2^17));
	write_ade7753_apos(phase);
}

//校准相位前，应先校准源的输出功率因数调节到0.5（或者其他，函数里代码按照0.5操作的）
void watt_hour_calibration_phase(uint8_t phase)
{
	int32_t laenergy;
	double energy_error, phase_error;
	
	laenergy = read_ade7753_line_accumulation_active_energy(phase);
	
	energy_error = ((double)laenergy * 2 - wh_calibration[phase].Laenergy_test) / wh_calibration[phase].Laenergy_test;
	
	phase_error = - asin(energy_error / sqrt(3));
	
	ade7753_regs[phase].rw_phase_calibration = (int8_t)(phase_error * ade7753_regs[phase].ro_voltage_period / 360) + 0x0D;
	
	write_phase_calibration(phase);
}

/**************************************************************\
VRMSOS = (V1*Vrms2 - V2*Vrms1) / (V2 - V1);
\**************************************************************/
struct _VRMS_OS_COMP_ 
{
	uint8_t enable_compensation;
	uint16_t vrms_v1;
	uint32_t vrms_adc1;
	uint16_t vrms_v2;
	uint32_t vrms_adc2;
} vrms_offset[3];

void calculate_vrms_offset(uint8_t phase)
{
	int64_t dtmp;
	int16_t itmp;
	
	if(vrms_offset[phase].enable_compensation)
	{
		vrms_offset[phase].enable_compensation = 0;
		
		itmp = vrms_offset[phase].vrms_v2 - vrms_offset[phase].vrms_v1;
		dtmp = (int64_t)vrms_offset[phase].vrms_v1 * vrms_offset[phase].vrms_adc2 - (int64_t)vrms_offset[phase].vrms_v2 * vrms_offset[phase].vrms_adc1;
		ade7753_regs[phase].rw_voltage_rms_offset = dtmp / itmp;
		
		ade7753_wr_buf[0] = ade7753_regs[phase].rw_voltage_rms_offset >> 8;
		ade7753_wr_buf[1] = ade7753_regs[phase].rw_voltage_rms_offset & 0xff;
		
		ade7753_cs_control(phase, CLR);
		write_ade7753(ADE7753_REG_VRMSOS_RW, ade7753_wr_buf, 2);
		ade7753_cs_control(phase, SET);
	}
}

/*******************************************************************\
IRMSOS = (I1^2 * Irms2^2 - I2^2 * Irms1^2) / (I2^2 - I1^2) / 32768;
	   = [(I1*Irms2 + I2*Irms1) * (I1*Irms2 - I2*Irms1)] / [(I2 + I1) * (I2 - I1)] / 32768;
\*******************************************************************/
struct _IRMS_OS_COMP_ 
{
	uint8_t enable_compensation;
	uint16_t irms_i1;
	uint32_t irms_adc1;
	uint16_t irms_i2;
	uint32_t irms_adc2;
} irms_offset[3];

void calculate_irms_offset(uint8_t phase)
{
	int64_t dtmp1, dtmp2;
	int32_t ltmp1, ltmp2;
	
	if(irms_offset[phase].enable_compensation)
	{
		// caculation.....
		dtmp1 = irms_offset[phase].irms_i1 * irms_offset[phase].irms_adc2;
		dtmp2 = irms_offset[phase].irms_i2 * irms_offset[phase].irms_adc1;
		ltmp1 = (dtmp1 + dtmp2) * (dtmp1 - dtmp2);
		
		dtmp1 = irms_offset[phase].irms_i2;
		dtmp2 = irms_offset[phase].irms_i1;
		ltmp2 = (dtmp1 + dtmp2) * (dtmp1 - dtmp2);
		
		ade7753_regs[phase].rw_current_rms_offset = (ltmp1 / ltmp2) >> 15;
		
		ade7753_wr_buf[0] = ade7753_regs[phase].rw_current_rms_offset >> 8;
		ade7753_wr_buf[1] = ade7753_regs[phase].rw_current_rms_offset & 0xff;
		
		ade7753_cs_control(phase, CLR);
		write_ade7753(ADE7753_REG_IRMSOS_RW, ade7753_wr_buf, 2);
		ade7753_cs_control(phase, SET);
	}
}


