#ifndef _INDI_LIST_H_
#define _INDI_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

//#define indi_list void;
typedef void indi_list;

extern indi_list *il_new();
extern void il_free(indi_list *l);

extern indi_list *il_iter(indi_list *l);
extern indi_list *il_next(indi_list *l);
extern indi_list *il_prepend(indi_list *l, void *data);
extern indi_list *il_append(indi_list *l, void *data);
extern indi_list *il_remove(indi_list *l, void *data);
extern indi_list *il_remove_first(indi_list *l);

extern void *il_first(indi_list *l);
extern void *il_item(indi_list *l);

extern int il_is_last(indi_list *l);
extern int il_length(indi_list *l);

#ifdef __cplusplus
}
#endif
#endif //_INDI_LIST_H_
