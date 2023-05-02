#ifndef __CONFIG_H__
#define __CONFIG_H__






#define CALLBACK_MODE_SUPPORT		1
#define	IMAGEQUEUE_ORIG_MODE		1


#define version_year                23
#define version_month               4
#define version_day                 24
#define version_subday              13
#define SDK_SVN_REVISION            1

#if defined (_WIN32)
#define CCM6000_SUPPORT             0
#define QHYCCD_OPENCV_SUPPORT
#define WINDOWS_PTHREAD_SUPPORT		1
#define WINPCAP_MODE_SUPPORT		0
#define PCIE_MODE_SUPPORT			1
#define CYUSB_MODE_SUPPORT  		1
#define WINUSB_MODE_SUPPORT  		1
#define LIBUSB_MODE_SUPPORT  		0
#define PCIE_MODE_TEST  			1
#else
#undef  QHYCCD_OPENCV_SUPPORT
#define WINDOWS_PTHREAD_SUPPORT		0
#define WINPCAP_MODE_SUPPORT		0

#if defined(__arm__) || defined (__arm64__) || defined (__aarch64__) || defined(__APPLE__)||defined (__ANDROID__)
#define PCIE_MODE_SUPPORT			0
#elif defined(__i386__) || defined(__x86_64__)
#define PCIE_MODE_SUPPORT			1
#else
#define PCIE_MODE_SUPPORT			0
#endif


#define CYUSB_MODE_SUPPORT  		0
#define WINUSB_MODE_SUPPORT  		0
#define LIBUSB_MODE_SUPPORT  		1
#endif



#endif

