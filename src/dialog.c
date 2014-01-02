/*
 * viking -- GPS Data and Topo Analyzer, Explorer, and Manager
 *
 * Copyright (C) 2003-2005, Evan Battaglia <gtoevan@gmx.net>
 * Copyright (C) 2008, Hein Ragas <viking@ragas.nl>
 * Copyright (C) 2010-2014, Rob Norris <rw_norris@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "viking.h"
#include "thumbnails.h"
#include "garminsymbols.h"
#include "degrees_converters.h"
#include "authors.h"
#include "documenters.h"
#include "vikgoto.h"
#include "util.h"
#include "geotag_exif.h"

#include <glib/gi18n.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

void a_dialog_msg ( GtkWindow *parent, gint type, const gchar *info, const gchar *extra )
{
  GtkWidget *msgbox = gtk_message_dialog_new ( parent, GTK_DIALOG_DESTROY_WITH_PARENT, type, GTK_BUTTONS_OK, info, extra );
  gtk_dialog_run ( GTK_DIALOG(msgbox) );
  gtk_widget_destroy ( msgbox );
}

gboolean a_dialog_goto_latlon ( GtkWindow *parent, struct LatLon *ll, const struct LatLon *old )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Go to Lat/Lon"),
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
  GtkWidget *latlabel, *lonlabel;
  GtkWidget *lat, *lon;
  gchar *tmp_lat, *tmp_lon;

  latlabel = gtk_label_new (_("Latitude:"));
  lat = gtk_entry_new ();
  tmp_lat = g_strdup_printf ( "%f", old->lat );
  gtk_entry_set_text ( GTK_ENTRY(lat), tmp_lat );
  g_free ( tmp_lat );

  lonlabel = gtk_label_new (_("Longitude:"));
  lon = gtk_entry_new ();
  tmp_lon = g_strdup_printf ( "%f", old->lon );
  gtk_entry_set_text ( GTK_ENTRY(lon), tmp_lon );
  g_free ( tmp_lon );

  gtk_widget_show ( latlabel );
  gtk_widget_show ( lonlabel );
  gtk_widget_show ( lat );
  gtk_widget_show ( lon );

  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), latlabel,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), lat, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), lonlabel,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), lon,  FALSE, FALSE, 0);

  // 'ok' when press return in the entry
  g_signal_connect_swapped (lat, "activate", G_CALLBACK(a_dialog_response_accept), dialog);
  g_signal_connect_swapped (lon, "activate", G_CALLBACK(a_dialog_response_accept), dialog);

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
  {
    ll->lat = convert_dms_to_dec ( gtk_entry_get_text ( GTK_ENTRY(lat) ) );
    ll->lon = convert_dms_to_dec ( gtk_entry_get_text ( GTK_ENTRY(lon) ) );
    gtk_widget_destroy ( dialog );
    return TRUE;
  }

  gtk_widget_destroy ( dialog );
  return FALSE;
}

gboolean a_dialog_goto_utm ( GtkWindow *parent, struct UTM *utm, const struct UTM *old )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Go to UTM"),
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
  GtkWidget *norlabel, *easlabel, *nor, *eas;
  GtkWidget *zonehbox, *zonespin, *letterentry;
  gchar *tmp_eas, *tmp_nor;
  gchar tmp_letter[2];

  norlabel = gtk_label_new (_("Northing:"));
  nor = gtk_entry_new ();
  tmp_nor = g_strdup_printf("%ld", (long) old->northing );
  gtk_entry_set_text ( GTK_ENTRY(nor), tmp_nor );
  g_free ( tmp_nor );

  easlabel = gtk_label_new (_("Easting:"));
  eas = gtk_entry_new ();
  tmp_eas = g_strdup_printf("%ld", (long) old->easting );
  gtk_entry_set_text ( GTK_ENTRY(eas), tmp_eas );
  g_free ( tmp_eas );

  zonehbox = gtk_hbox_new ( FALSE, 0 );
  gtk_box_pack_start ( GTK_BOX(zonehbox), gtk_label_new ( _("Zone:") ), FALSE, FALSE, 5 );
  zonespin = gtk_spin_button_new ( (GtkAdjustment *) gtk_adjustment_new ( old->zone, 1, 60, 1, 5, 0 ), 1, 0 );
  gtk_box_pack_start ( GTK_BOX(zonehbox), zonespin, TRUE, TRUE, 5 );
  gtk_box_pack_start ( GTK_BOX(zonehbox), gtk_label_new ( _("Letter:") ), FALSE, FALSE, 5 );
  letterentry = gtk_entry_new ();
  gtk_entry_set_max_length ( GTK_ENTRY(letterentry), 1 );
  gtk_entry_set_width_chars ( GTK_ENTRY(letterentry), 2 );
  tmp_letter[0] = old->letter;
  tmp_letter[1] = '\0';
  gtk_entry_set_text ( GTK_ENTRY(letterentry), tmp_letter );
  gtk_box_pack_start ( GTK_BOX(zonehbox), letterentry, FALSE, FALSE, 5 );

  gtk_widget_show ( norlabel );
  gtk_widget_show ( easlabel );
  gtk_widget_show ( nor );
  gtk_widget_show ( eas );

  gtk_widget_show_all ( zonehbox );

  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), norlabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), nor, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), easlabel,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), eas,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), zonehbox,  FALSE, FALSE, 0);

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
  {
    const gchar *letter;
    utm->northing = atof ( gtk_entry_get_text ( GTK_ENTRY(nor) ) );
    utm->easting = atof ( gtk_entry_get_text ( GTK_ENTRY(eas) ) );
    utm->zone = gtk_spin_button_get_value_as_int ( GTK_SPIN_BUTTON(zonespin) );
    letter = gtk_entry_get_text ( GTK_ENTRY(letterentry) );
    if (*letter)
       utm->letter = toupper(*letter);
    gtk_widget_destroy ( dialog );
    return TRUE;
  }

  gtk_widget_destroy ( dialog );
  return FALSE;
}

void a_dialog_response_accept ( GtkDialog *dialog )
{
  gtk_dialog_response ( dialog, GTK_RESPONSE_ACCEPT );
}

static void symbol_entry_changed_cb(GtkWidget *combo, GtkListStore *store)
{
  GtkTreeIter iter;
  gchar *sym;

  if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter))
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, (void *)&sym, -1 );
  /* Note: symm is NULL when "(none)" is select (first cell is empty) */
  gtk_widget_set_tooltip_text(combo, sym);
  g_free(sym);
}

