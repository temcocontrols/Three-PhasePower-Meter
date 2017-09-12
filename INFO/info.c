#include "stm32f10x.h"
#include "ProductModel.h"
#include "define.h"

#define FW_VER_HIGH ((SOFTREV >> 8) & 0xff)
#define FW_VER_LOW 	((SOFTREV) & 0xff)
	
const u8 pro_info[20] __attribute__((at(0x08008200))) = {'T','E','M','C','O','P', 'W', 'M', 'E', 'T', 'E', 'R', 0, 0, 0, '5', '0',  FW_VER_LOW, FW_VER_HIGH, 0,};
