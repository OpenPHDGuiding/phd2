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
//gcc -Wall -g -I. -o inditest indi.c indigui.c indi/base64.c indi/lilxml.c `pkg-config --cflags --libs gtk+-2.0 glib-2.0` -lz -DINDIMAIN
#include "indi.h"
#include "indigui.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

void indigui_make_device_page(struct indi_device_t *idev)
{
	GtkWidget *parent_notebook;
	idev->window = gtk_notebook_new();
	parent_notebook = g_object_get_data(G_OBJECT (idev->indi->window), "notebook");
	gtk_notebook_append_page(GTK_NOTEBOOK (parent_notebook), idev->window, gtk_label_new(idev->name));
	gtk_widget_show_all(parent_notebook);
}

void indigui_update_widget(struct indi_prop_t *iprop)
{
	GtkWidget *value;
	GtkWidget *entry;
	GtkWidget *state_label;
	GSList *gsl;
	char val[80];

	indi_prop_set_signals(iprop, 0);
	for (gsl = iprop->elems; gsl; gsl = g_slist_next(gsl)) {
		struct indi_elem_t *elem = (struct indi_elem_t *)gsl->data;

		value = g_object_get_data(G_OBJECT (iprop->widget), elem->name);
		switch (iprop->type) {
		case INDI_PROP_TEXT:
			entry = g_object_get_data(G_OBJECT (value), "entry");
			gtk_label_set_text(GTK_LABEL (value), elem->value.str);
			break;
		case INDI_PROP_NUMBER:
			entry = g_object_get_data(G_OBJECT (value), "entry");
			sprintf(val, "%f", elem->value.num.value);
			gtk_label_set_text(GTK_LABEL (value), val);
			break;
		case INDI_PROP_SWITCH:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (value), elem->value.set);
			break;
		}
	}
	state_label = g_object_get_data(G_OBJECT (iprop->widget), "_state");
	gtk_label_set_text(GTK_LABEL(state_label), indi_get_string_from_state(iprop->state));
	indi_prop_set_signals(iprop, 1);
}

static void indigui_send_cb( GtkWidget *widget, struct indi_prop_t *iprop )
{
	const char *valstr;
	GSList *gsl;
	GtkWidget *value;
	GtkWidget *entry;

	for (gsl = iprop->elems; gsl; gsl = g_slist_next(gsl)) {
		struct indi_elem_t *elem = (struct indi_elem_t *)gsl->data;
		value = g_object_get_data(G_OBJECT (iprop->widget), elem->name);
		switch (iprop->type) {
		case INDI_PROP_TEXT:
			entry = g_object_get_data(G_OBJECT (value), "entry");
			valstr = gtk_entry_get_text(GTK_ENTRY (entry));
			strncpy(elem->value.str, valstr, sizeof(elem->value.str));
			gtk_entry_set_text(GTK_ENTRY (entry), "");
			break;
		case INDI_PROP_NUMBER:
			entry = g_object_get_data(G_OBJECT (value), "entry");
			valstr = gtk_entry_get_text(GTK_ENTRY (entry));
			elem->value.num.value = strtod(valstr, NULL);
			gtk_entry_set_text(GTK_ENTRY (entry), "");
			break;
		}			
	}
	indi_send(iprop, NULL);
}

static void indigui_send_switch_cb( GtkWidget *widget, struct indi_prop_t *iprop )
{
	GSList *gsl;
	GtkWidget *value;
	struct indi_elem_t *ielem;
	int elem_state;

	//We need to handle mutex conditions here
	for (gsl = iprop->elems; gsl; gsl = g_slist_next(gsl)) {
		struct indi_elem_t *elem = (struct indi_elem_t *)gsl->data;
		value = g_object_get_data(G_OBJECT (iprop->widget), elem->name);
		elem_state  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (value));
		if (widget == value) {
			// Don't try to process the mutex rules, just blindly follow orders as per the INDI API
			ielem = elem;
			elem->value.set = elem_state;
			break;
#if 0
			if (iprop->rule != INDI_RULE_ONEOFMANY || elem_state) {
				elem->value.set = elem_state;
			}
		} else if (iprop->rule != INDI_RULE_ANYOFMANY) {
			elem->value.set = 0;
#endif
		}
	}
	//indi_send(iprop, NULL);
	//The INDI docs say clients should evaluate switch rules, so only send the selected
	//Widget data, and the the server tell us how to set the buttons.
	//We compute the button state ourselves anyway in case the server takes time to respond
	if (ielem) {
		indi_send(iprop, ielem);
	}
}

