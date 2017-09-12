#ifndef __ADE7753_H__
#define __ADE7753_H__
 
#include "bitmap.h"


#define ADE7753_CLOCK		3579545
#define MAX_CURRENT			50 //Amps

#define ADE7753_REG_WAVEFORM_RO		0x01
#define ADE7753_REG_AENERGY_RO		0x02
#define ADE7753_REG_RAENERGY_RO		0x03
#define ADE7753_REG_LENERGY_RO		0x04
#define ADE7753_REG_VAENERGY_RO		0x05
#define ADE7753_REG_RVAENERGY_RO	0x06
#define ADE7753_REG_LVAENERGY_RO	0x07
#define ADE7753_REG_LVARENERGY_RO	0x08

#define ADE7753_REG_MODE_RW			0x09
	//the second byte to send
	#define MODE_DISHPF			0x01
	#define MODE_DISLPF2		0x02
	#define MODE_DISCF			0x04
	#define MODE_DISSAG			0x08
	#define MODE_ASUSPEND		0x10
	#define MODE_TEMPSEL		0x20
	#define	MODE_SWRST			0x40
	#define MODE_CYCMODE		0x80
	
	//the first byte to send
	#define MODE_DISCH1			0x01
	#define MODE_DISCH2			0x02
	#define MODE_SWAP			0x04
	//waveform register udpate rate
	#define MODE_DTRT_CLK128	0x00	//27.9kSPS	
	#define MODE_DTRT_CLK256	0x08	//14kSPS
	#define MODE_DTRT_CLK512	0x10	//7kSPS
	#define MODE_DTRT_CLK1024	0x18	//3.5kSPS
	//select teh source of the sampled data for waveform register
	#define MODE_WAVE_ACT_POW	0x00	//24 bits active power signal(output of LPF2)
	#define MODE_WAVE_CH1		0x40	//24 bits channel1
	#define MODE_WAVE_CH2		0x60	//24 bits channel2
	#define MODE_POAM			0x80	//only positive active power to be accumulated
	
	
#define ADE7753_REG_IRQEN_RW		0x0A
#define ADE7753_REG_STATUS_RO		0x0B
#define ADE7753_REG_RSTSTATUS_RO	0x0C
	#define FLAG_AEHF			0x01
	#define FLAG_SAG			0x02
	#define FLAG_CYCEND			0x04
	#define FLAG_WSMP			0x08
	#define FLAG_ZX				0x10
	#define FLAG_TEMP			0x20
	#define FLAG_RESET			0x40
	#define FLAG_AEOF			0x80
	
	#define FLAG_PKV			0x01
	#define FLAG_PKI			0x02
	#define FLAG_VAEHF			0x04
	#define FLAG_VAEOF			0x08
	#define FLAG_ZXTO			0x10
	#define FLAG_PPOS			0x20
	#define FLAG_PNEG			0x40
	#define FLAG_RESERVED		0x80
	
#define ADE7753_REG_CH1OS_RW		0x0D
#define ADE7753_REG_CH2OS_RW		0x0E
#define ADE7753_REG_GAIN_RW			0x0F
	#define PGA1_GAIN_1			0x00
	#define PGA1_GAIN_2			0x01
	#define PGA1_GAIN_4			0x02
	#define PGA1_GAIN_8			0x03
	#define PGA1_GAIN_16		0x04
	
	#define AIN1_SCALE_05		(0x00 << 3) //default, because the CT normal is 330mv output
	#define AIN1_SCALE_025		(0x01 << 3)
	#define AIN1_SCALE_0125		(0x02 << 3)
	
	#define PGA2_GAIN_1			(0x00 << 5)
	#define PGA2_GAIN_2			(0x01 << 5)
	#define PGA2_GAIN_4			(0x02 << 5)
	#define PGA2_GAIN_8			(0x03 << 5)
	#define PGA2_GAIN_16		(0x04 << 5)
	
#define ADE7753_REG_PHCAL_RW		0x10
#define ADE7753_REG_APOS_RW			0x11
#define ADE7753_REG_WGAIN_RW		0x12
#define ADE7753_REG_WDIV_RW			0x13
#define ADE7753_REG_CFNUM_RW		0x14
#define ADE7753_REG_CFDEN_RW		0x15

