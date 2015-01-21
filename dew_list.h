#include<stdio.h>
/********8
需要补充删除头节点怎么办
************/
#ifndef DEW_LIST_H_
#define DEW_LIST_H_
#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})
		
typedef struct list_head{
	struct list_head *prev;
	struct list_head *next;
}list_head;

static void init_list_head(list_head  *head)
{
    head->next = head;
	head->prev = head;
}


static void _list_add(struct list_head *new,
                       struct list_head *prev,
                       struct list_head *next)
{


	/*next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
	*/
	new->next = next;
	new->prev = prev;
	prev->next = new;
	next->prev = new;


}


static  void  list_add_tail(struct list_head *new,struct list_head *head)
{
  
  _list_add(new,head->prev,head);
}



static void _list_del(struct list_head *prev,struct list_head *next)
{
    next->prev =prev;
	prev->next = next;

}

static void list_del(struct list_head *entry)
{
    _list_del(entry->prev,entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
	//free(entry);
	//entry->next = NULL;
	//entry->prev = NULL;


}
#endif
