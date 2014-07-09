#include "indisave.h"

class propList {
public:
	propList(struct indi_prop_t *_iprop, wxTreeItemId _id) { iprop = _iprop; id = _id; };
	struct indi_prop_t *iprop;
	wxTreeItemId id;
};

IndiSave::IndiSave(wxWindow * parent, const wxString & title, struct indi_t *_indi) : 
	wxDialog(parent, wxID_ANY, title, wxDefaultPosition,
	wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	indi = _indi;
	sizer = new wxBoxSizer(wxVERTICAL) ;
	tree = new wxCheckTreeCtrl(this, wxID_ANY);
	FillTree();
	sizer->Add(tree, 1, wxEXPAND | wxALL);
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL));
	SetSizer(sizer);
	sizer->SetSizeHints(this) ;
	sizer->Fit(this) ;
}

IndiSave::~IndiSave()
{
	il_free(props);
}
	
void IndiSave::FillTree()
{
	wxTreeItemId top, dev_tree, id;
	indi_list *isl_d, *isl_p;

	props = NULL;
	top = tree->AddRoot(_T("Devices"));
	for (isl_d = il_iter(indi->devices); ! il_is_last(isl_d); isl_d = il_next(isl_d)) {
		struct indi_device_t *dev = (struct indi_device_t *)il_item(isl_d);
		dev_tree = tree->AppendItem(top, wxString::FromAscii(dev->name));
		for (isl_p = il_iter(dev->props); ! il_is_last(isl_p); isl_p = il_next(isl_p)) {
			struct indi_prop_t *iprop = (struct indi_prop_t *)il_item(isl_p);
			id = tree->AddCheckedItem(dev_tree, wxString::FromAscii(iprop->name), iprop->save != 0);
			props = il_append(props, new propList(iprop, id));
		}
	}
	tree->SetSizeHints(320, 200);
	tree->ExpandAll();
}

void IndiSave::SetSave()
{
	propList *prop;
	while ((prop = (propList *)il_first(props))) {
		props = il_remove_first(props);
		prop->iprop->save = tree->GetData(prop->id)->GetChecked();
		delete prop;
	}
}
