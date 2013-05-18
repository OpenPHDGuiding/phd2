#ifndef _INDI_CONFIG_H_
#define _INDI_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

void *ic_init(struct indi_t *indi, const char *config);
void ic_prop_set(void *c, struct indi_prop_t *iprop);
void ic_prop_def(void *c, struct indi_prop_t *iprop);
void ic_update_props(void *c);

#ifdef __cplusplus
}
#endif
#endif //_INDI_CONFIG_H_
