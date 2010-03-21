#include "linuxvideodevice.h"

/*--------------------------------------------------------------------------------------*/

linuxvideodevice::linuxvideodevice(const char *device) {
	if (NULL != device) {
		size_t length = strlen(device);
		if (NULL != (devicename = (char*)malloc(length+1))) {
			strcpy(devicename, device);

			// Default values
			frameWidth = FRAMEWIDTH;
			frameHeight = FRAMEHEIGHT;
			io = V4L2_MEMORY_MMAP;
			fd = -1;
		}
	}
}

/*--------------------------------------------------------------------------------------*/

int linuxvideodevice::getFrameDims(short isHeight) {
	if (isHeight) {
		return(frameHeight);
	} else {
		return(frameWidth);
	}
}

/*--------------------------------------------------------------------------------------*/

unsigned char linuxvideodevice::getPixel(int pos) {
	return img[pos];	
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::shutdownvideodevice(void) {
	stop_capturing();
	uninit_device();
	close_device();
}

/*--------------------------------------------------------------------------------------*/

int linuxvideodevice::xioctl(int fd, int request, void *arg) {
	int r;

	do {
		r = ioctl(fd, request, arg);
	} while (r < 0 && EINTR == errno);
	return r;
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::errno_exit(const char *s) {

	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
#ifdef WITH_V4L2_LIB
	fprintf(stderr, "%s\n", v4lconvert_get_error_message(v4lconvert_data));
#endif
	exit(EXIT_FAILURE);
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::process_image(unsigned char *p, int len) {
	FILE *fp;
	long pixls,pos;
	unsigned char grayval;


#ifdef WITH_V4L2_LIB
	if (v4lconvert_convert(v4lconvert_data, &src_fmt, &fmt, p, len,
				dst_buf, fmt.fmt.pix.sizeimage) < 0) {
		if (errno != EAGAIN)
			errno_exit("v4l_convert");
		return;
	}
	p = dst_buf;
	len = fmt.fmt.pix.sizeimage;
#endif
	pos = 0;
	for (pixls=0; pixls < len; pixls=pixls+3) {
		grayval=(p[pixls]+p[pixls+1]+p[pixls+2])/3;
		img[pos]=grayval;
		pos++;
	}
	return;
}

/*--------------------------------------------------------------------------------------*/

int linuxvideodevice::read_frame(void) {
	struct v4l2_buffer buf;
	int i;

	switch (io) {
	case IO_METHOD_READ:
		i = read(fd, buffers[0].start, buffers[0].length);
		if (i < 0) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				errno_exit("read");
			}
		}
		process_image((unsigned char*)buffers[0].start, i);
		break;

	case V4L2_MEMORY_MMAP:
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (xioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}
		assert(buf.index < (unsigned int)n_buffers);

		process_image((unsigned char*)buffers[buf.index].start, buf.bytesused);

		if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
			errno_exit("VIDIOC_QBUF");
		break;
	case V4L2_MEMORY_USERPTR:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (xioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		for (i = 0; i < n_buffers; ++i)
			if (buf.m.userptr == (unsigned long) buffers[i].start
			    && buf.length == buffers[i].length)
				break;
		assert(i < n_buffers);

		process_image((unsigned char *) buf.m.userptr, buf.bytesused);
		if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
			errno_exit("VIDIOC_QBUF");
		break;
	}
	return 1;
}

/*--------------------------------------------------------------------------------------*/

int linuxvideodevice::get_frame(int etime) {
	fd_set fds;
	struct timeval tv;
	int r,res;
	float timeTaken,chmax;
	clock_t start, end;
	long count,exposures;;
	unsigned int sumGrayVal, imin, imax;

	expDuration = etime;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	r = select(fd + 1, &fds, NULL, NULL, &tv);
	if (r < 0) {
		if (EINTR == errno)
			return 0;

		errno_exit("select");
	}
	if (0 == r) {
		fprintf(stderr, "select timeout\n");
		exit(EXIT_FAILURE);
	}

	start = clock();
	res = read_frame();
	exposures = 1;
	end = clock();
	if (etime > 50) {
		timeTaken=(end-start)/((float)CLOCKS_PER_SEC)*1000;
		if (timeTaken < etime) {
			for (count = 0; count < frameWidth*frameHeight; count++) {
				simg[count]=img[count];
			}
			do {
				res = read_frame();
				end = clock();
				exposures++;
				timeTaken=(end-start)/((float)CLOCKS_PER_SEC)*1000;
				for (count = 0; count < frameWidth*frameHeight; count++) {
					simg[count]+=img[count];
				}
			} while (timeTaken < etime);
			imin = 60000;
			imax = 0;
			for (count = 0; count < frameWidth*frameHeight; count++) {
				if (simg[count] < imin) {
					imin = simg[count];
				}
				if (simg[count] > imax) {
					imax = simg[count];
				}
			}
			for (count = 0; count < frameWidth*frameHeight; count++) {
				sumGrayVal = (simg[count]-imin)/((float)imax)*255;
				img[count]=(unsigned char)sumGrayVal;
			}
		}
	}
	return res;
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::stop_capturing(void) {
	enum v4l2_buf_type type;

	// Get rid of the controls first
	controlMap.clear();

	switch (io) {
	case IO_METHOD_READ:
		break;
	case V4L2_MEMORY_MMAP:
	case V4L2_MEMORY_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (xioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
			errno_exit("VIDIOC_STREAMOFF");
		break;
	}
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::start_capturing(void) {
	int i;
	enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		printf("read method\n");
		break;
	case V4L2_MEMORY_MMAP:
		printf("mmap method\n");
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
				errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (xioctl(fd, VIDIOC_STREAMON, &type) < 0)
			errno_exit("VIDIOC_STREAMON");
		break;
	case V4L2_MEMORY_USERPTR:
		printf("userptr method\n");
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long) buffers[i].start;
			buf.length = buffers[i].length;

			if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
				errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (xioctl(fd, VIDIOC_STREAMON, &type) < 0)
			errno_exit("VIDIOC_STREAMON");
		break;
	}
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::uninit_device(void) {
	int i;

	switch (io) {
	case IO_METHOD_READ:
		free(buffers[0].start);
		break;
	case V4L2_MEMORY_MMAP:
		for (i = 0; i < n_buffers; ++i)
			if (-1 ==
			    munmap(buffers[i].start, buffers[i].length))
				errno_exit("munmap");
		break;
	case V4L2_MEMORY_USERPTR:
		for (i = 0; i < n_buffers; ++i)
			free(buffers[i].start);
		break;
	}

	free(buffers);
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::init_read(unsigned int buffer_size) {
	buffers = (buffer*)(calloc(1, sizeof(*buffers)));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);

	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::init_mmap(void) {
	struct v4l2_requestbuffers req;

	CLEAR(req);
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
		if (EINVAL == errno) {
			fprintf(stderr, "devcie does not support "
				"memory mapping\n");
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on input device");
		exit(EXIT_FAILURE);
	}

	buffers = (buffer*)(calloc(req.count, sizeof(*buffers)));
	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; (unsigned int)n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
		if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
			errno_exit("VIDIOC_QUERYBUF");
		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL ,buf.length, PROT_READ | PROT_WRITE,
						MAP_SHARED,fd, buf.m.offset);
		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::init_userp(unsigned int buffer_size) {
	struct v4l2_requestbuffers req;
	unsigned int page_size;

	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);
	CLEAR(req);
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
		if (EINVAL == errno) {
			fprintf(stderr, "device does not support "
				"user pointer i/o\n");
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}
	buffers = (buffer*)(calloc(4, sizeof(*buffers)));
	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = memalign(page_size, buffer_size);
		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::init_device(int* width, int *height) {
	struct v4l2_capability cap;
	int ret;
	int sizeimage;

	if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
		if (EINVAL == errno) {
			fprintf(stderr, "device is no V4L2 device\n");
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "device is no video capture device\n");
		exit(EXIT_FAILURE);
	}

	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "device does not support read i/o\n");
			exit(EXIT_FAILURE);
		}
		break;
	case V4L2_MEMORY_MMAP:
	case V4L2_MEMORY_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "device does not support streaming i/o\n");
			exit(EXIT_FAILURE);
		}
		break;
	}
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = frameWidth;
	fmt.fmt.pix.height = frameHeight;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
