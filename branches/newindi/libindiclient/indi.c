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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#include <netinet/in.h>
#else
#include <stdlib.h>
#include <string.h>
#pragma warning(disable: 4996)
#define strcasecmp(s1, s2) _stricmp(s1, s2)
#define snprintf _snprintf
#endif

#include <sys/types.h>


#include "zlib.h"

#include "lilxml.h"
#include "base64.h"

#include "indi.h"
#include "indigui.h"
#include "indi_io.h"
#include "indi_config.h"

#ifndef INDI_DEBUG
  #define INDI_DEBUG 0
#endif

#define dbg_printf if(INDI_DEBUG) printf
/* Define the version of the INDI API that we support */
#define INDIV   1.7

#define LILLP(x) ((LilXML *)((x)->xml_parser)

#ifndef FALSE
  #define FALSE (0)
#endif
#ifndef TRUE
  #define TRUE (1)
#endif

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

static struct indi_dev_cb_t *indi_find_dev_cb(struct indi_t *indi, const char *devname)
{
	indi_list *isl;

	for (isl = il_iter(indi->dev_cb_list); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_dev_cb_t *cb = (struct indi_dev_cb_t *)il_item(isl);
		if (strlen(cb->devname) == 0 || strncmp(cb->devname, devname, sizeof(cb->devname)) == 0) {
			return cb;
		}
	}
	return NULL;
}

struct indi_device_t *indi_find_device(struct indi_t *indi, const char *dev)
{
	indi_list *isl;
	struct indi_device_t *idev;

	if(! indi)
		return NULL;
	for (isl = il_iter(indi->devices); ! il_is_last(isl); isl = il_next(isl)) {
		idev = (struct indi_device_t *)il_item(isl);
		if (strlen(dev) == 0 || strncmp(idev->name, dev, sizeof(idev->name)) == 0) {
			return idev;
		}
	}
	return NULL;
}

static struct indi_cb_t *indi_create_cb(void (* cb_func)(void *, void *), void *cb_data)
{
	struct indi_cb_t *cb = (struct indi_cb_t *)calloc(sizeof(struct indi_cb_t), 1);
	cb->func = cb_func;
	cb->data = cb_data;
	return cb;
}

static struct indi_device_t *indi_new_device(struct indi_t *indi, const char *devname)
{
	struct indi_device_t *idev;
	struct indi_dev_cb_t *cb, *first_cb = NULL;
	indi_list *isl;

	idev = indi_find_device(indi, devname);
	if (idev)
		return idev;
	idev = (struct indi_device_t *)calloc(1, sizeof(struct indi_device_t));
	strncpy(idev->name, devname, sizeof(idev->name));
	idev->indi = indi;

	while ((cb = indi_find_dev_cb(indi, devname)) && cb != first_cb) {
		if (!first_cb) 
			first_cb = cb;
		idev->new_prop_cb = il_append(idev->new_prop_cb, indi_create_cb(cb->cb.func, cb->cb.data));

		indi->dev_cb_list = il_remove(indi->dev_cb_list, cb);
		if (strlen(cb->devname)) {
			free(cb);
		} else {
			//push this property onto the end of the list
			il_append(indi->dev_cb_list, cb);
		}
	}

	indigui_make_device_page(idev);
	indi->devices = il_append(indi->devices, idev);
	for (isl = il_iter(indi->newdev_cb_list); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_cb_t *cb = (struct indi_cb_t *)il_item(isl);
		cb->func(idev, cb->data);
	}
	return idev;
}

struct indi_prop_t *indi_find_prop(struct indi_device_t *idev, const char *name)
{
	indi_list *isl;
	struct indi_prop_t *iprop;
	for (isl = il_iter(idev->props); ! il_is_last(isl); isl = il_next(isl)) {
                iprop = (struct indi_prop_t *)il_item(isl);
		if (strncmp(iprop->name, name, sizeof(iprop->name)) == 0) {
			return iprop;
		}
	}
	return NULL;
}

struct indi_elem_t *indi_find_elem(struct indi_prop_t *iprop, const char *name)
{
	indi_list *isl;
	struct indi_elem_t *ielem;

	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl)) {
                ielem = (struct indi_elem_t *)il_item(isl);
		if (strncmp(ielem->name, name, sizeof(ielem->name)) == 0) {
			return ielem;
		}
	}
	return NULL;
}

struct indi_elem_t *indi_find_first_elem(struct indi_prop_t *iprop)
{
	struct indi_elem_t *ielem = NULL;

