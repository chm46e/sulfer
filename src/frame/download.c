#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "frame.h"
#include "util/util.h"

static GtkBuilder *builder;
static WebKitDownload *download_last = 0;

static int download_filename_choose(WebKitDownload *d);
static void download_destination_created(WebKitDownload *, char *dest);
static void download_progress_changed(WebKitDownload *d);
static void download_finished(WebKitDownload *);

void download_started(WebKitWebContext *, WebKitDownload *d)
{
	builder = builder_get();

	g_signal_connect(G_OBJECT(d), "decide-destination",
		G_CALLBACK(download_filename_choose), NULL);
	g_signal_connect(G_OBJECT(d), "created-destination",
		G_CALLBACK(download_destination_created), NULL);
	g_signal_connect(G_OBJECT(d), "notify::estimated-progress",
		G_CALLBACK(download_progress_changed), NULL);
	g_signal_connect(G_OBJECT(d), "finished",
		G_CALLBACK(download_finished), NULL);

	download_last = d;
}

static int download_filename_choose(WebKitDownload *d)
{
	GtkWidget *chooser, *btc, *btd;
	int ret;
	char *uri;

	chooser = gtk_file_chooser_dialog_new("Select Download Location",
		GTK_WINDOW(current_frame_get()->win), GTK_FILE_CHOOSER_ACTION_SAVE,
		NULL, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser), 0);

	/*
	 * doesn't get it for some reason:
	 * soup_message_headers_get_content_disposition(
	 *     webkit_uri_response_get_http_headers(webkit_download_get_response(d)),
	 *     dest, NULL));
	 * gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), dest);
	 */

	btc = gtk_dialog_add_button(GTK_DIALOG(chooser), "Cancel", 0);
	btd = gtk_dialog_add_button(GTK_DIALOG(chooser), "Download", 1);
	gtk_button_set_relief(GTK_BUTTON(btc), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(btd), GTK_RELIEF_NONE);

	ret = gtk_dialog_run(GTK_DIALOG(chooser));

	switch (ret) {
	case 1:
		uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(chooser));
		gtk_widget_destroy(chooser);
		break;
	case 0:
	default:
		webkit_download_cancel(d);
		gtk_widget_destroy(chooser);
		return 0;
	}

	webkit_download_set_destination(d, uri);
	return 1;
}

static void download_destination_created(WebKitDownload *, char *dest)
{
	char *name;

	name = ecalloc(21, sizeof(char));
	strncpy(name, g_filename_display_basename(dest), 20);

	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "menu_download_label")),
		name);

	efree(name);
}

static void download_progress_changed(WebKitDownload *d)
{
	char progress[6];

	sprintf(progress, "%i%%:", (int)(webkit_download_get_estimated_progress(d)*100));

	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder,
		"menu_download_progress_label")), progress);
}

static void download_finished(WebKitDownload *)
{
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder,
		"menu_download_progress_label")), "100%:");
	download_last = 0;
}

void download_quit(GtkWidget *)
{
	if (download_last != 0) {
		webkit_download_cancel(download_last);
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder,
			"menu_download_progress_label")), "quit:");
	}
}
