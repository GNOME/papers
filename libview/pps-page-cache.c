// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2009 Carlos Garcia Campos
 */

#include <config.h>

#include "pps-job-scheduler.h"
#include "pps-jobs.h"
#include "pps-page-cache.h"
#include <glib.h>

enum {
	PAGE_CACHED,
	LAST_SIGNAL
};

static guint pps_page_cache_signals[LAST_SIGNAL] = { 0 };

typedef struct _PpsPageCacheData {
	PpsJob *job;
	gboolean done : 1;
	PpsJobPageDataFlags flags;

	PpsMappingList *link_mapping;
	PpsMappingList *image_mapping;
	PpsMappingList *form_field_mapping;
	PpsMappingList *media_mapping;
	cairo_region_t *text_mapping;
	PpsRectangle *text_layout;
	guint text_layout_length;
	gchar *text;
	PangoAttrList *text_attrs;
	PangoLogAttr *text_log_attrs;
	gulong text_log_attrs_length;
} PpsPageCacheData;

struct _PpsPageCache {
	GObject parent;

	PpsDocument *document;
	PpsPageCacheData *page_list;
	gint n_pages;

	/* Current range */
	gint start_page;
	gint end_page;

	PpsJobPageDataFlags flags;
};

#define PPS_PAGE_DATA_FLAGS_DEFAULT (    \
    PPS_PAGE_DATA_INCLUDE_LINKS |        \
    PPS_PAGE_DATA_INCLUDE_TEXT_MAPPING | \
    PPS_PAGE_DATA_INCLUDE_IMAGES |       \
    PPS_PAGE_DATA_INCLUDE_FORMS |        \
    PPS_PAGE_DATA_INCLUDE_MEDIA)

#define PRE_CACHE_SIZE 1

static void job_page_data_finished_cb (PpsJob *job,
                                       PpsPageCache *cache);
static void job_page_data_cancelled_cb (PpsJob *job,
                                        PpsPageCacheData *data);

G_DEFINE_TYPE (PpsPageCache, pps_page_cache, G_TYPE_OBJECT)

static void
pps_page_cache_data_free (PpsPageCacheData *data)
{
	g_clear_object (&data->job);

	g_clear_pointer (&data->link_mapping, pps_mapping_list_unref);
	g_clear_pointer (&data->image_mapping, pps_mapping_list_unref);
	g_clear_pointer (&data->form_field_mapping, pps_mapping_list_unref);
	g_clear_pointer (&data->media_mapping, pps_mapping_list_unref);
	g_clear_pointer (&data->text_mapping, cairo_region_destroy);

	g_clear_pointer (&data->text_layout, g_free);
	data->text_layout_length = 0;

	g_clear_pointer (&data->text, g_free);
	g_clear_pointer (&data->text_attrs, pango_attr_list_unref);

	if (data->text_log_attrs) {
		g_clear_pointer (&data->text_log_attrs, g_free);
		data->text_log_attrs_length = 0;
	}
}

static void
pps_page_cache_finalize (GObject *object)
{
	PpsPageCache *cache = PPS_PAGE_CACHE (object);
	gint i;

	if (cache->page_list) {
		for (i = 0; i < cache->n_pages; i++) {
			PpsPageCacheData *data;

			data = &cache->page_list[i];

			if (data->job) {
				g_signal_handlers_disconnect_by_func (data->job,
				                                      G_CALLBACK (job_page_data_finished_cb),
				                                      cache);
				g_signal_handlers_disconnect_by_func (data->job,
				                                      G_CALLBACK (job_page_data_cancelled_cb),
				                                      data);
			}
			pps_page_cache_data_free (data);
		}

		g_clear_pointer (&cache->page_list, g_free);
		cache->n_pages = 0;
	}

	g_clear_object (&cache->document);

	G_OBJECT_CLASS (pps_page_cache_parent_class)->finalize (object);
}

static void
pps_page_cache_init (PpsPageCache *cache)
{
}

