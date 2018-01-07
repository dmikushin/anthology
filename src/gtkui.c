/* gtkui.c: GTK+ routines for dealing with the user interface
   Copyright (c) 2000-2005 Philip Kendall, Russell Marks

   $Id: gtkui.c 4968 2013-05-19 16:11:17Z zubzero $

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <glib.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "fuse.h"
#include "gtkcompat.h"
#include "gtkinternals.h"
#include "keyboard.h"
#include "machine.h"
#include "machines/specplus3.h"
#include "menu.h"
#include "peripherals/ide/simpleide.h"
#include "peripherals/ide/zxatasp.h"
#include "peripherals/ide/zxcf.h"
#include "peripherals/joystick.h"
#include "psg.h"
#include "rzx.h"
#include "screenshot.h"
#include "settings.h"
#include "snapshot.h"
#include "timer/timer.h"
#include "ui/ui.h"
#include "utils.h"

/* The main Fuse window */
extern GtkWidget *gtkui_window;

/* The area into which the screen will be drawn */
extern GtkWidget *gtkui_drawing_area;

/* The UIManager used to create the menu bar */
GtkUIManager *ui_manager_menu = NULL;

/* True if we were paused via the Machine/Pause menu item */
static int paused = 0;

/* Structure used by the radio button selection widgets (eg the
   graphics filter selectors and Machine/Select) */
typedef struct gtkui_select_info {

  GtkWidget *dialog;
  GtkWidget **buttons;

  /* Used by the graphics filter selectors */
  scaler_available_fn selector;
  scaler_type selected;

  /* Used by the joystick confirmation */
  ui_confirm_joystick_t joystick;

} gtkui_select_info;

static void menu_options_filter_done( GtkWidget *widget, gpointer user_data );
static void menu_machine_select_done( GtkWidget *widget, gpointer user_data );

static const GtkTargetEntry drag_types[] =
{
    { "text/uri-list", GTK_TARGET_OTHER_APP, 0 }
};

#include "tape.h"

extern libspectrum_tape *tape;

int
ui_event(void)
{
  while(gtk_events_pending())
    gtk_main_iteration();
  return 0;
}

int
ui_end(void)
{
  /* Don't display the window whilst doing all this! */
  gtk_widget_hide( gtkui_window );

  g_object_unref( ui_manager_menu );

  return 0;
}

/* Create a dialog box with the given error message */
int
ui_error_specific( ui_error_level severity, const char *message )
{
  GtkWidget *dialog, *label, *vbox, *content_area, *action_area;
  const gchar *title;

  /* If we don't have a UI yet, we can't output widgets */
  if( !display_ui_initialised ) return 0;

  /* Set the appropriate title */
  switch( severity ) {
  case UI_ERROR_INFO:	 title = "Fuse - Info"; break;
  case UI_ERROR_WARNING: title = "Fuse - Warning"; break;
  case UI_ERROR_ERROR:	 title = "Fuse - Error"; break;
  default:		 title = "Fuse - (Unknown Error Level)"; break;
  }

  /* Create the dialog box */
  dialog = gtkstock_dialog_new( title, G_CALLBACK( gtk_widget_destroy ) );

  /* Add the OK button into the lower half */
  gtkstock_create_close( dialog, NULL, G_CALLBACK (gtk_widget_destroy),
			 FALSE );

  /* Create a label with that message */
  label = gtk_label_new( message );

  /* Make a new vbox for the top part for saner spacing */
  vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );
  action_area = gtk_dialog_get_action_area( GTK_DIALOG( dialog ) );
  gtk_box_pack_start( GTK_BOX( content_area ), vbox, TRUE, TRUE, 0 );
  gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );
  gtk_container_set_border_width( GTK_CONTAINER( action_area ), 5 );

  /* Put the label in it */
  gtk_container_add( GTK_CONTAINER( vbox ), label );

  gtk_widget_show_all( dialog );

  return 0;
}

/* The callbacks used by various routines */

/* Select a graphics filter from those for which `available' returns
   true */
