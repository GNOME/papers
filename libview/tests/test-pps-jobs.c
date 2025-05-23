#include <glib.h>
#include <pps-document.h>
#include <pps-init.h>
#include <pps-job.h>
#include <pps-jobs.h>

static void
get_annots (void)
{
	PpsDocument *document;
	PpsJob *job = pps_job_load_new ();
	gchar *file_path = SRCDIR "/utf16le-annot.pdf";
	GFile *file = NULL;
	gchar *uri = NULL;
	gboolean hasBackend;
	gint n_pages;
	GList *annots;
	PpsPage *page;

	file = g_file_new_for_path (file_path);
	uri = g_file_get_uri (file);
	hasBackend = pps_init ();

	g_assert_nonnull (file);
	g_assert_nonnull (uri);
	g_assert_nonnull (job);
	g_assert_true (hasBackend);

	pps_job_load_set_uri (PPS_JOB_LOAD (job), uri);

	pps_job_run (job);

	document = pps_job_load_get_loaded_document (PPS_JOB_LOAD (job));
	n_pages = pps_document_get_n_pages (document);

	g_assert_nonnull (document);
	g_assert (n_pages == 1);

	page = pps_document_get_page (document, 0);
	annots = pps_document_annotations_get_annotations (PPS_DOCUMENT_ANNOTATIONS (document),
	                                                   page);

	g_assert (g_list_length (annots) == 2);
}

static void
load_encrypted (void)
{
	PpsDocument *document;
	PpsJob *job = pps_job_load_new ();
	gchar *file_path = SRCDIR "/PasswordEncrypted.pdf";
	GFile *file = NULL;
	gchar *uri = NULL;
	gboolean hasBackend;
	gint n_pages;

	file = g_file_new_for_path (file_path);
	uri = g_file_get_uri (file);
	hasBackend = pps_init ();

	g_assert_nonnull (file);
	g_assert_nonnull (uri);
	g_assert_nonnull (job);
	g_assert_true (hasBackend);

	pps_job_load_set_uri (PPS_JOB_LOAD (job), uri);

	pps_job_run (job);

	pps_job_reset (job);

	pps_job_load_set_password (PPS_JOB_LOAD (job), "password");

	pps_job_run (job);

	document = pps_job_load_get_loaded_document (PPS_JOB_LOAD (job));
	n_pages = pps_document_get_n_pages (document);

	g_assert_nonnull (document);
	g_assert (n_pages == 1);
}

gint
main (gint argc,
      gchar *argv[])
{
	g_test_init (&argc, &argv, NULL);
	g_test_add_func ("/libview-jobs/get_annots", get_annots);
	g_test_add_func ("/libview-jobs/load_encrypted", load_encrypted);
	return g_test_run ();
}
