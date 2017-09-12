1. 添加io_control_xxx.revx.lib， ud_str.h  controls.h 三个文件
ud_str.h  -- 定义了input，output的结构
control.h -- 在自己的代码里需要添加的函数

extern void Set_Input_Type(uint8_t point);  
extern unsigned int get_input_raw(uint8_t point);
extern void set_output_raw(uint8_t point,unsigned int value);
extern uint8_t get_max_output(void);
extern uint8_t get_max_input(void);
extern void map_extern_output(uint8_t point);
extern unsigned int conver_by_unit_5v(uint8 sample);
extern unsigned int conver_by_unit_10v(uint8 sample);
extern unsigned int conver_by_unit_custable(uint8_t point,uint8 sample);
extern unsigned long get_high_spd_counter(uint8_t point);

关于添加的函数可以， 参考io_example.c