scaler_type
menu_get_scaler( scaler_available_fn selector )
{
  GtkWidget *content_area;
  gtkui_select_info dialog;
  GSList *button_group = NULL;

  int count;
  scaler_type scaler;

  /* Store the function which tells us which scalers are currently
     available */
  dialog.selector = selector;

  /* No scaler currently selected */
  dialog.selected = SCALER_NUM;

  /* Some space to store the radio buttons in */
  dialog.buttons = malloc( SCALER_NUM * sizeof(GtkWidget* ) );
  if( dialog.buttons == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return SCALER_NUM;
  }

  count = 0;

  /* Create the necessary widgets */
  dialog.dialog = gtkstock_dialog_new( "Fuse - Select Scaler", NULL );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog.dialog ) );

  for( scaler = 0; scaler < SCALER_NUM; scaler++ ) {

    if( !selector( scaler ) ) continue;

    dialog.buttons[ count ] =
      gtk_radio_button_new_with_label( button_group, scaler_name( scaler ) );
    button_group =
      gtk_radio_button_get_group( GTK_RADIO_BUTTON( dialog.buttons[ count ] ) );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dialog.buttons[ count ] ),
				  current_scaler == scaler );

    gtk_container_add( GTK_CONTAINER( content_area ), dialog.buttons[ count ] );

    count++;
  }

  /* Create and add the actions buttons to the dialog box */
  gtkstock_create_ok_cancel( dialog.dialog, NULL,
                             G_CALLBACK( menu_options_filter_done ),
                             (gpointer) &dialog, DEFAULT_DESTROY,
                             DEFAULT_DESTROY );

  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  gtk_main();

  return dialog.selected;
}

/* Callback used by the filter selection dialog */
static void
menu_options_filter_done( GtkWidget *widget GCC_UNUSED, gpointer user_data )
{
  int i, count;
  gtkui_select_info *ptr = (gtkui_select_info*)user_data;

  count = 0;

  for( i = 0; i < SCALER_NUM; i++ ) {

    if( !ptr->selector( i ) ) continue;

    if( gtk_toggle_button_get_active(
	  GTK_TOGGLE_BUTTON( ptr->buttons[ count ] )
	)
      ) {
      ptr->selected = i;
    }

    count++;
  }

  gtk_widget_destroy( ptr->dialog );
  gtk_main_quit();
}

static gboolean
gtkui_run_main_loop( gpointer user_data GCC_UNUSED )
{
  gtk_main();
  return FALSE;
}

/* Machine/Pause */
void
menu_machine_pause( GtkAction *gtk_action GCC_UNUSED, gpointer data GCC_UNUSED )
{
  int error;

  if( paused ) {
    paused = 0;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED,
			 UI_STATUSBAR_STATE_INACTIVE );
    timer_estimate_reset();
    gtk_main_quit();
  } else {

    /* Stop recording any competition mode RZX file */
    if( rzx_recording && rzx_competition_mode ) {
      ui_error( UI_ERROR_INFO, "Stopping competition mode RZX recording" );
      error = rzx_stop_recording(); if( error ) return;
    }

    paused = 1;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED, UI_STATUSBAR_STATE_ACTIVE );

    /* Create nested main loop outside this callback to allow unpause */
    g_idle_add( (GSourceFunc)gtkui_run_main_loop, NULL );
  }

}

/* Called by the menu when Machine/Reset selected */
void
menu_machine_reset( GtkAction *gtk_action GCC_UNUSED, guint action )
{
  int hard_reset = action;
  const char *message = "Reset?";

  if( hard_reset )
    message = "Hard reset?";

  if( gtkui_confirm( message ) && machine_reset( hard_reset ) ) {
    ui_error( UI_ERROR_ERROR, "couldn't reset machine: giving up!" );

    /* FIXME: abort() seems a bit extreme here, but it'll do for now */
    fuse_abort();
  }
}

/* Called by the menu when Machine/Select selected */
void
menu_machine_select( GtkAction *gtk_action GCC_UNUSED,
                     gpointer data GCC_UNUSED )
{
  GtkWidget *content_area;
  gtkui_select_info dialog;

  int i;

  /* Some space to store the radio buttons in */
  dialog.buttons = malloc( machine_count * sizeof(GtkWidget* ) );
  if( dialog.buttons == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return;
  }

  /* Stop emulation */
  fuse_emulation_pause();

  /* Create the necessary widgets */
  dialog.dialog = gtkstock_dialog_new( "Fuse - Select Machine", NULL );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog.dialog ) );

  dialog.buttons[0] =
    gtk_radio_button_new_with_label(
      NULL, libspectrum_machine_name( machine_types[0]->machine )
    );

  for( i=1; i<machine_count; i++ ) {
    dialog.buttons[i] =
      gtk_radio_button_new_with_label(
        gtk_radio_button_get_group( GTK_RADIO_BUTTON( dialog.buttons[i-1] ) ),
	libspectrum_machine_name( machine_types[i]->machine )
      );
  }

  for( i=0; i<machine_count; i++ ) {
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dialog.buttons[i] ),
				  machine_current == machine_types[i] );
    gtk_container_add( GTK_CONTAINER( content_area ), dialog.buttons[i] );
  }

  /* Create and add the actions buttons to the dialog box */
  gtkstock_create_ok_cancel( dialog.dialog, NULL,
                             G_CALLBACK( menu_machine_select_done ),
                             (gpointer) &dialog, DEFAULT_DESTROY,
                             DEFAULT_DESTROY );

  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  gtk_main();

  /* And then carry on with emulation again */
  fuse_emulation_unpause();
}