/* Specify if a new waypoint or not */
/* If a new waypoint then it uses the default_name for the suggested name allowing the user to change it.
    The name to use is returned
 */
/* todo: less on this side, like add track */
gchar *a_dialog_waypoint ( GtkWindow *parent, gchar *default_name, VikTrwLayer *vtl, VikWaypoint *wp, VikCoordMode coord_mode, gboolean is_new, gboolean *updated )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Waypoint Properties"),
                                                   parent,
                                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_STOCK_CANCEL,
                                                   GTK_RESPONSE_REJECT,
                                                   GTK_STOCK_OK,
                                                   GTK_RESPONSE_ACCEPT,
                                                   NULL);
  struct LatLon ll;
  GtkWidget *latlabel, *lonlabel, *namelabel, *latentry, *lonentry, *altentry, *altlabel, *nameentry=NULL;
  GtkWidget *commentlabel, *commententry, *descriptionlabel, *descriptionentry, *imagelabel, *imageentry, *symbollabel, *symbolentry;
  GtkWidget *timelabel = NULL;
  GtkWidget *timevaluelabel = NULL; // No editing of time allowed ATM
  GtkWidget *hasGeotagCB = NULL;
  GtkWidget *consistentGeotagCB = NULL;
  GtkListStore *store;

  gchar *lat, *lon, *alt;

  vik_coord_to_latlon ( &(wp->coord), &ll );

  lat = g_strdup_printf ( "%f", ll.lat );
  lon = g_strdup_printf ( "%f", ll.lon );
  vik_units_height_t height_units = a_vik_get_units_height ();
  switch (height_units) {
  case VIK_UNITS_HEIGHT_METRES:
    alt = g_strdup_printf ( "%f", wp->altitude );
    break;
  case VIK_UNITS_HEIGHT_FEET:
    alt = g_strdup_printf ( "%f", VIK_METERS_TO_FEET(wp->altitude) );
    break;
  default:
    alt = g_strdup_printf ( "%f", wp->altitude );
    g_critical("Houston, we've had a problem. height=%d", height_units);
  }

  *updated = FALSE;

  namelabel = gtk_label_new (_("Name:"));
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), namelabel, FALSE, FALSE, 0);
  // Name is now always changeable
  nameentry = gtk_entry_new ();
  if ( default_name )
    gtk_entry_set_text( GTK_ENTRY(nameentry), default_name );
  g_signal_connect_swapped ( nameentry, "activate", G_CALLBACK(a_dialog_response_accept), GTK_DIALOG(dialog) );
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), nameentry, FALSE, FALSE, 0);

  latlabel = gtk_label_new (_("Latitude:"));
  latentry = gtk_entry_new ();
  gtk_entry_set_text ( GTK_ENTRY(latentry), lat );
  g_free ( lat );

  lonlabel = gtk_label_new (_("Longitude:"));
  lonentry = gtk_entry_new ();
  gtk_entry_set_text ( GTK_ENTRY(lonentry), lon );
  g_free ( lon );

  altlabel = gtk_label_new (_("Altitude:"));
  altentry = gtk_entry_new ();
  gtk_entry_set_text ( GTK_ENTRY(altentry), alt );
  g_free ( alt );

  commentlabel = gtk_label_new (_("Comment:"));
  commententry = gtk_entry_new ();
  gchar *cmt =  NULL;
  // Auto put in some kind of 'name' as a comment if one previously 'goto'ed this exact location
  cmt = a_vik_goto_get_search_string_for_this_place(VIK_WINDOW(parent));
  if (cmt)
    gtk_entry_set_text(GTK_ENTRY(commententry), cmt);

  descriptionlabel = gtk_label_new (_("Description:"));
  descriptionentry = gtk_entry_new ();

  imagelabel = gtk_label_new (_("Image:"));
  imageentry = vik_file_entry_new (GTK_FILE_CHOOSER_ACTION_OPEN, VF_FILTER_IMAGE);

  {
    GtkCellRenderer *r;
    symbollabel = gtk_label_new (_("Symbol:"));
    GtkTreeIter iter;

    store = gtk_list_store_new(3, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING);
    symbolentry = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(symbolentry), 6);

    g_signal_connect(symbolentry, "changed", G_CALLBACK(symbol_entry_changed_cb), store);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, NULL, 1, NULL, 2, _("(none)"), -1);
    a_populate_sym_list(store);

    r = gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (symbolentry), r, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (symbolentry), r, "pixbuf", 1, NULL);

    r = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (symbolentry), r, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (symbolentry), r, "text", 2, NULL);

    if ( !is_new && wp->symbol ) {
      gboolean ok;
      gchar *sym;
      for (ok = gtk_tree_model_get_iter_first ( GTK_TREE_MODEL(store), &iter ); ok; ok = gtk_tree_model_iter_next ( GTK_TREE_MODEL(store), &iter)) {
	gtk_tree_model_get ( GTK_TREE_MODEL(store), &iter, 0, (void *)&sym, -1 );
	if (sym && !strcmp(sym, wp->symbol)) {
	  g_free(sym);
	  break;
	} else {
	  g_free(sym);
	}
      }
      // Ensure is it a valid symbol in the given symbol set (large vs small)
      // Not all symbols are available in both
      // The check prevents a Gtk Critical message
      if ( iter.stamp )
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(symbolentry), &iter);
    }
  }

  if ( !is_new && wp->comment )
    gtk_entry_set_text ( GTK_ENTRY(commententry), wp->comment );

  if ( !is_new && wp->description )
    gtk_entry_set_text ( GTK_ENTRY(descriptionentry), wp->description );

  if ( !is_new && wp->image ) {
    vik_file_entry_set_filename ( VIK_FILE_ENTRY(imageentry), wp->image );

    // Geotag Info [readonly]
    hasGeotagCB = gtk_check_button_new_with_label ( _("Has Geotag") );
    gtk_widget_set_sensitive ( hasGeotagCB, FALSE );
    gboolean hasGeotag;
    gchar *ignore = a_geotag_get_exif_date_from_file ( wp->image, &hasGeotag );
    g_free ( ignore );
    gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON(hasGeotagCB), hasGeotag );

    consistentGeotagCB = gtk_check_button_new_with_label ( _("Consistent Position") );
    gtk_widget_set_sensitive ( consistentGeotagCB, FALSE );
    if ( hasGeotag ) {
      struct LatLon ll = a_geotag_get_position ( wp->image );
      VikCoord coord;
      vik_coord_load_from_latlon ( &coord, coord_mode, &ll );
      gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON(consistentGeotagCB), vik_coord_equals(&coord, &wp->coord) );
    }
  }

  if ( !is_new && wp->has_timestamp ) {
    gchar tmp_str[64];
    timelabel = gtk_label_new ( _("Time:") );
    timevaluelabel = gtk_label_new ( NULL );
    strftime ( tmp_str, sizeof(tmp_str), "%c", localtime(&(wp->timestamp)) );
    gtk_label_set_text ( GTK_LABEL(timevaluelabel), tmp_str );
  }

  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), latlabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), latentry, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), lonlabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), lonentry, FALSE, FALSE, 0);
  if ( timelabel ) {
    gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), timelabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), timevaluelabel, FALSE, FALSE, 0);
  }
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), altlabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), altentry, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), commentlabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), commententry, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), descriptionlabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), descriptionentry, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), imagelabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), imageentry, FALSE, FALSE, 0);
  if ( hasGeotagCB ) {
    GtkWidget *hbox =  gtk_hbox_new ( FALSE, 0 );
    gtk_box_pack_start (GTK_BOX(hbox), hasGeotagCB, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(hbox), consistentGeotagCB, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox, FALSE, FALSE, 0);
  }
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), symbollabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), GTK_WIDGET(symbolentry), FALSE, FALSE, 0);

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  gtk_widget_show_all ( gtk_dialog_get_content_area(GTK_DIALOG(dialog)) );

  if ( !is_new ) {
    // Shift left<->right to try not to obscure the waypoint.
    trw_layer_dialog_shift ( vtl, GTK_WINDOW(dialog), &(wp->coord), FALSE );
  }

  while ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
  {
    if ( strlen((gchar*)gtk_entry_get_text ( GTK_ENTRY(nameentry) )) == 0 ) /* TODO: other checks (isalpha or whatever ) */
      a_dialog_info_msg ( parent, _("Please enter a name for the waypoint.") );
    else {
      // NB: No check for unique names - this allows generation of same named entries.
      gchar *entered_name = g_strdup ( (gchar*)gtk_entry_get_text ( GTK_ENTRY(nameentry) ) );

      /* Do It */
      ll.lat = convert_dms_to_dec ( gtk_entry_get_text ( GTK_ENTRY(latentry) ) );
      ll.lon = convert_dms_to_dec ( gtk_entry_get_text ( GTK_ENTRY(lonentry) ) );
      vik_coord_load_from_latlon ( &(wp->coord), coord_mode, &ll );
      // Always store in metres
      switch (height_units) {
      case VIK_UNITS_HEIGHT_METRES:
        wp->altitude = atof ( gtk_entry_get_text ( GTK_ENTRY(altentry) ) );
        break;
      case VIK_UNITS_HEIGHT_FEET:
        wp->altitude = VIK_FEET_TO_METERS(atof ( gtk_entry_get_text ( GTK_ENTRY(altentry) ) ));
        break;
      default:
        wp->altitude = atof ( gtk_entry_get_text ( GTK_ENTRY(altentry) ) );
        g_critical("Houston, we've had a problem. height=%d", height_units);
      }
      if ( g_strcmp0 ( wp->comment, gtk_entry_get_text ( GTK_ENTRY(commententry) ) ) )
        vik_waypoint_set_comment ( wp, gtk_entry_get_text ( GTK_ENTRY(commententry) ) );
      if ( g_strcmp0 ( wp->description, gtk_entry_get_text ( GTK_ENTRY(descriptionentry) ) ) )
        vik_waypoint_set_description ( wp, gtk_entry_get_text ( GTK_ENTRY(descriptionentry) ) );
      if ( g_strcmp0 ( wp->image, vik_file_entry_get_filename ( VIK_FILE_ENTRY(imageentry) ) ) )
        vik_waypoint_set_image ( wp, vik_file_entry_get_filename ( VIK_FILE_ENTRY(imageentry) ) );
      if ( wp->image && *(wp->image) && (!a_thumbnails_exists(wp->image)) )
        a_thumbnails_create ( wp->image );

      GtkTreeIter iter, first;
      gtk_tree_model_get_iter_first ( GTK_TREE_MODEL(store), &first );
      if ( !gtk_combo_box_get_active_iter ( GTK_COMBO_BOX(symbolentry), &iter ) || !memcmp(&iter, &first, sizeof(GtkTreeIter)) ) {
        vik_waypoint_set_symbol ( wp, NULL );
      } else {
        gchar *sym;
        gtk_tree_model_get ( GTK_TREE_MODEL(store), &iter, 0, (void *)&sym, -1 );
        vik_waypoint_set_symbol ( wp, sym );
        g_free(sym);
      }

      gtk_widget_destroy ( dialog );
      if ( is_new )
        return entered_name;
      else {
        *updated = TRUE;
        // See if name has been changed
        if ( g_strcmp0 (default_name, entered_name ) )
          return entered_name;
        else
          return NULL;
      }
    }
  }
  gtk_widget_destroy ( dialog );
  return NULL;
}

