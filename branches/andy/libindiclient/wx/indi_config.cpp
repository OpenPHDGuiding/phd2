#include "wx/config.h"

#include "../indi.h"
#include "../indi_config.h"

class IndiConfig {
public:
	IndiConfig(struct indi_t *_indi, const char *cfg);
	~IndiConfig();
	IndiConfig();
	bool LoadINI(const char *cfg);
	void SetDefault(indi_prop_t *iprop, bool define = false);
	void UpdateProps();

private:
	void SendElems(indi_prop_t *iprop);

	struct indi_t *indi;
	wxConfig *config;
	bool connected;

};

void *ic_init(struct indi_t *indi, const char *config)
{
	if (! config)
		return NULL;
	return new IndiConfig(indi, config);
}

void ic_prop_set(void *c, struct indi_prop_t *iprop)
{
	IndiConfig *cfg = (IndiConfig *)c;
	if (! c)
		return;
	cfg->SetDefault(iprop);
}

void ic_prop_def(void *c, struct indi_prop_t *iprop)
{
	IndiConfig *cfg = (IndiConfig *)c;
	if (! c)
		return;
	cfg->SetDefault(iprop, true);
}


void ic_update_props(void *c)
{
	IndiConfig *cfg = (IndiConfig *)c;
	if (! c)
		return;
	cfg->UpdateProps();
}

void IndiConfig::SetDefault(indi_prop_t *iprop, bool define)
{
	indi_list *isl;

	if (iprop->state == INDI_RO)
		return;
	if (connected) {
		if (strcmp(iprop->name, "CONNECTION") == 0 && ! indi_prop_get_switch(iprop, "CONNECT")) {
			connected = false;
			return;
		}
		if (define)
			SendElems(iprop);
	} else {
		if (strcmp(iprop->name, "CONNECTION") != 0 || ! indi_prop_get_switch(iprop, "CONNECT")) {
			return;
		}
		connected = true;
		for (isl = il_iter(iprop->idev->props); ! il_is_last(isl); isl = il_next(isl)) {
			struct indi_prop_t *prop = (struct indi_prop_t *)il_item(isl);
			SendElems(prop);
		}
	}
}
			
void IndiConfig::SendElems(indi_prop_t *iprop)
{
	indi_list *isl;
	wxString basekey, key, value;
	long i = 0;
	basekey =  wxString::FromAscii(iprop->idev->name) + _T("/") + wxString::FromAscii(iprop->name) + _T("/");
	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_elem_t *ielem = (struct indi_elem_t *)il_item(isl);
		key = basekey + wxString::FromAscii(ielem->name);
		if (config->Read(key, &value)) {
			// Found a match...
			switch(iprop->type) {
			case INDI_PROP_TEXT:
				strncpy(ielem->value.str, value.mb_str(wxConvUTF8), sizeof(ielem->value.str));
				break;
			case INDI_PROP_NUMBER:
				value.ToDouble(&ielem->value.num.value);
				break;
			case INDI_PROP_SWITCH:
				value.ToLong(&i);
				ielem->value.set = i;
				break;
			}
			indi_send(iprop, ielem);
		}
	}
}

void IndiConfig::UpdateProps()
{
	wxString basekey, key;
	indi_list *isl_d, *isl_p, *isl_e;
	for (isl_d = il_iter(indi->devices); ! il_is_last(isl_d); isl_d = il_next(isl_d)) {
		struct indi_device_t *dev = (struct indi_device_t *)il_item(isl_d);
		for (isl_p = il_iter(dev->props); ! il_is_last(isl_p); isl_p = il_next(isl_p)) {
			struct indi_prop_t *iprop = (struct indi_prop_t *)il_item(isl_p);
			basekey =  wxString::FromAscii(dev->name) + _T("/") + wxString::FromAscii(iprop->name);
			config->DeleteGroup(key);
			if (! iprop->save)
				continue;
			for (isl_e = il_iter(iprop->elems); ! il_is_last(isl_e); isl_e = il_next(isl_e)) {
				struct indi_elem_t *ielem = (struct indi_elem_t *)il_item(isl_e);
				key =  basekey + _T("/") + wxString::FromAscii(ielem->name);

				switch(iprop->type) {
				case INDI_PROP_TEXT:
					config->Write(key, wxString::FromAscii(ielem->value.str));
					break;
				case INDI_PROP_NUMBER:
					config->Write(key, wxString::Format(_T("%f"),ielem->value.num.value));
					break;
				case INDI_PROP_SWITCH:
					if(ielem->value.set)
						config->Write(key, _T("1"));
					break;
				}
			}
		}
	}
	config->Flush();
}

IndiConfig::IndiConfig(struct indi_t *_indi, const char *cfg)
{
	indi = _indi;
	config = new wxConfig(wxString::FromAscii(cfg));
}

IndiConfig::~IndiConfig()
{
	delete config;
}