static void
pps_page_cache_class_init (PpsPageCacheClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->finalize = pps_page_cache_finalize;

	pps_page_cache_signals[PAGE_CACHED] =
	    g_signal_new ("page-cached",
	                  PPS_TYPE_PAGE_CACHE,
	                  G_SIGNAL_RUN_LAST,
	                  0,
	                  NULL, NULL,
	                  g_cclosure_marshal_VOID__INT,
	                  G_TYPE_NONE, 1, G_TYPE_INT);
}

static PpsJobPageDataFlags
pps_page_cache_get_flags_for_data (PpsPageCache *cache,
                                   PpsPageCacheData *data)
{
	PpsJobPageDataFlags flags = PPS_PAGE_DATA_INCLUDE_NONE;

	if (data->flags == cache->flags)
		return cache->flags;

	if (cache->flags & PPS_PAGE_DATA_INCLUDE_LINKS) {
		flags = (data->link_mapping) ? flags & ~PPS_PAGE_DATA_INCLUDE_LINKS : flags | PPS_PAGE_DATA_INCLUDE_LINKS;
	}

	if (cache->flags & PPS_PAGE_DATA_INCLUDE_IMAGES) {
		flags = (data->image_mapping) ? flags & ~PPS_PAGE_DATA_INCLUDE_IMAGES : flags | PPS_PAGE_DATA_INCLUDE_IMAGES;
	}

	if (cache->flags & PPS_PAGE_DATA_INCLUDE_FORMS) {
		flags = (data->form_field_mapping) ? flags & ~PPS_PAGE_DATA_INCLUDE_FORMS : flags | PPS_PAGE_DATA_INCLUDE_FORMS;
	}

	if (cache->flags & PPS_PAGE_DATA_INCLUDE_MEDIA) {
		flags = (data->media_mapping) ? flags & ~PPS_PAGE_DATA_INCLUDE_MEDIA : flags | PPS_PAGE_DATA_INCLUDE_MEDIA;
	}

	if (cache->flags & PPS_PAGE_DATA_INCLUDE_TEXT_MAPPING) {
		flags = (data->text_mapping) ? flags & ~PPS_PAGE_DATA_INCLUDE_TEXT_MAPPING : flags | PPS_PAGE_DATA_INCLUDE_TEXT_MAPPING;
	}

	if (cache->flags & PPS_PAGE_DATA_INCLUDE_TEXT) {
		flags = (data->text) ? flags & ~PPS_PAGE_DATA_INCLUDE_TEXT : flags | PPS_PAGE_DATA_INCLUDE_TEXT;
	}

	if (cache->flags & PPS_PAGE_DATA_INCLUDE_TEXT_LAYOUT) {
		flags = (data->text_layout_length > 0) ? flags & ~PPS_PAGE_DATA_INCLUDE_TEXT_LAYOUT : flags | PPS_PAGE_DATA_INCLUDE_TEXT_LAYOUT;
	}

	if (cache->flags & PPS_PAGE_DATA_INCLUDE_TEXT_ATTRS) {
		flags = (data->text_attrs) ? flags & ~PPS_PAGE_DATA_INCLUDE_TEXT_ATTRS : flags | PPS_PAGE_DATA_INCLUDE_TEXT_ATTRS;
	}

	if (cache->flags & PPS_PAGE_DATA_INCLUDE_TEXT_LOG_ATTRS) {
		flags = (data->text_log_attrs) ? flags & ~PPS_PAGE_DATA_INCLUDE_TEXT_LOG_ATTRS : flags | PPS_PAGE_DATA_INCLUDE_TEXT_LOG_ATTRS;
	}

	return flags;
}

PpsPageCache *
pps_page_cache_new (PpsDocument *document)
{
	PpsPageCache *cache;

	g_return_val_if_fail (PPS_IS_DOCUMENT (document), NULL);

	cache = PPS_PAGE_CACHE (g_object_new (PPS_TYPE_PAGE_CACHE, NULL));
	cache->document = g_object_ref (document);
	cache->n_pages = pps_document_get_n_pages (document);
	cache->flags = PPS_PAGE_DATA_FLAGS_DEFAULT;
	cache->page_list = g_new0 (PpsPageCacheData, cache->n_pages);

	return cache;
}

