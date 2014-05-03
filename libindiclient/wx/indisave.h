#ifndef _INDISAVE_H_
#define _INDISAVE_H_

#include <wx/dialog.h>
#include "wx/sizer.h"
#include "wxchecktreectrl.h"
#include "../indi.h"

class IndiSave : public wxDialog
{
public:
	IndiSave(wxWindow * parent, const wxString & title, struct indi_t *indi);
	~IndiSave();
	void SetSave();
private:
	void FillTree();
	struct indi_t *indi;
	wxCheckTreeCtrl *tree;
	wxBoxSizer *sizer;
	indi_list *props;
};
#endif
