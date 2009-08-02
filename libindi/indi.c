/*******************************************************************************
  Copyright(c) 2009 Geoffrey Hausheer. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
  
  
  Contact Information: gcx@phracturedblue.com <Geoffrey Hausheer>
*******************************************************************************/
//gcc -Wall -g -I. -o inditest indi.c indigui.c indi/base64.c indi/lilxml.c `pkg-config --cflags --libs gtk+-2.0 glib-2.0` -lz

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <zlib.h>

#include "lilxml.h"
#include "base64.h"

#include "indi.h"
#include "indigui.h"

#include <glib-object.h>

#ifndef INDI_DEBUG
  #define INDI_DEBUG 0
#endif

#define dbg_printf if(INDI_DEBUG) printf
/* Define the version of the INDI API that we support */
#define INDIV   1.7

#define LILLP(x) ((LilXML *)((x)->xml_parser)

static const char indi_state[4][6] = {
	"Idle",
	"Ok",
	"Busy",
	"Alert",
};

static const char indi_prop_type[6][8] = {
	"Unknown",
	"Text",
	"Switch",
	"Number",
	"Light",
	"BLOB",
};

struct indi_device_t *indi_find_device(struct indi_t *indi, const char *dev)
{
	GSList *gsl;
	struct indi_device_t *idev;

	for (gsl = indi->devices; gsl; gsl = g_slist_next(gsl)) {
		idev = (struct indi_device_t *)gsl->data;
		if (strncmp(idev->name, dev, sizeof(idev->name)) == 0) {
			return idev;
		}
	}
	idev = g_new0(struct indi_device_t, 1);
	strncpy(idev->name, dev, sizeof(idev->name));
	idev->indi = indi;
	indigui_make_device_page(idev);
	indi->devices = g_slist_prepend(indi->devices, idev);
	return idev;
}

struct indi_prop_t *indi_find_prop(struct indi_device_t *idev, const char *name)
{
	GSList *gsl;
	struct indi_prop_t *iprop;

	for (gsl = idev->props; gsl; gsl = g_slist_next(gsl)) {
		iprop = (struct indi_prop_t *)gsl->data;
		if (strncmp(iprop->name, name, sizeof(iprop->name)) == 0) {
			return iprop;
		}
	}
	return NULL;
}

struct indi_elem_t *indi_find_elem(struct indi_prop_t *iprop, const char *name)
{
	GSList *gsl;
	struct indi_elem_t *ielem;

	for (gsl = iprop->elems; gsl; gsl = g_slist_next(gsl)) {
		ielem = (struct indi_elem_t *)gsl->data;
		if (strncmp(ielem->name, name, sizeof(ielem->name)) == 0) {
			return ielem;
		}
	}
	return NULL;
}

double indi_prop_get_number(struct indi_prop_t *iprop, const char *elemname) {
	struct indi_elem_t *ielem;
	ielem = indi_find_elem(iprop, elemname);
	if(! ielem) {
		return 0;
	}
	return ielem->value.num.value;
}

struct indi_elem_t *indi_prop_set_number(struct indi_prop_t *iprop, const char *elemname, double value) {
	struct indi_elem_t *ielem;
	ielem = indi_find_elem(iprop, elemname);
	if(! ielem) {
		return NULL;
	}
	ielem->value.num.value = value;
	return ielem;
}

int indi_prop_get_switch(struct indi_prop_t *iprop, const char *elemname) {
	struct indi_elem_t *ielem;
	ielem = indi_find_elem(iprop, elemname);
	if(! ielem) {
		return 0;
	}
	return ielem->value.set;
}

struct indi_elem_t *indi_prop_set_switch(struct indi_prop_t *iprop, const char *elemname, int state)
{
	struct indi_elem_t *ielem;
	ielem = indi_find_elem(iprop, elemname);
	if(! ielem) {
		return NULL;
	}
	ielem->value.set = state;
	return ielem;
}

struct indi_elem_t *indi_dev_set_string(struct indi_device_t *idev, const char *propname, const char *elemname, const char *value)
{
	struct indi_prop_t *iprop;
	struct indi_elem_t *ielem;

	iprop = indi_find_prop(idev, propname);
	if(! iprop)
		return NULL;
	ielem = indi_find_elem(iprop, elemname);
	if(! ielem)
		return NULL;
	strncpy(ielem->value.str, value, sizeof(ielem->value.str));
	indi_send(iprop, ielem);
	return ielem;
}

struct indi_elem_t *indi_dev_set_switch(struct indi_device_t *idev, const char *propname, const char *elemname, int state)
{
	struct indi_prop_t *iprop;
	struct indi_elem_t *ielem;