static void
job_page_data_finished_cb (PpsJob *job,
                           PpsPageCache *cache)
{
	PpsJobPageData *job_data = PPS_JOB_PAGE_DATA (job);
	PpsPageCacheData *data;

	data = &cache->page_list[job_data->page];

	if (job_data->flags & PPS_PAGE_DATA_INCLUDE_LINKS)
		data->link_mapping = job_data->link_mapping;
	if (job_data->flags & PPS_PAGE_DATA_INCLUDE_IMAGES)
		data->image_mapping = job_data->image_mapping;
	if (job_data->flags & PPS_PAGE_DATA_INCLUDE_FORMS)
		data->form_field_mapping = job_data->form_field_mapping;
	if (job_data->flags & PPS_PAGE_DATA_INCLUDE_MEDIA)
		data->media_mapping = job_data->media_mapping;
	if (job_data->flags & PPS_PAGE_DATA_INCLUDE_TEXT_MAPPING)
		data->text_mapping = job_data->text_mapping;
	if (job_data->flags & PPS_PAGE_DATA_INCLUDE_TEXT_LAYOUT) {
		data->text_layout = job_data->text_layout;
		data->text_layout_length = job_data->text_layout_length;
	}
	if (job_data->flags & PPS_PAGE_DATA_INCLUDE_TEXT)
		data->text = job_data->text;
	if (job_data->flags & PPS_PAGE_DATA_INCLUDE_TEXT_ATTRS)
		data->text_attrs = job_data->text_attrs;
	if (job_data->flags & PPS_PAGE_DATA_INCLUDE_TEXT_LOG_ATTRS) {
		data->text_log_attrs = job_data->text_log_attrs;
		data->text_log_attrs_length = job_data->text_log_attrs_length;
	}

	data->done = TRUE;

	g_clear_object (&data->job);

	g_signal_emit (cache, pps_page_cache_signals[PAGE_CACHED], 0, job_data->page);
}

static void
job_page_data_cancelled_cb (PpsJob *job,
                            PpsPageCacheData *data)
{
	g_clear_object (&data->job);
}

static void
pps_page_cache_schedule_job_if_needed (PpsPageCache *cache,
                                       gint page)
{
	PpsPageCacheData *data = &cache->page_list[page];
	PpsJobPageDataFlags flags;

	if (data->flags == cache->flags && (data->done || data->job))
		return;

	if (data->job)
		pps_job_cancel (data->job);

	flags = pps_page_cache_get_flags_for_data (cache, data);

	data->flags = cache->flags;
	data->job = pps_job_page_data_new (cache->document, page, flags);
	g_signal_connect (data->job, "finished",
	                  G_CALLBACK (job_page_data_finished_cb),
	                  cache);
	g_signal_connect (data->job, "cancelled",
	                  G_CALLBACK (job_page_data_cancelled_cb),
	                  data);
	pps_job_scheduler_push_job (data->job, PPS_JOB_PRIORITY_NONE);
}

void
pps_page_cache_set_page_range (PpsPageCache *cache,
                               gint start,
                               gint end)
{
	gint i;
	gint pages_to_pre_cache;

	if (cache->flags == PPS_PAGE_DATA_INCLUDE_NONE)
		return;

	for (i = start; i <= end; i++)
		pps_page_cache_schedule_job_if_needed (cache, i);

	cache->start_page = start;
	cache->end_page = end;

	i = 1;
	pages_to_pre_cache = PRE_CACHE_SIZE * 2;
	while ((start - i > 0) || (end + i < cache->n_pages)) {
		if (end + i < cache->n_pages) {
			pps_page_cache_schedule_job_if_needed (cache, end + i);
			if (--pages_to_pre_cache == 0)
				break;
		}

		if (start - i > 0) {
			pps_page_cache_schedule_job_if_needed (cache, start - i);
			if (--pages_to_pre_cache == 0)
				break;
		}
		i++;
	}
}

PpsJobPageDataFlags
pps_page_cache_get_flags (PpsPageCache *cache)
{
	return cache->flags;
}

