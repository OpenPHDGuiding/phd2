#ifndef _INDI_H_
#define _INDI_H_

#include <glib.h>

enum INDI_PERMISSIONS {
	INDI_RO,
	INDI_WO,
	INDI_RW,
};

enum INDI_STATES {
	INDI_STATE_IDLE = 0,
	INDI_STATE_OK,
	INDI_STATE_BUSY,
	INDI_STATE_ALERT,
};

enum INDI_PROP_TYPES {
	INDI_PROP_UNKNOWN = 0,
	INDI_PROP_TEXT,
	INDI_PROP_SWITCH,
	INDI_PROP_NUMBER,
	INDI_PROP_LIGHT,
	INDI_PROP_BLOB,
};

enum INDI_RULES {
	INDI_RULE_ONEOFMANY,
	INDI_RULE_ATMOSTONE,
	INDI_RULE_ANYOFMANY,
};

struct indi_prop_t;
struct indi_device_t;
struct indi_t;

struct indi_elem_t {
	struct indi_prop_t *iprop;
	char name[80];
	char label[80];
	union {
		char str[80];
		int set;
		struct {
			double value;
			double min;
			double max;
			double step;
			char fmt[10];
		} num;
		struct {
			char *data;
			char *ptr;
			unsigned int size;
			unsigned int data_size;
			unsigned int compressed;
			const char *orig_data;
			unsigned int orig_size;
			char *tmp_data;
			void *zstrm;
			char fmt[40];
		} blob;
	} value;
};

struct indi_signals_t {
	void *object;
	unsigned long signal;
};

struct indi_prop_t {
	struct indi_device_t *idev;
	void *root;
	char name[80];
	void *widget;
	GSList *elems;
	GSList *signals;
	int permission;
	int state;
	int type;
	int rule;
	void (* prop_update_cb)(struct indi_prop_t *iprop, void *callback_data);
	void *callback_data;
};

struct indi_device_t {
	struct indi_t *indi;
	char name[80];
	unsigned int type;
	unsigned int capabilities;
	GSList *props;
	void *window;
	void (* new_prop_cb)(struct indi_prop_t *iprop, void *callback_data);
	void *callback_data;
};

struct indi_dev_cb_t {
	char devname[80];
	void (* new_prop_cb)(struct indi_prop_t *iprop, void *callback_data);
	void *callback_data;
};


struct indi_t {
	void *xml_parser;
	GIOChannel *fh;
	GSList *devices;
	GSList *dev_cb_list;
	void *window;
};

extern struct indi_device_t *indi_find_device(struct indi_t *indi, const char *dev);
extern struct indi_prop_t *indi_find_prop(struct indi_device_t *idev, const char *name);
extern struct indi_elem_t *indi_find_elem(struct indi_prop_t *iprop, const char *name);
extern struct indi_elem_t *indi_find_first_elem(struct indi_prop_t *iprop);

extern const char *indi_get_string_from_state(int state);

extern void indi_prop_add_signal(struct indi_prop_t *iprop, void *object, unsigned long signal);
extern void indi_prop_set_signals(struct indi_prop_t *iprop, int active);

extern void indi_send(struct indi_prop_t *iprop, struct indi_elem_t *ielem );

extern void indi_prop_add_cb(struct indi_prop_t *iprop,
                      void (* prop_update_cb)(struct indi_prop_t *iprop, void *callback_data),
                      void *callback_data);
extern struct indi_t *indi_init();

extern void indi_device_add_cb(struct indi_t *indi, const char *devname,
             void (* new_prop_cb)(struct indi_prop_t *iprop, void *callback_data),
             void *callback_data);

extern int indi_prop_get_switch(struct indi_prop_t *iprop, const char *elemname);
extern struct indi_elem_t *indi_prop_set_switch(struct indi_prop_t *iprop, const char *elemname, int state);

extern double indi_prop_get_number(struct indi_prop_t *iprop, const char *elemname);
extern struct indi_elem_t *indi_prop_set_number(struct indi_prop_t *iprop, const char *elemname, double value);

extern struct indi_elem_t *indi_prop_set_string(struct indi_prop_t *iprop, const char *elemname, const char *value);

extern struct indi_elem_t *indi_dev_set_string(struct indi_device_t *idev, const char *propname, const char *elemname, const char *value);
extern struct indi_elem_t *indi_dev_set_switch(struct indi_device_t *idev, const char *propname, const char *elemname, int state);
extern void indi_dev_enable_blob(struct indi_device_t *idev, int state);
#endif