	iprop = indi_find_prop(idev, propname);
	if(! iprop)
		return NULL;
	ielem = indi_find_elem(iprop, elemname);
	if(! ielem)
		return NULL;
	ielem->value.set =  state;
	indi_send(iprop, ielem);
	return ielem;
}

void indi_dev_enable_blob(struct indi_device_t *idev, int state)
{
	char msg[1024];
	unsigned int len;

	if (idev) {
		sprintf(msg, "<enableBLOB device=\"%s\">%s</enableBLOB>\n", idev->name, state ? "Also" : "Never");
		dbg_printf("sending (%d):\n%s", strlen(msg), msg);
		g_io_channel_write_chars(idev->indi->fh, msg, strlen(msg), &len, NULL);
		g_io_channel_flush(idev->indi->fh, NULL);
	}

}

static int indi_get_state_from_string(const char *statestr)
{
	if        (strcmp(statestr, "Idle") == 0) {
		return INDI_STATE_IDLE;
	} else if (strcasecmp(statestr, "Ok") == 0) {
		return INDI_STATE_OK;
	} else if (strcmp(statestr, "Busy") == 0) {
		return INDI_STATE_BUSY;
	}
	return INDI_STATE_ALERT;
}

const char *indi_get_string_from_state(int state)
{
	return indi_state[state];
}

int indi_get_type_from_string(const char *typestr)
{
	// 1st 3 chars are 'def', 'set', 'new', 'one'
	typestr +=3;

	if        (strncmp(typestr, "Text", 4) == 0) {
		return INDI_PROP_TEXT;
	} else if (strncmp(typestr, "Number", 6) == 0) {
		return INDI_PROP_NUMBER;
	} else if (strncmp(typestr, "Switch", 6) == 0) {
		return INDI_PROP_SWITCH;
	} else if (strncmp(typestr, "Light", 5) == 0) {
		return INDI_PROP_LIGHT;
	} else if (strncmp(typestr, "BLOB", 4) == 0) {
		return INDI_PROP_BLOB;
	}
	return INDI_PROP_UNKNOWN;
}

void indi_prop_add_signal(struct indi_prop_t *iprop, void *object, unsigned long signal)
{
	struct indi_signals_t *sig = g_new0(struct indi_signals_t, 1);
	sig->object = object;
	sig->signal = signal;
	iprop->signals = g_slist_prepend(iprop->signals, sig);
}

void indi_prop_set_signals(struct indi_prop_t *iprop, int active)
{
	GSList *gsl;
	for (gsl = iprop->signals; gsl; gsl = g_slist_next(gsl)) {
		struct indi_signals_t *sig = (struct indi_signals_t *)gsl->data;
		if(active) {
			g_signal_handler_unblock(G_OBJECT (sig->object), sig->signal);
		} else {
			g_signal_handler_block(G_OBJECT (sig->object), sig->signal);
		}
	}
}

void indi_send(struct indi_prop_t *iprop, struct indi_elem_t *ielem )
{
	char msg[4096], *ptr = msg;
	unsigned int len;
	char val[80];
	const char *valstr;
	const char *type;
	struct indi_device_t *idev = iprop->idev;
	GSList *gsl;

	type = indi_prop_type[iprop->type];
	ptr += sprintf(msg, "<new%sVector device=\"%s\" name=\"%s\">\n", type, idev->name, iprop->name);
	for (gsl = iprop->elems; gsl; gsl = g_slist_next(gsl)) {
		struct indi_elem_t *elem = (struct indi_elem_t *)gsl->data;
		if (ielem && elem != ielem) {
			continue;
		}
		switch (iprop->type) {
		case INDI_PROP_TEXT:
			valstr = elem->value.str;
			break;
		case INDI_PROP_NUMBER:
			sprintf(val, "%f", elem->value.num.value);
			valstr = val;
			break;
		case INDI_PROP_SWITCH:
			sprintf(val, "%s", elem->value.set ? "On" : "Off");
			valstr = val;
			break;

		}			
		ptr += sprintf(ptr, "  <one%s name=\"%s\">%s</one%s>\n", type, elem->name, valstr, type);
	}
	ptr += sprintf(ptr, "</new%sVector>\n", type);
	iprop->state = INDI_STATE_BUSY;
	dbg_printf("sending %s(%d):\n%s", type, strlen(msg), msg);
	indigui_update_widget(iprop);
	g_io_channel_write_chars(iprop->idev->indi->fh, msg, strlen(msg), &len, NULL);
	g_io_channel_flush(iprop->idev->indi->fh, NULL);
}

