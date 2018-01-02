/* sdljoystick.c: routines for dealing with the SDL joystick
   Copyright (c) 2003-2004 Darren Salt, Fredrick Meunier, Philip Kendall

   $Id: sdljoystick.c 4915 2013-04-07 05:32:09Z fredm $

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

   Fred: fredm@spamcop.net

*/

struct Keymap
{
	uint32_t joystick_key;
	uint32_t native_key;
};

extern struct Keymap joystick[3][127];

extern int selected_game;

#include <config.h>

#if !defined USE_JOYSTICK || defined HAVE_JSW_H
/* Fake joystick, or override UI-specific handling */
#include "../uijoystick.c"

#else			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */

#include <SDL.h>

#include "compat.h"
#include "input.h"
#include "sdljoystick.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"

static SDL_Joystick *joystick1 = NULL;
static SDL_Joystick *joystick2 = NULL;

static void do_axis( int which, Sint16 value, input_key negative,
		     input_key positive );

int
ui_joystick_init( void )
{
  int error, retval;

#ifdef UI_SDL
  error = SDL_InitSubSystem( SDL_INIT_JOYSTICK );
#else
  /* Other UIs could handle joysticks by the SDL library */
  error = SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_VIDEO);
#endif

  if ( error ) {
    ui_error( UI_ERROR_ERROR, "failed to initialise joystick subsystem" );
    return 0;
  }

  retval = SDL_NumJoysticks();

  if( retval >= 2 ) {

    retval = 2;

    if( ( joystick2 = SDL_JoystickOpen( 1 ) ) == NULL ) {
      ui_error( UI_ERROR_ERROR, "failed to initialise joystick 2" );
      return 0;
    }

    if( SDL_JoystickNumAxes( joystick2 ) < 2    ||
        SDL_JoystickNumButtons( joystick2 ) < 1    ) {
      ui_error( UI_ERROR_ERROR, "sorry, joystick 2 is inadequate!" );
      return 0;
    }

  }

  if( retval > 0 ) {

    if( ( joystick1 = SDL_JoystickOpen( 0 ) ) == NULL ) {
      ui_error( UI_ERROR_ERROR, "failed to initialise joystick 1" );
      return 0;
    }
 
    if( SDL_JoystickNumAxes( joystick1 ) < 2    ||
        SDL_JoystickNumButtons( joystick1 ) < 1    ) {
      ui_error( UI_ERROR_ERROR, "sorry, joystick 1 is inadequate!" );
      return 0;
    }
  }

  SDL_JoystickEventState( SDL_ENABLE );

  return retval;
}

extern int is_game_active, stop_event;

static void
button_action( SDL_JoyButtonEvent *buttonevent, input_event_type type )
{
	input_event_t fuse_event;
	fuse_event.type = type;
	input_key joystick_key = buttonevent->button;
	for (int i = 0; i < 127; i++)
	{
		if (joystick[selected_game][i].joystick_key == joystick_key)
		{
			input_key native_key = joystick[selected_game][i].native_key;

			if (native_key == INPUT_KEY_Escape)
			{
				is_game_active = FALSE;
				stop_event = -1;
				gtk_widget_queue_draw(gtkui_window);
				
				break;
			}

			fuse_event.types.key.native_key = native_key;
			fuse_event.types.key.spectrum_key = native_key;

			input_event( &fuse_event );
			
			break;
		}
	}
}

void
ui_joystick_poll( void )
{
  /* No action needed in SDL UI; joysticks already handled by the SDL events
     system */

#ifndef UI_SDL
  SDL_Event event;

  while( SDL_PollEvent( &event ) ) {
    switch( event.type ) {
    case SDL_JOYBUTTONDOWN:
      button_action(&(event.jbutton), INPUT_EVENT_KEYPRESS);
      break;
    case SDL_JOYBUTTONUP:
      button_action(&(event.jbutton), INPUT_EVENT_KEYRELEASE);
      break;
    case SDL_JOYAXISMOTION:
      sdljoystick_axismove(&(event.jaxis));
      break;
    default:
      break;
    }
  }
#endif

}

void
sdljoystick_axismove( SDL_JoyAxisEvent *axisevent )
{
  if( axisevent->axis == 0 ) {
    do_axis( axisevent->which, axisevent->value,
	     INPUT_JOYSTICK_LEFT, INPUT_JOYSTICK_RIGHT );
  } else if( axisevent->axis == 1 ) {
    do_axis( axisevent->which, axisevent->value,
	     INPUT_JOYSTICK_UP,   INPUT_JOYSTICK_DOWN  );
  }
}

static int map_key(input_key joystick_key, input_key* native_key)
{
	for (int i = 0; i < 127; i++)
		if (joystick[selected_game][i].joystick_key == joystick_key)
		{
			*native_key = joystick[selected_game][i].native_key;
			return 0;
		}
	
	return 1;
}

static void
do_axis( int which, Sint16 value, input_key negative, input_key positive )
{
	input_event_t fuse_event;
	input_key native_key;

	if (value == 0)
	{
		// Release both buttons, i.e. both joystick axis directions
		fuse_event.type = INPUT_EVENT_KEYRELEASE;
		if (map_key(negative, &native_key) == 0)
		{
			fuse_event.types.key.native_key = native_key;
			fuse_event.types.key.spectrum_key = native_key;
			input_event(&fuse_event);
		}
		if (map_key(positive, &native_key) == 0)
		{
			fuse_event.types.key.native_key = native_key;
			fuse_event.types.key.spectrum_key = native_key;
			input_event(&fuse_event);
		}
	}
	else
	{
		// Press button corresponding to either negative or positive joystick
		// axis direction
		fuse_event.type = INPUT_EVENT_KEYPRESS;
		if (map_key((value < 0) ? negative : positive, &native_key) == 0)
		{
			fuse_event.types.key.native_key = native_key;
			fuse_event.types.key.spectrum_key = native_key;
			input_event(&fuse_event);
		}
	}
}

void
ui_joystick_end( void )
{
  if( joystick1 != NULL || joystick2 != NULL ) {

    SDL_JoystickEventState( SDL_IGNORE );
    if( joystick1 != NULL ) SDL_JoystickClose( joystick1 );
    if( joystick2 != NULL ) SDL_JoystickClose( joystick2 );
    joystick1 = NULL;
    joystick2 = NULL;

  }

#ifdef UI_SDL
  SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
#else
  SDL_Quit();
#endif
}

#endif			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */
