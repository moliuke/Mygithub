#ifndef __CLIENTLIST_H
#define __CLIENTLIST_H

#include "config.h"
#include "common.h"


extern struct list_head user_Head;
extern pthread_mutex_t user_mutex;
void user_init(void);

user_t *user_search(int index_data);
void user_insert(user_t *new_user);
void user_del(user_t *del_user);
void user_destroy(void);
void user_printf(void);
int user_del_fd(int fd);
int user_getFreeUser(user_t **freeUser,int *cnt);

#endif