#define INDI_CHUNK_SIZE 65536
static int indi_blob_decode(struct indi_elem_t *ielem)
{
	char *ptr;
	int count = INDI_CHUNK_SIZE;
	int src_len;
	int pos = ielem->value.blob.ptr - ielem->value.blob.data;

	printf("Decoding from %d - %p\n", pos, ielem->iprop->root);
	if (ielem->value.blob.compressed) {
		if(! ielem->value.blob.zstrm)
			ielem->value.blob.zstrm = g_new0(z_stream, 1);
		if(pos == 0) {
			memset(ielem->value.blob.zstrm, 0, sizeof(z_stream));
			inflateInit(ielem->value.blob.zstrm);
		}
		if(! ielem->value.blob.tmp_data)
			ielem->value.blob.tmp_data = g_malloc(INDI_CHUNK_SIZE);
		ptr = ielem->value.blob.tmp_data;
	} else {
		ptr = ielem->value.blob.ptr;
	}
	if ((src_len = from64tobits(ptr,  ielem->value.blob.orig_data, &count)) < 0) {
		// failed to convert
		printf("Failed to decode base64 BLOB at %d\n", pos);
		ielem->value.blob.orig_size = 0;
		//FIXME: This should really only happen when all blobs are done decoding
		delXMLEle(ielem->iprop->root);
		ielem->value.blob.orig_data = NULL;
		return FALSE;
	}
	ielem->value.blob.orig_data += count;
	ielem->value.blob.orig_size -= count;
	if (ielem->value.blob.compressed) {
		z_stream *strm;
 		strm = ielem->value.blob.zstrm;
		strm->avail_out = ielem->value.blob.size - pos;
		strm->avail_in = src_len;
		strm->next_in = (unsigned char *)ptr;
		strm->next_out = (unsigned char *)ielem->value.blob.ptr;
		printf("\t Decompressing BLOB\n");
		if (inflate(strm, Z_NO_FLUSH) < 0) {
			// failed to convert
			printf("Failed to decompress BLOB at %d\n", pos);
			ielem->value.blob.orig_size = 0;
			//FIXME: This should really only happen when all blobs are done decoding
			delXMLEle(ielem->iprop->root);
			return FALSE;
		}
		ielem->value.blob.ptr = ielem->value.blob.data + (ielem->value.blob.size - strm->avail_out);
	} else {
		ielem->value.blob.ptr += src_len;
	}
	if (ielem->value.blob.orig_size == 0) {
		//We're done
		//FIXME: This should really only happen when all blobs are done decoding
		if (ielem->value.blob.compressed) {
			inflateEnd(ielem->value.blob.zstrm);
		}
		delXMLEle(ielem->iprop->root);
		if (ielem->iprop->prop_update_cb) {
			ielem->iprop->prop_update_cb(ielem->iprop, ielem->iprop->callback_data);
		}
		return FALSE;
	}
	return TRUE;
		
}

static int indi_convert_data(struct indi_elem_t *ielem, int type, const char *data, unsigned int data_size)
{
	if(! data)
		return FALSE;
	switch(type) {
	case INDI_PROP_TEXT:
		strncpy(ielem->value.str, data, sizeof(ielem->value.str));
		break;
	case INDI_PROP_NUMBER:
		ielem->value.num.value = strtod(data, NULL);
		break;
	case INDI_PROP_SWITCH:
		if (strcmp(data, "On") == 0) {
			ielem->value.set = 1;
		} else {
			ielem->value.set = 0;
		}
		break;
	case INDI_PROP_LIGHT:
		ielem->value.set = indi_get_state_from_string(data);
		break;
	case INDI_PROP_BLOB:
		if (ielem->value.blob.orig_size || ! data_size) {
			return FALSE;
		}
		if (ielem->value.blob.data && ielem->value.blob.size > ielem->value.blob.data_size) {
			// We free rather than realloc because there is no reason to copy
			// The old data if a new location is needed
			g_free(ielem->value.blob.data);
			ielem->value.blob.data = NULL;
		}
		if (! ielem->value.blob.data) {
			ielem->value.blob.data = g_malloc(ielem->value.blob.size);
			ielem->value.blob.data_size = ielem->value.blob.size;
		}
		ielem->value.blob.ptr = ielem->value.blob.data;
		ielem->value.blob.orig_data = data;
		ielem->value.blob.orig_size = data_size;
		printf("Found blob type: %s size: %d\n", ielem->value.blob.fmt, ielem->value.blob.size);
		ielem->value.blob.compressed = (ielem->value.blob.fmt[strlen(ielem->value.blob.fmt)-2] == '.'
			 && ielem->value.blob.fmt[strlen(ielem->value.blob.fmt)-1] == 'z')
			 ? 1 : 0;
		g_idle_add((GSourceFunc)indi_blob_decode, ielem);
		return TRUE;
	}
	return FALSE;
}