static void get_selected_foreach_func(GtkTreeModel *model,
                                      GtkTreePath *path,
                                      GtkTreeIter *iter,
                                      gpointer data)
{
  GList **list = data;
  gchar *name;
  gtk_tree_model_get (model, iter, 0, &name, -1);
  *list = g_list_prepend(*list, name);
}

GList *a_dialog_select_from_list ( GtkWindow *parent, GList *names, gboolean multiple_selection_allowed, const gchar *title, const gchar *msg )
{
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkWidget *view;

  GtkWidget *dialog = gtk_dialog_new_with_buttons (title,
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
  /* When something is selected then OK */
  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
  GtkWidget *response_w = NULL;
#if GTK_CHECK_VERSION (2, 20, 0)
  /* Default to not apply - as initially nothing is selected! */
  response_w = gtk_dialog_get_widget_for_response ( GTK_DIALOG(dialog), GTK_RESPONSE_REJECT );
#endif
  GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);

  GtkWidget *scrolledwindow;

  GList *runner = names;
  while (runner)
  {
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, runner->data, -1);
    runner = g_list_next(runner);
  }

  view = gtk_tree_view_new();
  renderer = gtk_cell_renderer_text_new();
  // Use the column header to display the message text,
  // this makes the overall widget allocation simple as treeview takes up all the space
  GtkTreeViewColumn *column;
  column = gtk_tree_view_column_new_with_attributes (msg, renderer, "text", 0, NULL );
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
  gtk_tree_selection_set_mode( gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),
      multiple_selection_allowed ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_BROWSE );
  g_object_unref(store);

  scrolledwindow = gtk_scrolled_window_new ( NULL, NULL );
  gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
  gtk_container_add ( GTK_CONTAINER(scrolledwindow), view );

  gtk_box_pack_start ( GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), scrolledwindow, TRUE, TRUE, 0 );
  // Ensure a reasonable number of items are shown, but let the width be automatically sized
  gtk_widget_set_size_request ( dialog, -1, 400) ;

  gtk_widget_show_all ( dialog );

  if ( response_w )
    gtk_widget_grab_focus ( response_w );

  while ( gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT )
  {
    GList *names_selected = NULL;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_selected_foreach(selection, get_selected_foreach_func, &names_selected);
    if (names_selected)
    {
      gtk_widget_destroy ( dialog );
      return names_selected;
    }
    a_dialog_error_msg(parent, _("Nothing was selected"));
  }
  gtk_widget_destroy ( dialog );
  return NULL;
}