#ifdef WITH_V4L2_LIB
	v4lconvert_data = v4lconvert_create(fd);

	if (v4lconvert_data == NULL)
		errno_exit("v4lconvert_create");

	if (v4lconvert_try_format(v4lconvert_data, &fmt, &src_fmt) != 0)
		errno_exit("v4lconvert_try_format");

	ret = xioctl(fd, VIDIOC_S_FMT, &src_fmt);
	sizeimage = src_fmt.fmt.pix.sizeimage;
	dst_buf = (unsigned char*)malloc(fmt.fmt.pix.sizeimage);

	// The image size might have changed - set width and height
	frameWidth = *width = src_fmt.fmt.pix.width;
	frameHeight = *height = src_fmt.fmt.pix.height;

	printf("raw pixfmt: %c%c%c%c %dx%d\n",
		src_fmt.fmt.pix.pixelformat & 0xff,
	       (src_fmt.fmt.pix.pixelformat >> 8) & 0xff,
	       (src_fmt.fmt.pix.pixelformat >> 16) & 0xff,
	       (src_fmt.fmt.pix.pixelformat >> 24) & 0xff,
		src_fmt.fmt.pix.width, src_fmt.fmt.pix.height);
#else
	ret = xioctl(fd, VIDIOC_S_FMT, &fmt);
	sizeimage = fmt.fmt.pix.sizeimage;