static void indi_update_prop(XMLEle *root, struct indi_prop_t *iprop)
{
	XMLEle *ep;
	int save = 0;

	iprop->root = root;
	iprop->state = indi_get_state_from_string(findXMLAttValu(root, "state"));
	for (ep = nextXMLEle (root, 1); ep != NULL; ep = nextXMLEle (root, 0)) {
		struct indi_elem_t *ielem;
		ielem = indi_find_elem(iprop, findXMLAttValu(ep, "name"));
		if (! ielem) {
			continue;
		}
		if(iprop->type == INDI_PROP_BLOB) {
			ielem->value.blob.size = strtoul(findXMLAttValu(ep, "size"), NULL, 10);
			strncpy(ielem->value.blob.fmt, findXMLAttValu(ep, "format"), sizeof(ielem->value.blob.fmt));
		}
		save |= indi_convert_data(ielem, iprop->type, pcdataXMLEle(ep), pcdatalenXMLEle(ep));
	}
	if (! save)
		delXMLEle (root);
	indigui_update_widget(iprop);
}

static struct indi_prop_t *indi_new_prop(XMLEle *root, struct indi_device_t *idev)
{
	const char *perm, *label, *rule;
	struct indi_prop_t *iprop;
	XMLEle *ep;

	iprop = g_new0(struct indi_prop_t, 1);
	iprop->idev = idev;

	strncpy(iprop->name, findXMLAttValu(root, "name"), sizeof(iprop->name));

	perm = findXMLAttValu(root, "perm");
	if(strcmp(perm, "rw") == 0) {
		iprop->permission = INDI_RW;
	} else if(strcmp(perm, "ro") == 0) {
		iprop->permission = INDI_RO;
	} else if(strcmp(perm, "wo") == 0) {
		iprop->permission = INDI_WO;
	}

	iprop->state = indi_get_state_from_string(findXMLAttValu(root, "state"));

	iprop->type = indi_get_type_from_string(tagXMLEle(root));

	for (ep = nextXMLEle (root, 1); ep != NULL; ep = nextXMLEle (root, 0)) {
		struct indi_elem_t *ielem;

		if (indi_get_type_from_string(tagXMLEle(ep)) != iprop->type) {
			// Unhandled type
			continue;
		}
		ielem = g_new0(struct indi_elem_t, 1);
		ielem->iprop = iprop;
		strncpy(ielem->name, findXMLAttValu(ep, "name"), sizeof(ielem->name));
		label = findXMLAttValu(root, "label");
		if (label) {
			strncpy(ielem->label, label, sizeof(ielem->label));
		} else {
			strncpy(ielem->label, ielem->name, sizeof(ielem->label));
		}
		indi_convert_data(ielem, iprop->type, pcdataXMLEle(ep), pcdatalenXMLEle(ep));
		if(iprop->type == INDI_PROP_NUMBER) {
			strncpy(ielem->value.num.fmt, findXMLAttValu(ep, "format"), sizeof(ielem->value.num.fmt));
			ielem->value.num.min  = strtod(findXMLAttValu(ep, "min"), NULL);
			ielem->value.num.max  = strtod(findXMLAttValu(ep, "max"), NULL);
			ielem->value.num.step = strtod(findXMLAttValu(ep, "step"), NULL);
		}
			
		iprop->elems = g_slist_prepend(iprop->elems, ielem);
	}
	if (iprop->type == INDI_PROP_SWITCH) {
		rule = findXMLAttValu(root, "rule");
		if(strcmp(rule, "OneOfMany") == 0) {
			iprop->rule = INDI_RULE_ONEOFMANY;
		} else if (strcmp(rule, "AtMostOne") == 0) {
			iprop->rule = INDI_RULE_ATMOSTONE;
		} else {
			iprop->rule = INDI_RULE_ANYOFMANY;
		}
	}
	idev->props = g_slist_prepend(idev->props, iprop);
	return iprop;
}

void indi_prop_add_cb(struct indi_prop_t *iprop,
                      void (* prop_update_cb)(struct indi_prop_t *iprop, void *callback_data),
                      void *callback_data)
{
	iprop->prop_update_cb = prop_update_cb;
	iprop->callback_data = callback_data;
}