gchar *a_dialog_new_track ( GtkWindow *parent, gchar *default_name, gboolean is_route )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons ( is_route ? _("Add Route") : _("Add Track"),
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
  GtkWidget *label = gtk_label_new ( is_route ? _("Route Name:") : _("Track Name:") );
  GtkWidget *entry = gtk_entry_new ();

  if (default_name)
    gtk_entry_set_text ( GTK_ENTRY(entry), default_name );

  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry, FALSE, FALSE, 0);

  g_signal_connect_swapped ( entry, "activate", G_CALLBACK(a_dialog_response_accept), GTK_DIALOG(dialog) );

  gtk_widget_show ( label );
  gtk_widget_show ( entry );

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  while ( gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT )
  {
    const gchar *constname = gtk_entry_get_text ( GTK_ENTRY(entry) );
    if ( *constname == '\0' )
      a_dialog_info_msg ( parent, _("Please enter a name for the track.") );
    else {
      gchar *name = g_strdup ( constname );
      gtk_widget_destroy ( dialog );
      return name;
    }
  }
  gtk_widget_destroy ( dialog );
  return NULL;
}

static guint today_year = 0;
static guint today_month = 0;
static guint today_day = 0;

static void today_clicked (GtkWidget *cal)
{
  gtk_calendar_select_month ( GTK_CALENDAR(cal), today_month, today_year );
  gtk_calendar_select_day ( GTK_CALENDAR(cal), today_day );
}

