#ifndef __FUSER_H
#define __FUSER_H

#include "mylist.h"
#include "../include/config.h"


typedef struct __Fuser
{
	int 				type;
	char				ip[16];
	int 				port;
	int 				fd;

	int 				id;

	int 				uartport;

	char				lst_fr_name[128];
	uint16_t			lst_fr_id;
	uint32_t			lst_offset;

	struct list_head 	list;
}Fuser_t;

extern pthread_mutex_t fuser_mutex;


void fuser_init(void);
Fuser_t *fuser_search(int user_fd);
void fuser_insert(Fuser_t *new_user);
void fuser_del(Fuser_t *del_user);
void fuser_del_d(Fuser_t *del_user);
void fuser_destroy(void);
void fuser_printf(void);
#endif

