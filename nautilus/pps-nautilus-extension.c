// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2000, 2001 Eazel Inc.
 * Copyright (C) 2003  Andrew Sobala <aes@gnome.org>
 * Copyright (C) 2005  Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2024  Qiu Wenbo <qiuwenbo@kylinos.com.cn>
 *
 * The Papers project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Papers. This
 * permission are above and beyond the permissions granted by the GPL license
 * Papers is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exemption clause.
 * See license_change file for details.
 *
 */

#include "pps-nautilus-extension.h"

#include <nautilus-extension.h>

static void properties_model_provider_iface_init (NautilusPropertiesModelProviderInterface *iface);

struct _PpsDocumentPropertiesModelProvider {
	GObject parent_instance;
};

static GList *pps_properties_get_models (NautilusPropertiesModelProvider *provider, GList *files);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (PpsDocumentPropertiesModelProvider,
                                pps_document_properties_model_provider,
                                G_TYPE_OBJECT,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (NAUTILUS_TYPE_PROPERTIES_MODEL_PROVIDER,
                                                               properties_model_provider_iface_init))

static GListModel *
build_properties (PpsDocument *document)
{
	g_autoptr (PpsDocumentInfo) info = pps_document_get_info (document);
	GListStore *model = g_list_store_new (NAUTILUS_TYPE_PROPERTIES_ITEM);
	const char *uri = pps_document_get_uri (document);
	GDateTime *datetime = NULL;
	char *text;

#define SET_PROPERTY(p, value)                                                                                       \
	do {                                                                                                         \
		g_list_store_append (model,                                                                          \
		                     nautilus_properties_item_new (_ (properties_info[p##_PROPERTY].label), value)); \
	} while (0)

#define FIELD_SET_PROPERTY(p, value)                     \
	if (info->fields_mask & PPS_DOCUMENT_INFO_##p) { \
		SET_PROPERTY (p, value);                 \
	}

	FIELD_SET_PROPERTY (TITLE, info->title);
	SET_PROPERTY (URI, uri);
	FIELD_SET_PROPERTY (SUBJECT, info->subject);
	FIELD_SET_PROPERTY (AUTHOR, info->author);
	FIELD_SET_PROPERTY (KEYWORDS, info->keywords);
	FIELD_SET_PROPERTY (PRODUCER, info->producer);
	FIELD_SET_PROPERTY (CREATOR, info->creator);

	datetime = pps_document_info_get_created_datetime (info);
	if (datetime != NULL) {
		text = pps_document_misc_format_datetime (datetime);
		SET_PROPERTY (CREATION_DATE, text);
		g_free (text);
	} else {
		SET_PROPERTY (CREATION_DATE, NULL);
	}
	datetime = pps_document_info_get_modified_datetime (info);
	if (datetime != NULL) {
		text = pps_document_misc_format_datetime (datetime);
		SET_PROPERTY (MOD_DATE, text);
		g_free (text);
	} else {
		SET_PROPERTY (MOD_DATE, NULL);
	}

	FIELD_SET_PROPERTY (FORMAT, info->format);

	if (info->fields_mask & PPS_DOCUMENT_INFO_N_PAGES) {
		text = g_strdup_printf ("%d", info->n_pages);
		SET_PROPERTY (N_PAGES, text);
		g_free (text);
	}
	FIELD_SET_PROPERTY (LINEARIZED, info->linearized);
	FIELD_SET_PROPERTY (SECURITY, info->security);

	if (info->fields_mask & PPS_DOCUMENT_INFO_PAPER_SIZE) {
		text = pps_document_info_regular_paper_size (info);
		SET_PROPERTY (PAPER_SIZE, text);
		g_free (text);
	}

	if (info->fields_mask & PPS_DOCUMENT_INFO_CONTAINS_JS) {
		if (info->contains_js == PPS_DOCUMENT_CONTAINS_JS_YES) {
			text = _ ("Yes");
		} else if (info->contains_js == PPS_DOCUMENT_CONTAINS_JS_NO) {
			text = _ ("No");
		} else {
			text = _ ("Unknown");
		}
		SET_PROPERTY (CONTAINS_JS, text);
	}

	if (pps_document_get_size (document)) {
		text = g_format_size (pps_document_get_size (document));
		SET_PROPERTY (FILE_SIZE, text);
		g_free (text);
	}

	return G_LIST_MODEL (model);
#undef SET_PROPERTY
#undef FIELD_SET_PROPERTY
}

static GList *
pps_properties_get_models (NautilusPropertiesModelProvider *provider,
                           GList *files)
{
	GError *error = NULL;
	PpsDocument *document = NULL;
	GList *models = NULL;
	NautilusFileInfo *file;
	gchar *uri = NULL;
	NautilusPropertiesModel *properties_group;

	/* only add properties page if a single file is selected */
	if (files == NULL || files->next != NULL)
		goto end;
	file = files->data;

	/* okay, make the page */
	uri = nautilus_file_info_get_uri (file);

	document = pps_document_factory_get_document (uri, &error);
	if (!document)
		goto end;

	pps_document_load (document, uri, &error);
	if (error) {
		g_error_free (error);
		goto end;
	}

	properties_group = nautilus_properties_model_new (_ ("Document Properties"), build_properties (document));

	models = g_list_prepend (models, properties_group);
end:
	g_free (uri);
	g_clear_pointer (&error, g_error_free);
	g_clear_object (&document);

	return models;
}

static void
pps_document_properties_model_provider_init (PpsDocumentPropertiesModelProvider *self)
{
}

static void
pps_document_properties_model_provider_class_init (PpsDocumentPropertiesModelProviderClass *klass)
{
}

static void
pps_document_properties_model_provider_class_finalize (PpsDocumentPropertiesModelProviderClass *klass)
{
}

static void
properties_model_provider_iface_init (NautilusPropertiesModelProviderInterface *iface)
{
	iface->get_models = pps_properties_get_models;
}

/* --- extension interface --- */

PPS_PUBLIC
void
nautilus_module_initialize (GTypeModule *module)
{
	pps_document_properties_model_provider_register_type (module);
	pps_init ();
}

PPS_PUBLIC
void
nautilus_module_shutdown (void)
{
	pps_shutdown ();
}

PPS_PUBLIC
void
nautilus_module_list_types (const GType **types,
                            int *num_types)
{
	static GType type_list[1];

	type_list[0] = PPS_TYPE_DOCUMENT_PROPERTIES_MODEL_PROVIDER;
	*types = type_list;
	*num_types = G_N_ELEMENTS (type_list);
}