/**
 * a_dialog_get_date:
 *
 * Returns: a date as a string - always in ISO8601 format (YYYY-MM-DD)
 *  This string can be NULL (especially when the dialog is cancelled)
 *  Free the string after use
 */
gchar *a_dialog_get_date ( GtkWindow *parent, const gchar *title )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons ( title,
                                                    parent,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_REJECT,
                                                    GTK_STOCK_OK,
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);
  GtkWidget *cal = gtk_calendar_new ();
  GtkWidget *today = gtk_button_new_with_label ( _("Today") );

  static guint year = 0;
  static guint month = 0;
  static guint day = 0;

  if ( year == 0 ) {
    // Store today's date so we can return to it on the button callback
    gtk_calendar_get_date ( GTK_CALENDAR(cal), &year, &month, &day );
    today_year = year;
    today_month = month;
    today_day = day;
  }
  else {
    // Otherwise restore the last selected date
    gtk_calendar_select_month ( GTK_CALENDAR(cal), month, year );
    gtk_calendar_select_day ( GTK_CALENDAR(cal), day );
  }

  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), today, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), cal, FALSE, FALSE, 0);

  g_signal_connect_swapped ( G_OBJECT(today), "clicked", G_CALLBACK(today_clicked), cal );

  gtk_widget_show_all ( dialog );

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  gchar *date_str = NULL;
  if ( gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT )
  {
    gtk_calendar_get_date ( GTK_CALENDAR(cal), &year, &month, &day );
    date_str = g_strdup_printf ( "%d-%02d-%02d", year, month+1, day );
  }
  gtk_widget_destroy ( dialog );
  return date_str;
}

/* creates a vbox full of labels */
GtkWidget *a_dialog_create_label_vbox ( gchar **texts, int label_count, gint spacing, gint padding )
{
  GtkWidget *vbox, *label;
  int i;
  vbox = gtk_vbox_new( TRUE, spacing );

  for ( i = 0; i < label_count; i++ )
  {
    label = gtk_label_new(NULL);
    gtk_label_set_markup ( GTK_LABEL(label), _(texts[i]) );
    gtk_box_pack_start ( GTK_BOX(vbox), label, FALSE, TRUE, padding );
  }
  return vbox;
}

gboolean a_dialog_yes_or_no ( GtkWindow *parent, const gchar *message, const gchar *extra )
{
  GtkWidget *dia;
  dia = gtk_message_dialog_new ( parent,
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_QUESTION,
                                 GTK_BUTTONS_YES_NO,
                                 message, extra );

  if ( gtk_dialog_run ( GTK_DIALOG(dia) ) == GTK_RESPONSE_YES )
  {
    gtk_widget_destroy ( dia );
    return TRUE;
  }
  else
  {
    gtk_widget_destroy ( dia );
    return FALSE;
  }
}

