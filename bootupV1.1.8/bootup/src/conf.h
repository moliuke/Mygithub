#ifndef __CONF_H
#define __CONF_H
#include "stdio.h"

int conf_file_read(char *profile, char *field, char *KeyName, char *KeyVal );  
int conf_file_write(const char *profile,char *field,char* KeyName,char* KeyVal); 



#endif