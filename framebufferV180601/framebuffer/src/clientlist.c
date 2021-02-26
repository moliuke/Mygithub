#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "clientlist.h"

#if 0
#include "queue.h"
#include "protocol.h"
#include "mylist.h"
#include "debug.h"
#else
#include "./include/queue.h"
//#include "./include/protocol.h"
#include "./include/common.h"
#include "include/mylist.h"
#include "./include/debug.h"
#endif

struct list_head user_Head;
pthread_mutex_t user_mutex;

void user_init(void)
{
	INIT_LIST_HEAD(&user_Head);
	pthread_mutex_init(&user_mutex,NULL);
}

void user_printf(void)
{
	user_t *user;
	struct list_head *pos = NULL;

	list_for_each(pos, &user_Head)//这里的pos会自动被赋新值
	{
		user = list_entry(pos,user_t,list);
		if(user != NULL)
			debug_printf("++++user->ip = %s,user->port = %d,user->fd = %d\n",user->ip,user->port,user->fd);
	}
}

user_t *user_search(int index_data)
{
	user_t *user;
	struct list_head *pos = NULL;
	//丫的，在这里踩了个巨坑，在调用list_for_each之前一定要先判断链表
	//是否为空，如果链表为空的,那么pos的指向就是user_Head，而user_Head只是一个
	//头结点，没有嵌入到任何结构当中，当调用list_entry去通过user_Head
	//来获取宿主结构的地址时，程序就会挂掉了，在这里跪了许久。
	if(list_empty(&user_Head))
		return NULL;
	list_for_each(pos, &user_Head)//这里的pos会自动被赋新值
	{
		user = list_entry(pos,user_t,list);
		if(user == NULL)
			return NULL;
		debug_printf("*user->ip = %s,user->port = %d,user->fd = %d,user->id = %d\n",user->ip,user->port,user->fd,user->id); 
		if(user->fd == index_data)
			break;
	}
	if(user->fd != index_data)
	{
		debug_printf("user [ %d ] not exist!\n",index_data);
		return NULL;
	}
	return user;
}

int user_getFreeUser(user_t **freeUser,int *cnt)
{
	user_t *user;
	int n = 0;
	struct list_head *pos = NULL;

	if(list_empty(&user_Head))
		return 0;
	
	list_for_each(pos, &user_Head)//这里的pos会自动被赋新值
	{
		user = list_entry(pos,user_t,list);
		if(user == NULL)
			return 0;
		
		if(user->busyState == USER_IS_FREE)
			freeUser[n++] = user;
	}
	
	*cnt = n;
	return n;
}

void user_insert(user_t *new_user)
{
	list_add(&(new_user->list),&user_Head);
}

int user_del_fd(int fd)
{
	struct list_head *pos = NULL;
	user_t *user = NULL;

	if(list_empty(&user_Head))
		return 0;

	list_for_each(pos,&user_Head)
	{
		user = list_entry(pos,user_t,list);
		if(user->fd == fd)
		{
			debug_printf("user [ fd = %d ] will be del~~~~~~~~~~~~~~~~~~~~~~~~~~~!\n",user->fd);
			list_del(&(user->list));
			//pthread_mutex_destroy(&(del_user->mutex));
			free(user);
			return 1;
		}
	}
}

void user_del(user_t *del_user)
{
	struct list_head *pos = NULL;
	user_t *user = NULL;

	if(list_empty(&user_Head))
		return;

	list_for_each(pos,&user_Head)
	{
		user = list_entry(pos,user_t,list);
		if(user == del_user)
		{
			debug_printf("user [ fd = %d ] will be del~~~~~~~~~~~~~~~~~~~~~~~~~~~!\n",user->fd);
			list_del(&(del_user->list));
			//pthread_mutex_destroy(&(del_user->mutex));
			free(del_user);
			return;
		}
	}
}

void user_destroy(void)
{
	user_t *user;
	struct list_head *pos = NULL;

	if(list_empty(&user_Head))
		return;

	list_for_each(pos, &user_Head)//这里的pos会自动被赋新值
	{
		user = list_entry(pos,user_t,list);
		if(user == NULL)
			return ;

		list_del(pos);
		//pthread_mutex_destroy(&(user->mutex));
		free(user);
	}
}



#if 0

int main(void)
{
	user_t *user_s = NULL;
	user_t *user = (user_t *)malloc(sizeof(user_t));

	user_init();
	
	strcpy(user->ip,"192.168.1.110");
	user->fd = 34;

	user_insert(user);

	user_s = user_search(34);

	debug_printf("user_s->ip = %s\n",user_s->ip);

	while(1)
	{
		sleep(1);
	}
	
	return 0;
}

#endif