static void zoom_spin_changed ( GtkSpinButton *spin, GtkWidget *pass_along[3] )
{
  if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON(pass_along[2]) ) )
    gtk_spin_button_set_value ( 
        GTK_SPIN_BUTTON(pass_along[GTK_WIDGET(spin) == pass_along[0] ? 1 : 0]),
        gtk_spin_button_get_value ( spin ) );
}

gboolean a_dialog_custom_zoom ( GtkWindow *parent, gdouble *xmpp, gdouble *ympp )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Zoom Factors..."),
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
  GtkWidget *table, *label, *xlabel, *xspin, *ylabel, *yspin, *samecheck;
  GtkWidget *pass_along[3];

  table = gtk_table_new ( 4, 2, FALSE );
  gtk_box_pack_start ( GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table, TRUE, TRUE, 0 );

  label = gtk_label_new ( _("Zoom factor (in meters per pixel):") );
  xlabel = gtk_label_new ( _("X (easting): "));
  ylabel = gtk_label_new ( _("Y (northing): "));

  pass_along[0] = xspin = gtk_spin_button_new ( (GtkAdjustment *) gtk_adjustment_new ( *xmpp, VIK_VIEWPORT_MIN_ZOOM, VIK_VIEWPORT_MAX_ZOOM, 1, 5, 0 ), 1, 8 );
  pass_along[1] = yspin = gtk_spin_button_new ( (GtkAdjustment *) gtk_adjustment_new ( *ympp, VIK_VIEWPORT_MIN_ZOOM, VIK_VIEWPORT_MAX_ZOOM, 1, 5, 0 ), 1, 8 );

  pass_along[2] = samecheck = gtk_check_button_new_with_label ( _("X and Y zoom factors must be equal") );
  /* TODO -- same factor */
  /*  samecheck = gtk_check_button_new_with_label ( "Same x/y zoom factor" ); */

  if ( *xmpp == *ympp )
    gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON(samecheck), TRUE );

  gtk_table_attach_defaults ( GTK_TABLE(table), label, 0, 2, 0, 1 );
  gtk_table_attach_defaults ( GTK_TABLE(table), xlabel, 0, 1, 1, 2 );
  gtk_table_attach_defaults ( GTK_TABLE(table), xspin, 1, 2, 1, 2 );
  gtk_table_attach_defaults ( GTK_TABLE(table), ylabel, 0, 1, 2, 3 );
  gtk_table_attach_defaults ( GTK_TABLE(table), yspin, 1, 2, 2, 3 );
  gtk_table_attach_defaults ( GTK_TABLE(table), samecheck, 0, 2, 3, 4 );

  gtk_widget_show_all ( table );

  g_signal_connect ( G_OBJECT(xspin), "value-changed", G_CALLBACK(zoom_spin_changed), pass_along );
  g_signal_connect ( G_OBJECT(yspin), "value-changed", G_CALLBACK(zoom_spin_changed), pass_along );

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
  {
    *xmpp = gtk_spin_button_get_value ( GTK_SPIN_BUTTON(xspin) );
    *ympp = gtk_spin_button_get_value ( GTK_SPIN_BUTTON(yspin) );
    gtk_widget_destroy ( dialog );
    return TRUE;
  }
  gtk_widget_destroy ( dialog );
  return FALSE;
}

static void split_spin_focused ( GtkSpinButton *spin, GtkWidget *pass_along[1] )
{
  gtk_toggle_button_set_active    (GTK_TOGGLE_BUTTON(pass_along[0]), 1);
}