	if (iprop->elems)
		ielem = (struct indi_elem_t *)il_first(iprop->elems);
	return ielem;
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

struct indi_elem_t *indi_prop_set_string(struct indi_prop_t *iprop, const char *elemname, const char *value) {
	struct indi_elem_t *ielem;
	ielem = indi_find_elem(iprop, elemname);
	if(! ielem) {
		return NULL;
	}
	strncpy(ielem->value.str, value, sizeof(ielem->value.str));
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

	if (idev) {
		sprintf(msg, "<enableBLOB device=\"%s\">%s</enableBLOB>\n", idev->name, state ? "Also" : "Never");
		dbg_printf("sending (%lu):\n%s", (unsigned long)strlen(msg), msg);
		io_indi_sock_write(idev->indi->fh, msg, strlen(msg));
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

void indi_send(struct indi_prop_t *iprop, struct indi_elem_t *ielem )
{
	char msg[4096], *ptr = msg;
	char val[80];
	const char *valstr = NULL;
	const char *type;
	struct indi_device_t *idev = iprop->idev;
	indi_list *isl;

	type = indi_prop_type[iprop->type];
	ptr += sprintf(msg, "<new%sVector device=\"%s\" name=\"%s\">\n", type, idev->name, iprop->name);
	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
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
		/* do not send uninitialized data to the server */
		if (valstr) {
			ptr += sprintf(ptr, "  <one%s name=\"%s\">%s</one%s>\n", type, elem->name, valstr, type);
		} else {
			dbg_printf("WARNING: unhandled iprop type %d for elem '%s'", iprop->type, elem->name);
		}
	}
	ptr += sprintf(ptr, "</new%sVector>\n", type);
	iprop->state = INDI_STATE_BUSY;
	dbg_printf("sending %s(%lu):\n%s", type, (unsigned long)strlen(msg), msg);
	indigui_update_widget(iprop);
	io_indi_sock_write(iprop->idev->indi->fh, msg, strlen(msg));
}

static void indi_exec_cb(void *cb_list, void *idata)
{
	indi_list *isl;

	if (! cb_list)
		return;
	for (isl = il_iter(cb_list); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_cb_t *cb = ( struct indi_cb_t *)il_item(isl);
		cb->func(idata, cb->data);
	}
}

#define INDI_CHUNK_SIZE 65536
static int indi_blob_decode(void *data)
{
	struct indi_elem_t *ielem = (struct indi_elem_t *)data;
	char *ptr;
	int count = INDI_CHUNK_SIZE;
	int src_len;
	int pos = ielem->value.blob.ptr - ielem->value.blob.data;

	//printf("Decoding from %d - %p\n", pos, ielem->iprop->root);
	if (ielem->value.blob.compressed) {
		if(! ielem->value.blob.zstrm)
			ielem->value.blob.zstrm = (z_stream *)calloc(1, sizeof(z_stream));
		if(pos == 0) {
			memset(ielem->value.blob.zstrm, 0, sizeof(z_stream));
			inflateInit((z_stream *)ielem->value.blob.zstrm);
		}
		if(! ielem->value.blob.tmp_data)
			ielem->value.blob.tmp_data = (char *)malloc(INDI_CHUNK_SIZE);
		ptr = ielem->value.blob.tmp_data;
	} else {
		ptr = ielem->value.blob.ptr;
	}
	if ((src_len = from64tobits(ptr,  ielem->value.blob.orig_data, &count)) < 0) {
		// failed to convert
		printf("Failed to decode base64 BLOB at %d\n", pos);
		ielem->value.blob.orig_size = 0;
		//FIXME: This should really only happen when all blobs are done decoding
		delXMLEle((XMLEle *)ielem->iprop->root);
		ielem->value.blob.orig_data = NULL;
		return FALSE;
	}
	ielem->value.blob.orig_data += count;
	ielem->value.blob.orig_size -= count;
	if (ielem->value.blob.compressed) {
		z_stream *strm;
 		strm = (z_stream *)ielem->value.blob.zstrm;
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
			delXMLEle((XMLEle *)ielem->iprop->root);
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
			inflateEnd((z_stream *)ielem->value.blob.zstrm);
		}
		delXMLEle((XMLEle *)ielem->iprop->root);
		indi_exec_cb(ielem->iprop->prop_update_cb, ielem->iprop);
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
			free(ielem->value.blob.data);
			ielem->value.blob.data = NULL;
		}
		if (! ielem->value.blob.data) {
			ielem->value.blob.data = (char *)malloc(ielem->value.blob.size);
			ielem->value.blob.data_size = ielem->value.blob.size;
		}
		ielem->value.blob.ptr = ielem->value.blob.data;
		ielem->value.blob.orig_data = data;
		ielem->value.blob.orig_size = data_size;
		//printf("Found blob type: %s size: %lu\n", ielem->value.blob.fmt, (unsigned long)ielem->value.blob.size);
		ielem->value.blob.compressed = (ielem->value.blob.fmt[strlen(ielem->value.blob.fmt)-2] == '.'
			 && ielem->value.blob.fmt[strlen(ielem->value.blob.fmt)-1] == 'z')
			 ? 1 : 0;
		io_indi_idle_callback(indi_blob_decode, ielem);
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
	strncpy(iprop->message, findXMLAttValu(root, "message"), sizeof(iprop->message) - 1);
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

