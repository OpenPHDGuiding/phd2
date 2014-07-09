#include "../indi_list.h"

#include <cstring>
#include <list>
using namespace std;

class IndiList
{
public:
	list<void *> l;
	list<void *>::iterator iter;
};

void il_free(indi_list *l)
{
	IndiList *il = (IndiList *)l;
	if (il)
		delete il;
}

indi_list *il_iter(indi_list *l)
{
	IndiList *il = (IndiList *)l;
	if (il)
		il->iter = il->l.begin();
	return l;
}

indi_list *il_next(indi_list *l)
{
	IndiList *il = (IndiList *)l;
	if(il)
		il->iter++;
	return l;
}

int il_is_last(indi_list *l)
{
	IndiList *il = (IndiList *)l;
	if (! il)
		return 1;
	return il->iter == il->l.end();
}

indi_list *il_prepend(indi_list *l, void *data)
{
	IndiList *il = (IndiList *)l;
	if (! il)
		il = new IndiList();
	il->l.push_front(data);
	return il;
}

indi_list *il_append(indi_list *l, void *data)
{
	IndiList *il = (IndiList *)l;
	if (! il)
		il = new IndiList();
	il->l.push_back(data);
	return il;
}

indi_list *il_remove(indi_list *l, void *data)
{
	IndiList *il = (IndiList *)l;
	if (il)
		il->l.remove(data);
	return l;
}
indi_list *il_remove_first(indi_list *l)
{
	IndiList *il = (IndiList *)l;
	if (il)
		il->l.pop_front();
	return l;
}

void *il_item(void *l)
{
	IndiList *il = (IndiList *)l;
	if (! il)
		return NULL;
	return *il->iter;
}

void *il_first(void *l)
{
	IndiList *il = (IndiList *)l;
	if (! il)
		return NULL;
	return il->l.front();
}
int il_length(indi_list *l)
{
	IndiList *il = (IndiList *)l;
	if (! il)
		return 0;
	return il->l.size();
}
