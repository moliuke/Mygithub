#include "mtime.h"
#include "config.h"
#include "debug.h"

int set_sys_time(uint8_t *timestr,uint8_t len)
{
    struct tm _tm;  
    struct timeval tv;  
    time_t timep;

	debug_printf("timestr = %s\n",timestr);
    sscanf(timestr, "%4d/%02d/%02d-%02d:%02d:%02d", &_tm.tm_year,  
        &_tm.tm_mon, &_tm.tm_mday,&_tm.tm_hour,  
        &_tm.tm_min, &_tm.tm_sec); 

	debug_printf("_tm.tm_year = %d,_tm.tm_mon = %d,_tm.tm_mday = %d\n_tm.tm_hour = %d,_tm.tm_min = %d,_tm.tm_sec = %d\n",
		_tm.tm_year,_tm.tm_mon,_tm.tm_mday,_tm.tm_hour,_tm.tm_min,_tm.tm_sec);
	
    _tm.tm_mon = _tm.tm_mon - 1;  
    _tm.tm_year = _tm.tm_year - 1900;  
  
    timep = mktime(&_tm);  
    tv.tv_sec = timep;  
    tv.tv_usec = 0;
	debug_printf("tv.tv_sec = %d,tv.tv_usec = %d\n",(int)tv.tv_sec,(int)tv.tv_usec);
	
    if(settimeofday (&tv, (struct timezone *) 0) < 0)  
    {  
    	DEBUG_PRINTF;
	    debug_printf("Set system datatime error!/n");  
	    return -1;  
    }  
	return 0;
}

int get_sys_time(uint8_t *timestr,uint8_t *len)
{
	time_t timer;	
	struct tm* t_tm; 
	
	time(&timer);	
	t_tm = localtime(&timer);
	
	debug_printf("today is %4d%02d%02d%02d%02d%02d\n", t_tm->tm_year+1900,   
	t_tm->tm_mon+1, t_tm->tm_mday, t_tm->tm_hour, t_tm->tm_min, t_tm->tm_sec);	
	sprintf(timestr,"%4d/%02d/%02d-%02d:%02d:%02d",t_tm->tm_year+1900,
		t_tm->tm_mon+1, t_tm->tm_mday, t_tm->tm_hour, t_tm->tm_min, t_tm->tm_sec);
	debug_printf("timestr = %s\n",timestr);
	*len = strlen(timestr);
	timestr[19] = '\0';
	return 0;
}


#if 0
int main(void)
{

	uint32_t year = 0;
	uint8_t month = 0;
	uint8_t day   = 0;
	uint8_t hour  = 0,min = 0,sec = 0;

	uint8_t timestr[24];
	sprintf(timestr,"%4d/%02d/%02d-%02d:%02d:%02d",2017,12,5,14,25,38);
	
	set_sys_time(timestr,strlen(timestr));
}
#endif

