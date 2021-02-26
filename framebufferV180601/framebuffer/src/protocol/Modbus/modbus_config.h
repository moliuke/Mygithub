#ifndef __MODBUS_CONFIG_H
#define __MODBUS_CONFIG_H

#define modbusConfig	sys_dir"/config/modbus.conf"
//#define MODBUS_ASCII	
//#define MODBUS_RTU

#define MODBUS_DEBUG
#ifdef MODBUS_DEBUG
#define MDBS_PARSE_DEBUG
#define MDBS_DISPLAYDEBUG
#define MDBS_PROTOCOL_DEBUG
#define MDBS_TASK_DEBUG
#endif


#define BRIGHT_MODE_AUTO	0
#define BRIGHT_MODE_HAND	1

extern int MSwidth,MSheight;

#endif