#define ADE7753_REG_IRMS_RO			0x16
#define ADE7753_REG_VRMS_RO			0x17
#define ADE7753_REG_IRMSOS_RW		0x18
#define ADE7753_REG_VRMSOS_RW		0x19
#define ADE7753_REG_VAGAIN_RW		0x1A
#define ADE7753_REG_VADIV_RW		0x1B
#define ADE7753_REG_LINECYC_RW		0x1C
#define ADE7753_REG_ZXTOUT_RW		0x1D
#define ADE7753_REG_SAGCYC_RW		0x1E
#define ADE7753_REG_SAGLVL_RW		0x1F
#define ADE7753_REG_IPKLVL_RW		0x20
#define ADE7753_REG_VPKLVL_RW		0x21
#define ADE7753_REG_IPEAK_RO		0x22
#define ADE7753_REG_RSTIPEAK_RO		0x23
#define ADE7753_REG_VPEAK_RO		0x24
#define ADE7753_REG_RSTVPEAK_RO		0x25
#define ADE7753_REG_TEMP_RO			0x26
#define ADE7753_REG_PERIOD_RO		0x27
//0X28 - 0X3C reserved
#define ADE7753_REG_TMODE_RW		0x3D
#define ADE7753_REG_CHKSUM_RO		0x3E
#define ADE7753_REG_DIEREV_RO		0x3F




#define PHASE_A		0
#define PHASE_B		1
#define PHASE_C		2

#define CLR			0
#define SET			1

#define CHANNEL1	1
#define CHANNEL2	2

typedef union
{
	float f;
	uint32_t l;
} _FLOAT_TO_LONG;

typedef struct
{
	uint16_t value;
	uint32_t adc;
	_FLOAT_TO_LONG k_slope;
} _TYPE_CAL_;

extern _TYPE_CAL_ vrms_cal[3];
extern _TYPE_CAL_ irms_cal[3];


//mode define
typedef struct
{
	unsigned dishpf		:1;
	unsigned dislpf2	:1;
	unsigned discf		:1;
	unsigned dissag		:1;
	unsigned asuspend	:1;
	unsigned tempsel	:1;
	unsigned swrst		:1;
	unsigned cycmode	:1;
	unsigned disch1		:1;
	unsigned disch2		:1;
	unsigned swap		:1;
	unsigned dtrt		:2;
	unsigned wavsel		:2;
	unsigned poam		:1;
} _MODE_BITS_;

typedef union
{
	uint16_t modes;
	_MODE_BITS_ mode;
} _MODE_TYPE_;

//interrupt define
typedef struct
{
	unsigned aehf		:1;
	unsigned sag		:1;
	unsigned cycend		:1;
	unsigned wsmp		:1;
	unsigned zx			:1;
	unsigned temp		:1;
	unsigned reset		:1;
	unsigned aeof		:1;
	unsigned pkv		:1;
	unsigned pki		:1;
	unsigned avehf		:1;
	unsigned vaeof		:1;
	unsigned zxto		:1;
	unsigned ppos		:1;
	unsigned pneg		:1;
	unsigned reserved	:1;
} _INTTERUPT_BITS_;

typedef union
{
	uint16_t flags;
	_INTTERUPT_BITS_ flag;
} _INTERRUPT_TYPE_;


typedef struct
{
	unsigned ch1_pca_gain	:3;
	unsigned ch1_full_scale	:2;
	unsigned ch2_pca_gain	:3;
} _PCA_GAIN_BITS_;

typedef union
{
	uint8_t pca_gain;
	_PCA_GAIN_BITS_ pca_gain_bits;
} _PCA_GAIN_TYPE_;

