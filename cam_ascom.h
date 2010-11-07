#ifndef ASCOMCAMDEF
#define ASCOMCAMDEF

#ifdef __WINDOWS__
#import "file:c:\Program Files (x86)\Common Files\ASCOM\Interface\AscomMasterInterfaces.tlb"
#import "file:C:\\Windows\\System32\\ScrRun.dll" \
	no_namespace \
	rename("DeleteFile","DeleteFileItem") \
	rename("MoveFile","MoveFileItem") \
	rename("CopyFile","CopyFileItem") \
	rename("FreeSpace","FreeDriveSpace") \
	rename("Unknown","UnknownDiskType") \
	rename("Folder","DiskFolder")

//
// Creates COM "smart pointer" _com_ptr_t named _ChooserPtr
//
#import "progid:DriverHelper.Chooser" \
	rename("Yield","ASCOMYield") \
	rename("MessageBox","ASCOMMessageBox")

using namespace AscomInterfacesLib;
using namespace DriverHelper;


#endif

class Camera_ASCOMClass : public GuideCamera {
public:
	bool	CaptureFull(int duration, usImage& img, bool recon);	// Captures a full-res shot
	bool	Connect();
	bool	Disconnect();
//	void	InitCapture();

	bool	PulseGuideScope(int direction, int duration);
//	void	ClearGuidePort();
	bool	Color;
	Camera_ASCOMClass(); 
private:
#ifdef __WINDOWS__
	ICameraPtr pCam;
#endif
//	bool GenericCapture(int duration, usImage& img, int xsize, int ysize, int xpos, int ypos);
};
#endif  //ASCOMCAMDEF
