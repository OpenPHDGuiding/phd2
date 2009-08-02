#ifndef _INDIGUI_H_
#define _INDIGUI_H_

extern void indigui_make_device_page(struct indi_device_t *idev);
extern void indigui_update_widget(struct indi_prop_t *iprop);
extern void indigui_add_prop(struct indi_device_t *idev, const char *groupname, struct indi_prop_t *iprop);
extern void *indigui_create_window(void);
extern void indigui_show_dialog(void *data, int modal);

#endif
