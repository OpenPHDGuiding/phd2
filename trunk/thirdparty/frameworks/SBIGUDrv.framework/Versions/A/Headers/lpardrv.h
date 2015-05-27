/*

lpardrv.h - Local options for pardrv.h

*/
#ifndef _LPARDRV_
#define _LPARDRV_

#define ENV_WIN				1			/* Target for Windows environment */
#define ENV_WINVXD		2			/* SBIG Use Only, Win 9X VXD */
#define ENV_WINSYS		3			/* SBIG Use Only, Win NT SYS */
#define ENV_ESRVJK		4			/* SBIG Use Only, Ethernet Remote */ 
#define ENV_ESRVWIN		5			/* SBIG Use Only, Ethernet Remote */
#define ENV_MACOSX		6			/* SBIG Use Only, Mac OSX */
#define ENV_LINUX			7			/* SBIG Use Only, Linux */
#define ENV_NIOS	    8			/* SBIG Use Only, Embedded NIOS */ 

#ifndef TARGET
#define TARGET			ENV_MACOSX		/* Set for your target */
#endif

#if TARGET == ENV_MACOSX

	#include <SBIGUDrv/sbigudrv.h>
	#ifdef _DEBUG
	#define _DEBUG		1
	#endif

#else

	#include "sbigudrv.h"

#endif

#if TARGET == ENV_LINUX

	#include <libusb.h>

	#ifdef _DEBUG
	#define _DEBUG 1
	#endif

#endif

#endif