/* Callback used by the machine selection dialog */
static void
menu_machine_select_done( GtkWidget *widget GCC_UNUSED, gpointer user_data )
{
  int i;
  gtkui_select_info *ptr = user_data;

  for( i=0; i<machine_count; i++ ) {
    if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(ptr->buttons[i]) ) &&
        machine_current != machine_types[i]
      )
    {
      machine_select( machine_types[i]->machine );
    }
  }

  gtk_widget_destroy( ptr->dialog );
  gtk_main_quit();
}

void
menu_machine_debugger( GtkAction *gtk_action GCC_UNUSED,
                       gpointer data GCC_UNUSED )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  if( paused ) ui_debugger_activate();
}

/* Called on machine selection */
int
ui_widgets_reset( void )
{
  gtkui_pokefinder_clear();
  return 0;
}

void
menu_help_keyboard( GtkAction *gtk_action GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtkui_picture( "keyboard.scr", 0 );
}

void
menu_help_about( GtkAction *gtk_action GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtk_show_about_dialog( GTK_WINDOW( gtkui_window ),
                         "name", "Fuse",
                         "comments", "The Free Unix Spectrum Emulator",
                         "copyright", FUSE_COPYRIGHT,
                         "version", VERSION,
                         "website", PACKAGE_URL,
                         NULL );
}

/* Generic `tidy-up' callback */
void
gtkui_destroy_widget_and_quit( GtkWidget *widget, gpointer data GCC_UNUSED )
{
  gtk_widget_destroy( widget );
  gtk_main_quit();
}

/* Functions to activate and deactivate certain menu items */

int
ui_menu_item_set_active( const char *path, int active )
{
  GtkWidget *menu_item;

  /* Translate UI-indepentment path to GTK UI path */
  gchar *full_path = g_strdup_printf ("/MainMenu%s", path );

  menu_item = gtk_ui_manager_get_widget( ui_manager_menu, full_path );
  g_free( full_path );

  if( !menu_item ) {
    ui_error( UI_ERROR_ERROR, "couldn't get menu item '%s' from menu_factory",
	      path );
    return 1;
  }
  gtk_widget_set_sensitive( menu_item, active );

  return 0;
}

static void
confirm_joystick_done( GtkWidget *widget GCC_UNUSED, gpointer user_data )
{
  int i;
  gtkui_select_info *ptr = user_data;

  for( i = 0; i < JOYSTICK_CONN_COUNT; i++ ) {

    GtkToggleButton *button = GTK_TOGGLE_BUTTON( ptr->buttons[ i ] );

    if( gtk_toggle_button_get_active( button ) ) {
      ptr->joystick = i;
      break;
    }
  }

  gtk_widget_destroy( ptr->dialog );
  gtk_main_quit();
}

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type,
		     int inputs GCC_UNUSED )
{
  GtkWidget *content_area;
  gtkui_select_info dialog;
  char title[ 80 ];
  int i;
  GSList *group = NULL;

  if( !settings_current.joy_prompt ) return UI_CONFIRM_JOYSTICK_NONE;

  /* Some space to store the radio buttons in */
  dialog.buttons =
    malloc( JOYSTICK_CONN_COUNT * sizeof( *dialog.buttons ) );
  if( !dialog.buttons ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return UI_CONFIRM_JOYSTICK_NONE;
  }

  /* Stop emulation */
  fuse_emulation_pause();

  /* Create the necessary widgets */
  snprintf( title, sizeof( title ), "Fuse - Configure %s Joystick",
	    libspectrum_joystick_name( libspectrum_type ) );
  dialog.dialog = gtkstock_dialog_new( title, NULL );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog.dialog ) );

  for( i = 0; i < JOYSTICK_CONN_COUNT; i++ ) {

    GtkWidget **button = &( dialog.buttons[ i ] );

    *button =
      gtk_radio_button_new_with_label( group, joystick_connection[ i ] );
    group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( *button ) );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( *button ), i == 0 );
    gtk_container_add( GTK_CONTAINER( content_area ), *button );
  }

  /* Create and add the actions buttons to the dialog box */
  gtkstock_create_ok_cancel( dialog.dialog, NULL,
                             G_CALLBACK( confirm_joystick_done ),
                             (gpointer) &dialog, DEFAULT_DESTROY,
                             DEFAULT_DESTROY );

  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  dialog.joystick = UI_CONFIRM_JOYSTICK_NONE;
  gtk_main();

  /* And then carry on with emulation again */
  fuse_emulation_unpause();

  return dialog.joystick;
}

