#ifndef _ANALOG_OUTPUT_H_
#define _ANALOG_OUTPUT_H_

#define Relay1_Out	 PDout(15)
#define Relay2_Out	 PDout(14)
#define Relay3_Out	 PDout(13)
#define Relay4_Out	 PDout(12)

void vOutputTask(void *pvParameters);


#endif