gboolean a_dialog_time_threshold ( GtkWindow *parent, gchar *title_text, gchar *label_text, guint *thr )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (title_text, 
                                                  parent,
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
  GtkWidget *table, *t1, *t2, *t3, *t4, *spin, *label;
  GtkWidget *pass_along[1];

  table = gtk_table_new ( 4, 2, FALSE );
  gtk_box_pack_start ( GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table, TRUE, TRUE, 0 );

  label = gtk_label_new (label_text);

  t1 = gtk_radio_button_new_with_label ( NULL, _("1 min") );
  t2 = gtk_radio_button_new_with_label_from_widget ( GTK_RADIO_BUTTON(t1), _("1 hour") );
  t3 = gtk_radio_button_new_with_label_from_widget ( GTK_RADIO_BUTTON(t2), _("1 day") );
  t4 = gtk_radio_button_new_with_label_from_widget ( GTK_RADIO_BUTTON(t3), _("Custom (in minutes):") );

  pass_along[0] = t4;

  spin = gtk_spin_button_new ( (GtkAdjustment *) gtk_adjustment_new ( *thr, 0, 65536, 1, 5, 0 ), 1, 0 );

  gtk_table_attach_defaults ( GTK_TABLE(table), label, 0, 2, 0, 1 );
  gtk_table_attach_defaults ( GTK_TABLE(table), t1, 0, 1, 1, 2 );
  gtk_table_attach_defaults ( GTK_TABLE(table), t2, 0, 1, 2, 3 );
  gtk_table_attach_defaults ( GTK_TABLE(table), t3, 0, 1, 3, 4 );
  gtk_table_attach_defaults ( GTK_TABLE(table), t4, 0, 1, 4, 5 );
  gtk_table_attach_defaults ( GTK_TABLE(table), spin, 1, 2, 4, 5 );

  gtk_widget_show_all ( table );

  g_signal_connect ( G_OBJECT(spin), "grab-focus", G_CALLBACK(split_spin_focused), pass_along );

  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );

  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
  {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t1))) {
      *thr = 1;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t2))) {
      *thr = 60;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t3))) {
      *thr = 60 * 24;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t4))) {
      *thr = gtk_spin_button_get_value ( GTK_SPIN_BUTTON(spin) );
    }
    gtk_widget_destroy ( dialog );
    return TRUE;
  }
  gtk_widget_destroy ( dialog );
  return FALSE;
}

/**
 * a_dialog_get_positive_number:
 * 
 * Dialog to return a positive number via a spinbox within the supplied limits
 * 
 * Returns: A value of zero indicates the dialog was cancelled
 */
guint a_dialog_get_positive_number ( GtkWindow *parent, gchar *title_text, gchar *label_text, guint default_num, guint min, guint max, guint step )
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons (title_text,
						   parent,
						   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_STOCK_CANCEL,
						   GTK_RESPONSE_REJECT,
						   GTK_STOCK_OK,
						   GTK_RESPONSE_ACCEPT,
						   NULL);
  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
  GtkWidget *response_w = NULL;
#if GTK_CHECK_VERSION (2, 20, 0)
  response_w = gtk_dialog_get_widget_for_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
#endif

  GtkWidget *table, *spin, *label;
  guint result = default_num;

  table = gtk_table_new ( 2, 1, FALSE );
  gtk_box_pack_start ( GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table, TRUE, TRUE, 0 );

  label = gtk_label_new (label_text);
  spin = gtk_spin_button_new ( (GtkAdjustment *) gtk_adjustment_new ( default_num, min, max, step, 5, 0 ), 1, 0 );

  gtk_table_attach_defaults ( GTK_TABLE(table), label, 0, 1, 0, 1 );
  gtk_table_attach_defaults ( GTK_TABLE(table), spin, 0, 1, 1, 2 );

  if ( response_w )
    gtk_widget_grab_focus ( response_w );

  gtk_widget_show_all ( table );

  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
  {
    result = gtk_spin_button_get_value ( GTK_SPIN_BUTTON(spin) );
    gtk_widget_destroy ( dialog );
    return result;
  }

  // Dialog cancelled
  gtk_widget_destroy ( dialog );
  return 0;
}

#if (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 24)
static void about_url_hook (GtkAboutDialog *about,
                            const gchar    *link,
                            gpointer        data)
{
  open_url (GTK_WINDOW(about), link);
}

static void about_email_hook (GtkAboutDialog *about,
                              const gchar    *email,
                              gpointer        data)
{
  new_email (GTK_WINDOW(about), email);
}
#endif

void a_dialog_about ( GtkWindow *parent )
{
  const gchar *program_name = PACKAGE_NAME;
  const gchar *version = VIKING_VERSION;
  const gchar *website = VIKING_URL;
  const gchar *copyright = "2003-2008, Evan Battaglia\n2008-2013, Viking's contributors";
  const gchar *comments = _("GPS Data and Topo Analyzer, Explorer, and Manager.");
  const gchar *license = _("This program is free software; you can redistribute it and/or modify "
			"it under the terms of the GNU General Public License as published by "
			"the Free Software Foundation; either version 2 of the License, or "
			"(at your option) any later version."
			"\n\n"
			"This program is distributed in the hope that it will be useful, "
			"but WITHOUT ANY WARRANTY; without even the implied warranty of "
			"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
			"GNU General Public License for more details."
			"\n\n"
			"You should have received a copy of the GNU General Public License "
			"along with this program; if not, write to the Free Software "
			"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA");

  // Would be nice to use gtk_about_dialog_add_credit_section (), but that requires gtk 3.4
  // For now shove it in the 'artists' section so at least the information is easily visible
  // Something more advanced might have proper version information too...
  const gchar *libs[] = {
    "Compiled in libraries:",
    // Default libs
    "libglib-2.0",
    "libgthread-2.0",
    "libgtk+-2.0",
    "libgio-2.0",
    // Potentially optional libs (but probably couldn't build without them)
#ifdef HAVE_LIBM
    "libm",
#endif
#ifdef HAVE_LIBZ
    "libz",
#endif
#ifdef HAVE_LIBCURL
    "libcurl",
#endif
    // Actually optional libs
#ifdef HAVE_LIBGPS
    "libgps",
#endif
#ifdef HAVE_LIBEXIF
    "libexif",
#endif
#ifdef HAVE_LIBX11
    "libX11",
#endif
#ifdef HAVE_LIBMAGIC
    "libmagic",
#endif
#ifdef HAVE_LIBBZ2
    "libbz2",
#endif
#ifdef HAVE_LIBSQLITE3
    "libsqlite3",
#endif
    NULL
  };
  // Newer versions of GTK 'just work', calling gtk_show_uri() on the URL or email and opens up the appropriate program
  // This is the old method:
#if (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 24)
  gtk_about_dialog_set_url_hook (about_url_hook, NULL, NULL);
  gtk_about_dialog_set_email_hook (about_email_hook, NULL, NULL);
#endif

  gtk_show_about_dialog (parent,
	/* TODO do not set program-name and correctly set info for g_get_application_name */
  	"program-name", program_name,
	"version", version,
	"website", website,
	"comments", comments,
	"copyright", copyright,
	"license", license,
	"wrap-license", TRUE,
	/* logo automatically retrieved via gtk_window_get_default_icon_list */
	"authors", AUTHORS,
	"documenters", DOCUMENTERS,
	"translator-credits", _("Translation is coordinated on http://launchpad.net/viking"),
	"artists", libs,
	NULL);
}