	iprop = (struct indi_prop_t *)calloc(1, sizeof(struct indi_prop_t));
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
		ielem = (struct indi_elem_t *)calloc(1, sizeof(struct indi_elem_t));
		ielem->iprop = iprop;
		strncpy(ielem->name, findXMLAttValu(ep, "name"), sizeof(ielem->name));
		label = findXMLAttValu(ep, "label");
		if (label && strlen(label)) {
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
			
		iprop->elems = il_append(iprop->elems, ielem);
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
	idev->props = il_append(idev->props, iprop);
	return iprop;
}
static void indi_delete_elem(struct indi_elem_t *ielem)
{
	if(ielem->iprop->type == INDI_PROP_BLOB) {
		if(ielem->value.blob.data)
			free(ielem->value.blob.data);
		if(ielem->value.blob.tmp_data)
			free(ielem->value.blob.tmp_data);
		if(ielem->value.blob.zstrm)
			free(ielem->value.blob.zstrm);
	}
	free(ielem);
}

static void indi_delete_prop(struct indi_prop_t *iprop)
{
	indi_list *isl;
	struct indi_elem_t *ielem;

	indigui_delete_prop(iprop);

	while((isl = il_iter(iprop->elems)) && ! il_is_last(isl)) {
                ielem = (struct indi_elem_t *)il_item(isl);
		iprop->elems = il_remove_first(iprop->elems);
		indi_delete_elem(ielem);
	}
	if(iprop->root)
		delXMLEle((XMLEle *)iprop->root);
	iprop->idev->props = il_remove(iprop->idev->props, iprop);
	free(iprop);
}

void indi_new_device_cb(struct indi_t *indi,
                     void (* cb_func)(void *idev, void *cb_data),
                     void *cb_data)
{
	struct indi_cb_t *cb = indi_create_cb(cb_func, cb_data);
	indi->newdev_cb_list = il_append(indi->newdev_cb_list, cb);
}

void indi_remove_cb(struct indi_t *indi, void (* cb_func)(void *idev, void *cb_data))
{
	indi_list *isl;
	/* New device callbacks */
	for (isl = il_iter(indi->newdev_cb_list); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_cb_t *cb = (struct indi_cb_t *)il_item(isl);
		if (cb->func == cb_func) {
			indi->newdev_cb_list = il_remove(indi->newdev_cb_list, cb);
			free(cb);
			break;
		}
	}
	/* Unapplied device callbacks */
	for (isl = il_iter(indi->dev_cb_list); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_dev_cb_t *cb = (struct indi_dev_cb_t *)il_item(isl);
		if (cb->cb.func == cb_func) {
			indi->dev_cb_list = il_remove(indi->dev_cb_list, cb);
			free(cb);
			break;
		}
	}
	/* Applied device callbacks */
	for (isl = il_iter(indi->devices); ! il_is_last(isl); isl = il_next(isl)) {
		indi_list *isl1;
		struct indi_device_t *idev = (struct indi_device_t *)il_item(isl);
		for (isl1 = il_iter(idev->new_prop_cb); ! il_is_last(isl1); isl1 = il_next(isl1)) {
			struct indi_dev_cb_t *cb = (struct indi_dev_cb_t *)il_item(isl1);
			if (cb->cb.func == cb_func) {
				idev->new_prop_cb = il_remove(idev->new_prop_cb, cb);
				free(cb);
				break;
			}
		}
		/* Applied prop update callbacks */
		for (isl1 = il_iter(idev->props); ! il_is_last(isl1); isl1 = il_next(isl1)) {
			indi_list *isl2;
			struct indi_prop_t *iprop = (struct indi_prop_t *)il_item(isl1);
			for (isl2 = il_iter(iprop->prop_update_cb); ! il_is_last(isl2); isl2 = il_next(isl2)) {
				struct indi_cb_t *cb = (struct indi_cb_t *)il_item(isl2);
				if (cb->func == cb_func) {
					iprop->prop_update_cb = il_remove(iprop->prop_update_cb, cb);
					free(cb);
					break;
				}
			}
		}
	}
}

void indi_device_add_cb(struct indi_t *indi, const char *devname,
                     void (* cb_func)(void *iprop, void *cb_data),
                     void *cb_data)
{
	struct indi_device_t *idev;
	struct indi_prop_t *iprop;
	indi_list *isl;

	idev = indi_find_device(indi, devname);
	if (idev) {
		idev->new_prop_cb = il_append(idev->new_prop_cb, indi_create_cb(cb_func, cb_data));

		// Execute the callback for all existing properties
		for (isl = il_iter(idev->props); ! il_is_last(isl); isl = il_next(isl)) {
			iprop = (struct indi_prop_t *)il_item(isl);
			cb_func(iprop, cb_data);
		}
	} else {
		// Device doesn't exist yet, so save this callback for the future
		struct indi_dev_cb_t *cb = (struct indi_dev_cb_t *)calloc(1, sizeof(struct indi_dev_cb_t));
		strncpy(cb->devname, devname, sizeof(cb->devname));
		cb->cb.func = cb_func;
		cb->cb.data = cb_data;
		indi->dev_cb_list = il_append(indi->dev_cb_list, cb);
	}
}

void indi_prop_add_cb(struct indi_prop_t *iprop,
                      void (* cb_func)(void *iprop, void *cb_data),
                      void *cb_data)
{
	iprop->prop_update_cb = il_append(iprop->prop_update_cb, indi_create_cb(cb_func, cb_data));
}


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
		if (iprop->type != INDI_PROP_BLOB) {
			// BLOB callbacks are handled after decoding
			indi_exec_cb(iprop->prop_update_cb, iprop);
		}
		ic_prop_set(idev->indi->config, iprop);
	} else if (strncmp(proptype, "def", 3) == 0) {
		// Exit if this property is already known
		if (indi_find_prop(idev, propname)) {
			return;
		}
		iprop = indi_new_prop(root, idev);
		// We need to build GUI elements here
		groupname = findXMLAttValu(root, "group");
		if (! groupname) {
			groupname = default_group;
		}
		indigui_add_prop(idev, groupname, iprop);
		delXMLEle (root);
		indi_exec_cb(idev->new_prop_cb, iprop);
		ic_prop_def(idev->indi->config, iprop);
	} else if (strncmp(proptype, "del", 3) == 0) {
		//Delete property or device
		if(propname) {
			iprop = indi_find_prop(idev, propname);
			if (! iprop) {
				return;
			}
			indi_delete_prop(iprop);
		} else {
			//indi_delete_device(idev);
		}
	} else if (strncmp(proptype, "message", 7) == 0) {
		// Display message
		indigui_show_message(idev->indi, findXMLAttValu(root, "message"));
		delXMLEle (root);
	} else {
		char str[256] = {0};
		snprintf(str, sizeof(str)-1, "Unknown property recieved: %s\n", proptype);
		indigui_show_message(idev->indi, str);
	}
}

