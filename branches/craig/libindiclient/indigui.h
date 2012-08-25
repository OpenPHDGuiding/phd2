#ifndef _INDIGUI_H_
#define _INDIGUI_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void indigui_make_device_page(struct indi_device_t *idev);
extern void indigui_update_widget(struct indi_prop_t *iprop);
extern void indigui_add_prop(struct indi_device_t *idev, const char *groupname, struct indi_prop_t *iprop);
extern void indigui_delete_prop(struct indi_prop_t *iprop);
extern void *indigui_create_window(struct indi_t *indi);
extern void indigui_show_dialog(void *data);
extern void indigui_show_message(struct indi_t *indi, const char *message);
#ifdef __cplusplus
}
#endif
#endif //_INDIGUI_H_