#pragma pack (1)
typedef struct 
{
	int32_t ro_waveform;
	int32_t ro_active_energy;
	int32_t ro_active_energy_reset;
	int32_t ro_line_accumulation_active_energy;
	uint32_t ro_apparent_energy;
	uint32_t ro_apparent_energy_reset;
	uint32_t ro_line_accumulation_apparent_energy;
	int32_t ro_line_accumulation_reactive_energy;
	
	_MODE_TYPE_ rw_ade7753_mode;
///////////////////////////////////////////
	_INTERRUPT_TYPE_ rw_interrupt_enable;
	_INTERRUPT_TYPE_ ro_interrupt_status;
///////////////////////////////////////////
	int8_t rw_offset_channel1;
	int8_t rw_offset_channel2;
///////////////////////////////////////////
	_PCA_GAIN_TYPE_ rw_pga_gain;
///////////////////////////////////////////
	int8_t rw_phase_calibration;
	int16_t rw_active_power_offset;
	int16_t rw_power_gain;
	uint8_t rw_active_energy_divider;
	uint16_t rw_cf_frequency_divider_numerator;
	uint16_t rw_cf_frequency_divider_denominator;
	uint32_t ro_current_rms;
	uint32_t ro_voltage_rms;
	uint16_t ro_vrms_value;
	uint16_t ro_irms_value;
	int16_t rw_current_rms_offset;
	int16_t rw_voltage_rms_offset;
	int16_t rw_apparent_gain;
	uint8_t rw_apparent_energy_divider;
	uint16_t rw_line_cycle;
	uint16_t rw_zx_timeout;
	uint8_t rw_sag_cycle;
	uint8_t rw_sag_level;
	uint8_t rw_current_peak_level_thershold;
	uint8_t rw_voltage_peak_level_thershold;
	uint32_t ro_current_peak;
	uint32_t ro_current_peak_reset;
	uint32_t ro_voltage_peak;
	uint32_t ro_voltage_peak_reset;
	int8_t ro_temperature;
	uint16_t ro_voltage_period;
	uint8_t rw_test_mode;
	uint8_t ro_checksum;
	uint8_t ro_die_revision;
	
	uint16_t voltage_frequency;
} _SRT_ADE7753_REGS_;
#pragma pack ()

typedef struct
{
	uint8_t cf_calibration_enable;
	uint16_t cf_imp_per_kwh;
	uint16_t Vtest;
	uint16_t Itest;
	_FLOAT_TO_LONG cf_expected;
	_FLOAT_TO_LONG const_w_lsb;
	uint32_t Laenergy_test;
	uint16_t LineCyc_ib;

	uint8_t offset_calibration_pro;
	uint8_t offset_calibration_enable;
	_FLOAT_TO_LONG I_correction;
	uint32_t Laenergy_correction; //reading from LAENERGY register
	_FLOAT_TO_LONG lsb_variation;  //error, it is better to make it less than 0.1%
	uint16_t LineCyc_imin;
} _STR_WATT_HOUR_CALIBRATION;


typedef struct
{
	uint32_t Lvaenergy_expected;
	_FLOAT_TO_LONG const_vah_lsb;
} _STR_VAH_CALIBRATION;


extern _STR_WATT_HOUR_CALIBRATION wh_calibration[3];








extern _SRT_ADE7753_REGS_ ade7753_regs[3];

extern uint8_t ade7753_test;
extern uint16_t test_counter;

void ade7753_init(void);
void read_ade7753_temperature(uint8_t phase);
void read_ade7753_mode_register(uint8_t phase);

void read_current_rms(uint8_t phase);
void read_current_peak(uint8_t phase);

void read_voltage_rms(uint8_t phase);
void read_voltage_peak(uint8_t phase);
void read_voltage_peak_reset(uint8_t phase);
void read_frequency(uint8_t phase);

void read_status(uint8_t phase);
uint8_t set_ade7753_waveform_source(uint8_t phase, uint8_t waveform_source);
void read_ade7753_waveform(uint8_t phase);


void get_vrms(uint8_t phase);
void get_irms(uint8_t phase);

uint8_t watt_hour_calibration_cf(uint8_t phase);

void ade7753_energy_setup(uint8_t phase, uint16_t wgain, uint8_t wdiv, int16_t apos, int16_t vagain, uint8_t vadiv, uint8_t phcal);


//测试：
//1. 当waveform选择了电流通道，验证waveform寄存器的值是否与IRMS寄存器一致
//2. 当waveform选择了电压通道，验证waveform寄存器的值是否与VRMS寄存器一致


#endif