void
pps_page_cache_set_flags (PpsPageCache *cache,
                          PpsJobPageDataFlags flags)
{
	if (cache->flags == flags)
		return;

	cache->flags = flags;

	/* Update the current range for new flags */
	pps_page_cache_set_page_range (cache, cache->start_page, cache->end_page);
}

PpsMappingList *
pps_page_cache_get_link_mapping (PpsPageCache *cache,
                                 gint page)
{
	PpsPageCacheData *data;

	g_return_val_if_fail (PPS_IS_PAGE_CACHE (cache), NULL);
	g_return_val_if_fail (page >= 0 && page < cache->n_pages, NULL);

	if (!(cache->flags & PPS_PAGE_DATA_INCLUDE_LINKS))
		return NULL;

	data = &cache->page_list[page];
	if (data->done)
		return data->link_mapping;

	if (data->job)
		return PPS_JOB_PAGE_DATA (data->job)->link_mapping;

	return data->link_mapping;
}

PpsMappingList *
pps_page_cache_get_image_mapping (PpsPageCache *cache,
                                  gint page)
{
	PpsPageCacheData *data;

	g_return_val_if_fail (PPS_IS_PAGE_CACHE (cache), NULL);
	g_return_val_if_fail (page >= 0 && page < cache->n_pages, NULL);

	if (!(cache->flags & PPS_PAGE_DATA_INCLUDE_IMAGES))
		return NULL;

	data = &cache->page_list[page];
	if (data->done)
		return data->image_mapping;

	if (data->job)
		return PPS_JOB_PAGE_DATA (data->job)->image_mapping;

	return data->image_mapping;
}

PpsMappingList *
pps_page_cache_get_form_field_mapping (PpsPageCache *cache,
                                       gint page)
{
	PpsPageCacheData *data;

	g_return_val_if_fail (PPS_IS_PAGE_CACHE (cache), NULL);
	g_return_val_if_fail (page >= 0 && page < cache->n_pages, NULL);

	if (!(cache->flags & PPS_PAGE_DATA_INCLUDE_FORMS))
		return NULL;

	data = &cache->page_list[page];
	if (data->done)
		return data->form_field_mapping;

	if (data->job)
		return PPS_JOB_PAGE_DATA (data->job)->form_field_mapping;

	return data->form_field_mapping;
}

PpsMappingList *
pps_page_cache_get_media_mapping (PpsPageCache *cache,
                                  gint page)
{
	PpsPageCacheData *data;

	g_return_val_if_fail (PPS_IS_PAGE_CACHE (cache), NULL);
	g_return_val_if_fail (page >= 0 && page < cache->n_pages, NULL);

	if (!(cache->flags & PPS_PAGE_DATA_INCLUDE_MEDIA))
		return NULL;

	data = &cache->page_list[page];
	if (data->done)
		return data->media_mapping;

	if (data->job)
		return PPS_JOB_PAGE_DATA (data->job)->media_mapping;

	return data->media_mapping;
}

cairo_region_t *
pps_page_cache_get_text_mapping (PpsPageCache *cache,
                                 gint page)
{
	PpsPageCacheData *data;

	g_return_val_if_fail (PPS_IS_PAGE_CACHE (cache), NULL);
	g_return_val_if_fail (page >= 0 && page < cache->n_pages, NULL);

	if (!(cache->flags & PPS_PAGE_DATA_INCLUDE_TEXT_MAPPING))
		return NULL;

	data = &cache->page_list[page];
	if (data->done)
		return data->text_mapping;

	if (data->job)
		return PPS_JOB_PAGE_DATA (data->job)->text_mapping;

	return data->text_mapping;
}

const gchar *
pps_page_cache_get_text (PpsPageCache *cache,
                         gint page)
{
	PpsPageCacheData *data;

	g_return_val_if_fail (PPS_IS_PAGE_CACHE (cache), NULL);
	g_return_val_if_fail (page >= 0 && page < cache->n_pages, NULL);

	if (!(cache->flags & PPS_PAGE_DATA_INCLUDE_TEXT))
		return NULL;

	data = &cache->page_list[page];
	if (data->done)
		return data->text;

	if (data->job)
		return PPS_JOB_PAGE_DATA (data->job)->text;

	return data->text;
}