#ifdef INDI_TEST_BLOB
static void indi_camera_capture_cb(struct indi_prop_t *iprop, void *data)
{
	FILE *fh;
	char str[80];
	static int img_count;
	struct indi_elem_t *ielem = indi_find_elem(iprop, "CCD1");

	sprintf(str, "test%03d.fits", img_count++);
	printf("Writing: %s\n", str);
	fh = fopen(str, "w+");
	fwrite(ielem->value.blob.data, ielem->value.blob.size, 1, fh);
	fclose(fh);
}
#endif

static void indi_handle_message(struct indi_device_t *idev, XMLEle *root)
{
	struct indi_prop_t *iprop;
	const char *proptype = tagXMLEle(root);
	const char *propname = findXMLAttValu(root, "name");
	const char default_group[] = "Main";
	const char *groupname;

	if        (strncmp(proptype, "set", 3) == 0) {
		// Update values
		iprop = indi_find_prop(idev, propname);
		if (! iprop) {
			return;
		}
		indi_update_prop(root, iprop);
		if (iprop->prop_update_cb && iprop->type != INDI_PROP_BLOB) {
			// BLOB callbacks are handled after decoding
			iprop->prop_update_cb(iprop, iprop->callback_data);
		}
	} else if (strncmp(proptype, "def", 3) == 0) {
		// Exit if this property is already known
		if (indi_find_prop(idev, propname)) {
			return;
		}
		iprop = indi_new_prop(root, idev);
#ifdef INDI_TEST_BLOB
		if (iprop->type == INDI_PROP_BLOB) {
			indi_dev_enable_blob(iprop->idev, 1);
		        indi_prop_add_cb(iprop, indi_camera_capture_cb, NULL);
		}
#endif
		// We need to build GUI elements here
		groupname = findXMLAttValu(root, "group");
		if (! groupname) {
			groupname = default_group;
		}
		indigui_add_prop(idev, groupname, iprop);
		delXMLEle (root);
		if (idev->indi->new_prop_cb) {
			idev->indi->new_prop_cb(iprop, idev->indi->callback_data);
		}
	}
}

static gboolean indi_read_cb (GIOChannel *source, GIOCondition condition, struct indi_t *indi)
{
	int status, i;
	unsigned int len;
	char buf[4096];
	char errmsg[1024];
	XMLEle *root;
	struct indi_device_t *idev;
	LilXML *lillp = (LilXML *)indi->xml_parser;

	/* read from server, exit if find all requested properties */
	status = g_io_channel_read_chars(source, buf, sizeof(buf), &len, NULL);
	if(len) {
		dbg_printf("Received (%d): %s\n", len, buf);
		for(i = 0; i < len; i++) {
			root = readXMLEle(lillp, buf[i], errmsg);
		        if (root) {
				const char *dev = findXMLAttValu (root, "device");
				if (! dev)
					continue;
				idev = indi_find_device(indi, dev);
				indi_handle_message(idev, root);
			}
		}
	}
	return TRUE;
}

static GIOChannel *openINDIServer (char *host, int port)
{
	struct sockaddr_in serv_addr;
	struct hostent *hp;
	int sockfd;
	GIOChannel *fh;

	/* lookup host address */
	hp = gethostbyname (host);
	if (!hp) {
	    herror ("gethostbyname");
	    exit (2);
	}

	/* create a socket to the INDI server */
	(void) memset ((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr =
			    ((struct in_addr *)(hp->h_addr_list[0]))->s_addr;
	serv_addr.sin_port = htons(port);
	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
	    perror ("socket");
	    exit(2);
	}

	/* connect */
	if (connect (sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0){
	    perror ("connect");
	    exit(2);
	}

	/* prepare for line-oriented i/o with client */
	fh = g_io_channel_unix_new(sockfd);
	g_io_channel_set_flags (fh, G_IO_FLAG_NONBLOCK, NULL);
	return fh;
}

struct indi_t *indi_init(void (* new_prop_cb)(struct indi_prop_t *iprop, void *callback_data), void *callback_data)
{
	struct indi_t *indi;

	unsigned int len;
	char msg[1024];

	indi = g_new0(struct indi_t, 1);

	indi->window = indigui_create_window();

	indi->xml_parser = (void *)newLilXML();
	indi->fh = openINDIServer("localhost", 7624);
	if (new_prop_cb) {
		indi->new_prop_cb = new_prop_cb;
		indi->callback_data = callback_data;
	}
	g_io_add_watch(indi->fh, G_IO_IN, (GIOFunc) indi_read_cb, indi);
	sprintf(msg, "<getProperties version='%g'/>\n", INDIV);
	g_io_channel_write_chars(indi->fh, msg, strlen(msg), &len, NULL);
	g_io_channel_flush(indi->fh, NULL);

	return indi;
}