/*
 * Font code
 */

int
gtkui_get_monospaced_font( gtkui_font *font )
{
  *font = pango_font_description_from_string( "Monospace 10" );
  if( !(*font) ) {
    ui_error( UI_ERROR_ERROR, "couldn't find a monospaced font" );
    return 1;
  }

  return 0;
}

void
gtkui_free_font( gtkui_font font )
{
  pango_font_description_free( font );
}

void
gtkui_set_font( GtkWidget *widget, gtkui_font font )
{
  gtk_widget_modify_font( widget, font );
}

static gboolean
key_press( GtkTreeView *list, GdkEventKey *event, gpointer user_data )
{
  GtkAdjustment *adjustment = user_data;
  GtkTreePath *path;
  GtkTreeViewColumn *focus_column;
  gdouble base, oldbase, base_limit;
  gdouble page_size, step_increment;
  int cursor_row = 0;
  int num_rows;

  base = oldbase = gtk_adjustment_get_value( adjustment );
  page_size = gtk_adjustment_get_page_size( adjustment );
  step_increment = gtk_adjustment_get_step_increment( adjustment );
  num_rows = ( page_size + 1 ) / step_increment;

  /* Get selected row */
  gtk_tree_view_get_cursor( list, &path, &focus_column );
  if( path ) {
    int *indices = gtk_tree_path_get_indices( path );
    if( indices ) cursor_row = indices[0];
    gtk_tree_path_free( path );
  }

  switch( event->keyval )
  {
  case GDK_KEY_Up:
    if( cursor_row == 0 )
      base -= step_increment;
    break;

  case GDK_KEY_Down:
    if( cursor_row == num_rows - 1 )
      base += step_increment;
    break;

  case GDK_KEY_Page_Up:
    base -= gtk_adjustment_get_page_increment( adjustment );
    break;

  case GDK_KEY_Page_Down:
    base += gtk_adjustment_get_page_increment( adjustment );
    break;

  case GDK_KEY_Home:
    cursor_row = 0;
    base = gtk_adjustment_get_lower( adjustment );
    break;

  case GDK_KEY_End:
    cursor_row = num_rows - 1;
    base = gtk_adjustment_get_upper( adjustment ) - page_size;
    break;

  default:
    return FALSE;
  }

  if( base < 0 ) {
    base = 0;
  } else {
    base_limit = gtk_adjustment_get_upper( adjustment ) - page_size;
    if( base > base_limit ) base = base_limit;
  }

  if( base != oldbase ) {
    gtk_adjustment_set_value( adjustment, base );

    /* Mark selected row */
    path = gtk_tree_path_new_from_indices( cursor_row, -1 );
    gtk_tree_view_set_cursor( list, path, NULL, FALSE );
    gtk_tree_path_free( path );
    return TRUE;
  }

  return FALSE;
}

static gboolean
wheel_scroll_event( GtkTreeView *list GCC_UNUSED, GdkEvent *event,
                    gpointer user_data )
{
  GtkAdjustment *adjustment = user_data;
  gdouble base, oldbase, base_limit;

  base = oldbase = gtk_adjustment_get_value( adjustment );

  switch( event->scroll.direction )
  {
  case GDK_SCROLL_UP:
    base -= gtk_adjustment_get_page_increment( adjustment ) / 2;
    break;
  case GDK_SCROLL_DOWN:
    base += gtk_adjustment_get_page_increment( adjustment ) / 2;
    break;
  default:
    return FALSE;
  }

  if( base < 0 ) {
    base = 0;
  } else {
    base_limit = gtk_adjustment_get_upper( adjustment ) - 
                 gtk_adjustment_get_page_size( adjustment );
    if( base > base_limit ) base = base_limit;
  }

  if( base != oldbase ) gtk_adjustment_set_value( adjustment, base );

  return TRUE;
}

void
gtkui_scroll_connect( GtkTreeView *list, GtkAdjustment *adj )
{
  g_signal_connect( list, "key-press-event",
                    G_CALLBACK( key_press ), adj );
  g_signal_connect( list, "scroll-event",
                    G_CALLBACK( wheel_scroll_event ), adj );
}