static void indigui_create_text_widget(struct indi_prop_t *iprop, int num_props)
{
	int pos = 0;
	GtkWidget *value;
	GtkWidget *entry;
	GtkWidget *button;
	GSList *gsl;
	unsigned long signal;

	for (gsl = iprop->elems; gsl; gsl = g_slist_next(gsl), pos++) {
		struct indi_elem_t *elem = (struct indi_elem_t *)gsl->data;

		gtk_table_attach(GTK_TABLE (iprop->widget),
			gtk_label_new(elem->name),
			0, 1, pos, pos + 1,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

		value = gtk_label_new(elem->value.str);
		g_object_ref(G_OBJECT (value));
		g_object_set_data(G_OBJECT (iprop->widget), elem->name, value);
		gtk_table_attach(GTK_TABLE (iprop->widget),
			value,
			1, 2, pos, pos + 1,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

		if (iprop->permission != INDI_RO) {
			entry = gtk_entry_new();
			g_object_ref(G_OBJECT (entry));
			g_object_set_data(G_OBJECT (value), "entry", entry);
			gtk_table_attach(GTK_TABLE (iprop->widget),
				entry,
				2, 3, pos, pos + 1,
				GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
		}
	}
	if (iprop->permission != INDI_RO) {
		button = gtk_button_new_with_label("Set");
		signal = g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (indigui_send_cb), iprop);
		indi_prop_add_signal(iprop, button, signal);
		gtk_table_attach(GTK_TABLE (iprop->widget),
			button,
			3, 4, 0, num_props,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
	}
		
}

static void indigui_create_switch_widget(struct indi_prop_t *iprop, int num_props)
{
	int pos = 0;
	GtkWidget *button;
	GSList *gsl;
	unsigned long signal;

	for (gsl = iprop->elems; gsl; gsl = g_slist_next(gsl), pos++) {
		struct indi_elem_t *elem = (struct indi_elem_t *)gsl->data;

		gtk_table_attach(GTK_TABLE (iprop->widget),
			gtk_label_new(elem->name),
			0, 1, pos, pos + 1,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
		button = gtk_check_button_new();
		g_object_ref(G_OBJECT (button));
		g_object_set_data(G_OBJECT (iprop->widget), elem->name, button);
		gtk_table_attach(GTK_TABLE (iprop->widget),
			button,
			1, 2, pos, pos + 1,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
		if (elem->value.set) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (button), TRUE);
		}
		if (iprop->permission == INDI_RO) {
			gtk_widget_set_sensitive(button, FALSE);
		}
		signal = g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (indigui_send_switch_cb), iprop);
		indi_prop_add_signal(iprop, button, signal);
	}
}

static void indigui_create_number_widget(struct indi_prop_t *iprop, int num_props)
{
	int pos = 0;
	GtkWidget *value;
	GtkWidget *entry;
	GtkWidget *button;
	GSList *gsl;
	char val[80];
	unsigned long signal;

	for (gsl = iprop->elems; gsl; gsl = g_slist_next(gsl), pos++) {
		struct indi_elem_t *elem = (struct indi_elem_t *)gsl->data;

		gtk_table_attach(GTK_TABLE (iprop->widget),
			gtk_label_new(elem->name),
			0, 1, pos, pos + 1,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

		sprintf(val, "%f", elem->value.num.value);
		value = gtk_label_new(val);
		g_object_ref(G_OBJECT (value));
		g_object_set_data(G_OBJECT (iprop->widget), elem->name, value);
		gtk_table_attach(GTK_TABLE (iprop->widget),
			value,
			1, 2, pos, pos + 1,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

		if (iprop->permission != INDI_RO) {
			entry = gtk_entry_new();
			g_object_ref(G_OBJECT (entry));
			g_object_set_data(G_OBJECT (value), "entry", entry);
			gtk_table_attach(GTK_TABLE (iprop->widget),
				entry,
				2, 3, pos, pos + 1,
				GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
		}
	}
	if (iprop->permission != INDI_RO) {
		button = gtk_button_new_with_label("Set");
		signal = g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (indigui_send_cb), iprop);
		indi_prop_add_signal(iprop, button, signal);
		gtk_table_attach(GTK_TABLE (iprop->widget),
			button,
			3, 4, 0, num_props,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
	}
}

static void indigui_create_light_widget(struct indi_prop_t *iprop, int num_props)
{
}

static void indigui_create_blob_widget(struct indi_prop_t *iprop, int num_props)
{
}

static void indigui_build_prop_widget(struct indi_prop_t *iprop)
{
	GtkWidget *state_label;
	GtkWidget *name_label;
	int num_props;

 	num_props = g_slist_length(iprop->elems);
	iprop->widget = gtk_table_new(num_props, 4, FALSE);

	state_label = gtk_label_new(indi_get_string_from_state(iprop->state));
	g_object_ref(G_OBJECT (state_label));
	g_object_set_data(G_OBJECT (iprop->widget), "_state", state_label);

	name_label = gtk_label_new(iprop->name);
	g_object_ref(G_OBJECT (name_label));
	g_object_set_data(G_OBJECT (iprop->widget), "_name", name_label);

	switch (iprop->type) {
	case INDI_PROP_TEXT:
		indigui_create_text_widget(iprop, num_props);
		break;
	case INDI_PROP_SWITCH:
		indigui_create_switch_widget(iprop, num_props);
		break;
	case INDI_PROP_NUMBER:
		indigui_create_number_widget(iprop, num_props);
		break;
	case INDI_PROP_LIGHT:
		indigui_create_light_widget(iprop, num_props);
		break;
	case INDI_PROP_BLOB:
		indigui_create_blob_widget(iprop, num_props);
		break;
	}
}

void indigui_add_prop(struct indi_device_t *idev, const char *groupname, struct indi_prop_t *iprop)
{
	GtkWidget *page;
	int next_free_row;

	page = g_object_get_data(G_OBJECT (idev->window), groupname);
	if (! page) {
		page = gtk_table_new(1, 4, FALSE);
		gtk_notebook_append_page(GTK_NOTEBOOK (idev->window), page, gtk_label_new(groupname));
		g_object_set_data(G_OBJECT (page), "next-free-row", 0);
		g_object_set_data(G_OBJECT (idev->window), groupname, page);
	}
	next_free_row = (int) g_object_get_data(G_OBJECT (page), "next-free-row");

	indigui_build_prop_widget(iprop);
	gtk_table_attach(GTK_TABLE (page),
		GTK_WIDGET (g_object_get_data( G_OBJECT (iprop->widget), "_state")),
		0, 1, next_free_row, next_free_row + 1,
		GTK_FILL, GTK_FILL, 20, 10);
	gtk_table_attach(GTK_TABLE (page),
		GTK_WIDGET (g_object_get_data( G_OBJECT (iprop->widget), "_name")),
		1, 2, next_free_row, next_free_row + 1,
		GTK_FILL, GTK_FILL, 20, 10);
	gtk_table_attach(GTK_TABLE (page),
		iprop->widget,
		2, 3, next_free_row, next_free_row + 1,
		GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
	g_object_set_data(G_OBJECT (page), "next-free-row", (gpointer) (next_free_row + 1));
	gtk_widget_show_all(page);
}

void *indigui_create_window(void)
{
	GtkWidget *window;
	GtkWidget *notebook;
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	notebook = gtk_notebook_new();
	g_object_ref(G_OBJECT (notebook));
	g_object_set_data(G_OBJECT (window), "notebook", notebook);
	gtk_widget_show(notebook);
	gtk_container_add(GTK_CONTAINER (window), notebook);

	gtk_window_set_title (GTK_WINDOW (window), "INDI Options");
	gtk_window_set_default_size (GTK_WINDOW (window), 640, 400);
	return window;
}

static gboolean indigui_delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
	gtk_main_quit ();
	return FALSE;
}

void indigui_show_dialog(void *data, int modal)
{
    struct indi_t *indi = data;
    gtk_widget_show_all(indi->window);
    if(modal) {
        g_signal_connect (G_OBJECT (indi->window), "delete_event",
            G_CALLBACK (indigui_delete_event), NULL);
        gtk_main();
    }
}

#ifdef INDIMAIN

int main(int argc, char **argv) {
	struct indi_t *indi;

	gtk_init (&argc, &argv);
	indi = indi_init(NULL, NULL);
	g_signal_connect (G_OBJECT (indi->window), "delete_event",
		G_CALLBACK (indigui_delete_event), NULL);

	gtk_widget_show_all(indi->window);
	gtk_main();
	return 0;
}
#endif
