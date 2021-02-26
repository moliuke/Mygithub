#include <stdio.h>
#include <stdlib.h>
#include "fuser.h"
#include "../include/config.h"
#include "../include/debug.h"


struct list_head fuser_head;
pthread_mutex_t fuser_mutex;


static pthread_once_t init_fuser = PTHREAD_ONCE_INIT; 


void __fuser_init(void)
{
	INIT_LIST_HEAD(&fuser_head);
	pthread_mutex_init(&fuser_mutex,NULL);
}

//一次性初始化全局变量
void fuser_init(void)
{
    pthread_once(&init_fuser,__fuser_init);  
}



Fuser_t *fuser_search(int user_fd)
{
	Fuser_t *fuser;
	struct list_head *pos = NULL;
	debug_printf("1user_fd = %d\n",user_fd);
	//丫的，在这里踩了个巨坑，在调用list_for_each之前一定要先判断链表
	//是否为空，如果链表为空的,那么pos的指向就是user_Head，而user_Head只是一个
	//头结点，没有嵌入到任何结构当中，当调用list_entry去通过user_Head
	//来获取宿主结构的地址时，程序就会挂掉了，在这里跪了许久。
	if(list_empty(&fuser_head))
		return NULL;
	debug_printf("2user_fd = %d\n",user_fd);
	DEBUG_PRINTF;

	list_for_each(pos, &fuser_head)//这里的pos会自动被赋新值
	{
		debug_printf("3user_fd = %d\n",user_fd);
		fuser = list_entry(pos,Fuser_t,list);
		if(fuser == NULL)
			return NULL;
		debug_printf("4user_fd = %d\n",user_fd);
		debug_printf("fuser->fd = %d,user_fd = %d\n",fuser->fd,user_fd);
		if(fuser->fd == user_fd)
			return fuser;
	}
	
	return NULL;
}

void fuser_insert(Fuser_t *new_user)
{
	list_add(&(new_user->list),&fuser_head);
}

void fuser_del(Fuser_t *del_user)
{
	struct list_head *pos = NULL;
	Fuser_t *fuser = NULL;

	if(list_empty(&fuser_head))
		return;

	list_for_each(pos,&fuser_head)
	{
		fuser = list_entry(pos,Fuser_t,list);
		if(fuser == del_user)
		{
			list_del(&(del_user->list));
			free(del_user);
			del_user = NULL;
			return;
		}
	}
}

void fuser_printf(void)
{
	Fuser_t *fuser;
	struct list_head *pos = NULL;

	if(list_empty(&fuser_head))
	{
		debug_printf("the fuser list is empty\n");
		return;
	}

	list_for_each(pos, &fuser_head)//这里的pos会自动被赋新值
	{
		fuser = list_entry(pos,Fuser_t,list);
		if(fuser == NULL)
			return ;
		debug_printf("fuser->ip = %s,fuser->port = %d,fuser->fd = %d,fuser->id = %d\n",fuser->ip,fuser->port,fuser->fd,fuser->id);
	}
}

void fuser_del_d(Fuser_t *del_user)
{
	list_del(&(del_user->list));
	free(del_user);
	del_user = NULL;
}

void fuser_destroy(void)
{
	Fuser_t *fuser;
	struct list_head *pos = NULL;

	if(list_empty(&fuser_head))
		return;

	list_for_each(pos, &fuser_head)//这里的pos会自动被赋新值
	{
		fuser = list_entry(pos,Fuser_t,list);
		if(fuser == NULL)
			return ;

		list_del(&(fuser->list));
		free(fuser);
		fuser = NULL;
	}
}




