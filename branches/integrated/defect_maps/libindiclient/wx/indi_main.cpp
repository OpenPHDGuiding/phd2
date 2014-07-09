#include <stdlib.h>

#include "../indigui.h"
#include "../indi.h"

#include <wx/wx.h>
static struct indi_t *indi;
class MyApp : public wxApp
{
public:
	virtual bool OnInit();
};

IMPLEMENT_APP(MyApp)

static void camera_capture_cb(struct indi_prop_t *iprop, void *data)
{
	FILE *fh;
	char str[80];
	static int img_count;
	struct indi_elem_t *ielem = indi_find_first_elem(iprop);

	sprintf(str, "test%03d.fits", img_count++);
	printf("Writing: %s\n", str);
	fh = fopen(str, "w+");
	fwrite(ielem->value.blob.data, ielem->value.blob.size, 1, fh);
	fclose(fh);
}

static void find_blob_cb(struct indi_prop_t *iprop, void *data)
{
	if (iprop->type == INDI_PROP_BLOB) {
		printf("Found blob\n");
		indi_dev_enable_blob(iprop->idev, 1);
	        indi_prop_add_cb(iprop, (IndiPropCB)camera_capture_cb, NULL);
	}
}

bool MyApp::OnInit()
{
	indi = indi_init("localhost", 7624, "INDI_wx");
	if (! indi)
		return false;
	indi_device_add_cb(indi, "", (IndiDevCB)find_blob_cb, NULL);
	((wxFrame *)(indi->window))->Show();
	SetTopWindow((wxFrame *)(indi->window));
	return true;
}