#endif
	if (ret < 0)
		errno_exit("VIDIOC_S_FMT");
	printf("pixfmt: %c%c%c%c %dx%d\n",
		fmt.fmt.pix.pixelformat & 0xff,
	       (fmt.fmt.pix.pixelformat >> 8) & 0xff,
	       (fmt.fmt.pix.pixelformat >> 16) & 0xff,
	       (fmt.fmt.pix.pixelformat >> 24) & 0xff,
		fmt.fmt.pix.width, fmt.fmt.pix.height);
	switch (io) {
	case IO_METHOD_READ:
		init_read(sizeimage);
		break;
	case V4L2_MEMORY_MMAP:
		init_mmap();
		break;
	case V4L2_MEMORY_USERPTR:
		init_userp(sizeimage);
		break;
	}
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::close_device(void) {
	close(fd);
}

/*--------------------------------------------------------------------------------------*/

int linuxvideodevice::openvideodevice(int *width, int *height) {
	struct stat st;

	if (stat(devicename, &st) < 0) {
		fprintf(stderr, "Cannot identify device: %d, %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "device is no device\n");
		exit(EXIT_FAILURE);
	}
	fd = open(devicename, O_RDWR | O_NONBLOCK, 0);
	if (fd < 0) {
		fprintf(stderr, "Cannot open device: %d, %s\n",
			errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	init_device(width, height);
	start_capturing();

	return fd;
}

/*--------------------------------------------------------------------------------------*/

int linuxvideodevice::queryV4LControls() {
	struct v4l2_queryctrl ctrl;

	// Check all the standard controls
	for (int i=V4L2_CID_BASE; i<V4L2_CID_LASTP1; i++) {
		ctrl.id = i;
		if (0 == v4l2_ioctl(fd, VIDIOC_QUERYCTRL, &ctrl)) {
	    	if (ctrl.flags & V4L2_CTRL_FLAG_DISABLED)
	    		continue;

	    	addControl(ctrl);
		}
	}

	// Check any custom controls
	for (int i=V4L2_CID_PRIVATE_BASE; ; i++) {
		ctrl.id = i;
		if (0 == v4l2_ioctl(fd, VIDIOC_QUERYCTRL, &ctrl)) {
	    	if (ctrl.flags & V4L2_CTRL_FLAG_DISABLED)
	    		continue;

			addControl(ctrl);
		} else {
			break;
		}
	}

	return controlMap.size();
}

/*--------------------------------------------------------------------------------------*/

void linuxvideodevice::addControl(struct v4l2_queryctrl &ctrl) {
	if (controlMap.find(ctrl.id) == controlMap.end()) {
		// No element with that key exists in the map ...
	    switch(ctrl.type) {
			case V4L2_CTRL_TYPE_BOOLEAN:
	        case V4L2_CTRL_TYPE_INTEGER:
	        case V4L2_CTRL_TYPE_MENU:
				controlMap[ctrl.id] = new V4LControl(fd, ctrl);
				break;
	        case V4L2_CTRL_TYPE_BUTTON:
	        case V4L2_CTRL_TYPE_INTEGER64:
	        case V4L2_CTRL_TYPE_CTRL_CLASS:
	        default:
	            break;
	    }
	}
}

/*--------------------------------------------------------------------------------------*/

const V4LControl* linuxvideodevice::getV4LControl(int id) {
	V4LControlMap::iterator it;

	if (controlMap.end() != (it = controlMap.find(id))) {
		return (V4LControl*)it->second;
	}

	return NULL;
}