gboolean a_dialog_map_n_zoom(GtkWindow *parent, gchar *mapnames[], gint default_map, gchar *zoom_list[], gint default_zoom, gint *selected_map, gint *selected_zoom)
{
  gchar **s;

  GtkWidget *dialog = gtk_dialog_new_with_buttons ( _("Download along track"), parent, 0, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL );
  gtk_dialog_set_default_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
  GtkWidget *response_w = NULL;
#if GTK_CHECK_VERSION (2, 20, 0)
  response_w = gtk_dialog_get_widget_for_response ( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
#endif

  GtkWidget *map_label = gtk_label_new(_("Map type:"));
  GtkWidget *map_combo = vik_combo_box_text_new();
  for (s = mapnames; *s; s++)
    vik_combo_box_text_append (GTK_COMBO_BOX(map_combo), *s);
  gtk_combo_box_set_active (GTK_COMBO_BOX(map_combo), default_map);

  GtkWidget *zoom_label = gtk_label_new(_("Zoom level:"));
  GtkWidget *zoom_combo = vik_combo_box_text_new();
  for (s = zoom_list; *s; s++)
    vik_combo_box_text_append (GTK_COMBO_BOX(zoom_combo), *s);
  gtk_combo_box_set_active (GTK_COMBO_BOX(zoom_combo), default_zoom);

  GtkTable *box = GTK_TABLE(gtk_table_new(2, 2, FALSE));
  gtk_table_attach_defaults(box, map_label, 0, 1, 0, 1);
  gtk_table_attach_defaults(box, map_combo, 1, 2, 0, 1);
  gtk_table_attach_defaults(box, zoom_label, 0, 1, 1, 2);
  gtk_table_attach_defaults(box, zoom_combo, 1, 2, 1, 2);

  gtk_box_pack_start ( GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), GTK_WIDGET(box), FALSE, FALSE, 5 );

  if ( response_w )
    gtk_widget_grab_focus ( response_w );

  gtk_widget_show_all ( dialog );
  if ( gtk_dialog_run ( GTK_DIALOG(dialog) ) != GTK_RESPONSE_ACCEPT ) {
    gtk_widget_destroy(dialog);
    return FALSE;
  }

  *selected_map = gtk_combo_box_get_active(GTK_COMBO_BOX(map_combo));
  *selected_zoom = gtk_combo_box_get_active(GTK_COMBO_BOX(zoom_combo));

  gtk_widget_destroy(dialog);
  return TRUE;
}

/**
 * Display a dialog presenting the license of a map.
 * Allow to read the license by launching a web browser.
 */
void a_dialog_license ( GtkWindow *parent, const gchar *map, const gchar *license, const gchar *url)
{
  GtkWidget *dialog = gtk_message_dialog_new (parent,
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_INFO,
                                 GTK_BUTTONS_OK,
                                 _("The map data is licensed: %s."),
                                 license);
  gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
    _("The data provided by '<b>%s</b>' are licensed under the following license: <b>%s</b>."),
    map, license);
#define RESPONSE_OPEN_LICENSE 600
  if (url != NULL) {
    gtk_dialog_add_button (GTK_DIALOG (dialog), _("Open license"), RESPONSE_OPEN_LICENSE);
  }
  gint response;
  do {
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    if (response == RESPONSE_OPEN_LICENSE) {
      open_url (parent, url);
    }
  } while (response != GTK_RESPONSE_DELETE_EVENT && response != GTK_RESPONSE_OK);
  gtk_widget_destroy (dialog);
}
