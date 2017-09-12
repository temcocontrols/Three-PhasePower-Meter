#ifndef  _STORE_H
#define  _STORE_H 

#include "ud_str.h"
#include "define.h"
 

// page 125 0x0803 e800  - 0x0803 efff    2K  OUT
// page 126 0x0803 f000  - 0x0803 f7ff    2K  IN
// page 127 0x0803 f800  - 0x0803 ffff    2K  VAR

// page 250 0x0807 d000  - 0x0807 d7ff    2K  OUT
// page 251 0x0807 d800  - 0x0807 dfff    2K
// page 252 0x0807 e000  - 0x0807 e7ff    2K  IN
// page 253 0x0807 e800  - 0x0807 efff    2K  
// page 254 0x0807 f000  - 0x0807 f7ff    2K  VAR
// page 255 0x0807 f800  - 0x0807 ffff    2K  

//#define OUT_PAGE_FLAG	0x803effe
//#define OUT_PAGE_FLAG	0x803e800
//#define IN_PAGE_FLAG 	0x803f000	
//#define AV_PAGE_FLAG 	0x803f800

#define FLASH_APP1_ADDR		0x08008000 
#define STM_SECTOR_SIZE	2048


#define OUT_PAGE_FLAG	0x807d000
#define IN_PAGE_FLAG 	0x807e000	
#define AV_PAGE_FLAG 	0x807f000  //FLASH_APP1_ADDR + 254*STM_SECTOR_SIZE


#define OUT_PAGE	(OUT_PAGE_FLAG+2)	
#define IN_PAGE		(IN_PAGE_FLAG+2)
#define AV_PAGE		(AV_PAGE_FLAG+2) 
 
enum{
	OUT_TYPE,
	IN_TYPE,
	VAR_TYPE,
	
	MAX_TYPE,
};

extern Str_variable_point var[] ;


void Flash_Write_Mass(void) ;

void mass_flash_init(void) ;


extern uint8_t write_page_en[MAX_TYPE] ;

void reset_to_factory(void);








#endif

