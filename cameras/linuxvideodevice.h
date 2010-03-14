#include "v4lcontrol.h"

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>		/* getopt_long() */
#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>		/* for videodev2.h */
#include <linux/videodev2.h>
#define WITH_V4L2_LIB 1		/* v4l library */
#ifdef WITH_V4L2_LIB
#include <libv4lconvert.h>
#endif

struct buffer {
	void *start;
	size_t length;
};

#define IO_METHOD_READ 7	/* !! must be != V4L2_MEMORY_MMAP / USERPTR */
static struct v4l2_format fmt;		/* gtk pormat */
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define NFRAMES 30

#define FRAMEWIDTH 640
#define FRAMEHEIGHT 480

/*--------------------------------------------------------------------------------------*/

class linuxvideodevice {

public:
	linuxvideodevice(const char*);
	int openvideodevice(int*, int*);
	void shutdownvideodevice(void);
	int get_frame(int);
	unsigned char getPixel(int);
	int getFrameDims(short);

	int get_fd() { return fd; }

	void queryV4LControls(V4LControlMap &controlMap);

private:
	char *devicename;
	int io;
	int fd;
	struct buffer *buffers;
	int n_buffers;
	int frameWidth;
	int frameHeight;
	int expDuration;
	unsigned int simg[16581184];
	unsigned char img[16581184];

#ifdef WITH_V4L2_LIB
	struct v4lconvert_data *v4lconvert_data;
	struct v4l2_format src_fmt;	/* raw format */
	unsigned char *dst_buf;
#endif

	void init_device(int*, int*);
	void start_capturing(void);
	void stop_capturing(void);
	void uninit_device(void);
	void close_device(void);
	void errno_exit(const char*);
	int xioctl(int, int, void*);
	void process_image(unsigned char*, int);
	int read_frame(void);
	void init_read(unsigned int);
	void init_mmap(void);
	void init_userp(unsigned int);

	void addControl(V4LControlMap &controlMap, struct v4l2_queryctrl &ctrl);
};

/*------------------------------------------------------------*/