gboolean
pps_page_cache_get_text_layout (PpsPageCache *cache,
                                gint page,
                                PpsRectangle **areas,
                                guint *n_areas)
{
	PpsPageCacheData *data;

	g_return_val_if_fail (PPS_IS_PAGE_CACHE (cache), FALSE);
	g_return_val_if_fail (page >= 0 && page < cache->n_pages, FALSE);

	if (!(cache->flags & PPS_PAGE_DATA_INCLUDE_TEXT_LAYOUT))
		return FALSE;

	data = &cache->page_list[page];
	if (data->done) {
		*areas = data->text_layout;
		*n_areas = data->text_layout_length;

		return TRUE;
	}

	if (data->job) {
		*areas = PPS_JOB_PAGE_DATA (data->job)->text_layout;
		*n_areas = PPS_JOB_PAGE_DATA (data->job)->text_layout_length;

		return TRUE;
	}

	return FALSE;
}

/**
 * pps_page_cache_get_text_attrs:
 * @cache: a #PpsPageCache
 * @page:
 *
 * FIXME
 */
PangoAttrList *
pps_page_cache_get_text_attrs (PpsPageCache *cache,
                               gint page)
{
	PpsPageCacheData *data;

	g_return_val_if_fail (PPS_IS_PAGE_CACHE (cache), NULL);
	g_return_val_if_fail (page >= 0 && page < cache->n_pages, NULL);

	if (!(cache->flags & PPS_PAGE_DATA_INCLUDE_TEXT_ATTRS))
		return NULL;

	data = &cache->page_list[page];
	if (data->done)
		return data->text_attrs;

	if (data->job)
		return PPS_JOB_PAGE_DATA (data->job)->text_attrs;

	return data->text_attrs;
}

/**
 * pps_page_cache_get_text_log_attrs:
 * @cache: a #PpsPageCache
 * @page:
 * @log_attrs: (out) (transfer full) (array length=n_attrs):
 * @n_attrs: (out):
 *
 * FIXME
 *
 * Returns: %TRUE on success with @log_attrs filled in, %FALSE otherwise
 */
gboolean
pps_page_cache_get_text_log_attrs (PpsPageCache *cache,
                                   gint page,
                                   PangoLogAttr **log_attrs,
                                   gulong *n_attrs)
{
	PpsPageCacheData *data;

	g_return_val_if_fail (PPS_IS_PAGE_CACHE (cache), FALSE);
	g_return_val_if_fail (page >= 0 && page < cache->n_pages, FALSE);

	if (!(cache->flags & PPS_PAGE_DATA_INCLUDE_TEXT_LOG_ATTRS))
		return FALSE;

	data = &cache->page_list[page];
	if (data->done) {
		*log_attrs = data->text_log_attrs;
		*n_attrs = data->text_log_attrs_length;

		return TRUE;
	}

	if (data->job) {
		*log_attrs = PPS_JOB_PAGE_DATA (data->job)->text_log_attrs;
		*n_attrs = PPS_JOB_PAGE_DATA (data->job)->text_log_attrs_length;

		return TRUE;
	}

	return FALSE;
}

void
pps_page_cache_ensure_page (PpsPageCache *cache,
                            gint page)
{
	g_return_if_fail (PPS_IS_PAGE_CACHE (cache));
	g_return_if_fail (page >= 0 && page < cache->n_pages);

	pps_page_cache_schedule_job_if_needed (cache, page);
}

gboolean
pps_page_cache_is_page_cached (PpsPageCache *cache,
                               gint page)
{
	PpsPageCacheData *data;

	g_return_val_if_fail (PPS_IS_PAGE_CACHE (cache), FALSE);
	g_return_val_if_fail (page >= 0 && page < cache->n_pages, FALSE);

	data = &cache->page_list[page];

	return data->done;
}