void indi_read_cb (void *fd, void *opaque)
{
	struct indi_t *indi = (struct indi_t *)opaque;
	int i, len;
	char buf[4096];
	char errmsg[1024];
	XMLEle *root;
	struct indi_device_t *idev;
	LilXML *lillp = (LilXML *)indi->xml_parser;

	len = io_indi_sock_read(fd, buf, sizeof(buf));
	if(len > 0) {
		dbg_printf("Received (%d): %s\n", len, buf);
		for(i = 0; i < len; i++) {
			root = readXMLEle(lillp, buf[i], errmsg);
		        if (root) {
				const char *dev = findXMLAttValu (root, "device");
				if (! dev) {
					const char *proptype = tagXMLEle(root);
					if (strncmp(proptype, "message", 7) == 0) {
						indigui_show_message(indi, findXMLAttValu(root, "message"));
					}
					continue;
				}
				idev = indi_new_device(indi, dev);
				indi_handle_message(idev, root);
			}
		}
	}
}

struct indi_t *indi_init(const char *hostname, int port, const char *config)
{
	struct indi_t *indi;

	char msg[1024];

	indi = (struct indi_t *)calloc(1, sizeof(struct indi_t));

	indi->ClientCount = 0;
	indi->window = indigui_create_window(indi);
	indi->config = ic_init(indi, config);

	indi->xml_parser = (void *)newLilXML();
	indi->fh = io_indi_open_server(hostname, port, indi_read_cb, indi);
	if (! indi->fh) {
		fprintf(stderr, "Failed to connect to INDI server\n");
		free(indi);
		return NULL;
	}
	sprintf(msg, "<getProperties version='%g'/>\n", INDIV);
	
	io_indi_sock_write(indi->fh, msg, strlen(msg));
	return indi;
}

void indi_close(struct indi_t *indi)
{
	if (indi) {
	  io_indi_close_server(indi->fh); 
	}  
}
