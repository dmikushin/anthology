#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_memfile.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <input.h>
#include <libspectrum.h>
#include <map>
#include <memory>
#include <SDL.h>
#include <vector>

// on 3.12.x the _start and _end versions of these functions were renamed
// make sure to still support the old names
#if ! GTK_CHECK_VERSION(3,12,0)
#define gtk_widget_set_margin_start gtk_widget_set_margin_left
#define gtk_widget_set_margin_end gtk_widget_set_margin_right
#endif

using namespace std;

// Container for embedded music sources.
unique_ptr<map<string, vector<char> > > music_sources;

// Container for embedded image sources.
unique_ptr<map<string, vector<char> > > image_sources;

// Container for embedded game sources.
unique_ptr<map<string, vector<char> > > game_sources;

extern "C"
{
	GtkWidget *gtkui_drawing_area = NULL;
	int selected_game = 0;
	int is_game_active = FALSE;
}

class Music
{
	ALLEGRO_FILE* trackFile;
	ALLEGRO_SAMPLE* trackSample;
	ALLEGRO_SAMPLE_ID trackId;

public :

	Music()
	{
		srand(time(NULL));

		if (!al_install_system(ALLEGRO_VERSION_INT, NULL))
		{
			fprintf(stderr, "Failed to initialize allegro\n");
			exit(-1);
		}

		if (!al_install_audio())
		{
			fprintf(stderr, "Failed to initialize audio\n");
			exit(-1);
		}

		if (!al_init_acodec_addon())
		{
			fprintf(stderr, "Failed to initialize audio codecs\n");
			exit(-1);
		}

		if (!al_reserve_samples(1))
		{
			fprintf(stderr, "Failed to reserve samples\n");
			exit(-1);
		}

		if (!image_sources.get())
		{
			fprintf(stderr, "Image sources list is empty\n");
			exit(-1);
		}
		
		int ii = 0;
		const int itrack = rand() % music_sources->size();
		for (map<string, vector<char> >::iterator i = music_sources->begin(), e = music_sources->end(); i != e; i++)
		{
			if (ii != itrack)
			{
				ii++;
				continue;
			}
			
			const string& filename = i->first;
			vector<char>& track = i->second;

			trackFile = al_open_memfile(&track[0], track.size(), "rb");
			if (!trackFile)
			{
				fprintf(stderr, "Error reading music track #%d \"%s\"\n", itrack, filename.c_str());
				exit(-1);
			}

			string::size_type idx = filename.rfind('.');
			
			if (idx == string::npos)
			{
				fprintf(stderr, "Error determining music track #%d \"%s\" ident\n", itrack, filename.c_str());
				exit(-1);
			}

			string ext = filename.substr(idx);
			
			trackSample = al_load_sample_f(trackFile, ext.c_str());
			
			break;
		}
	
		al_play_sample(trackSample, 2.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &trackId);
	}
	
	~Music()
	{
		al_stop_sample(&trackId);
		al_destroy_sample(trackSample);
		al_fclose(trackFile);
		al_uninstall_system();
	}
};

unique_ptr<Music> music = NULL;

static cairo_status_t
stdio_read_func (void *closure, unsigned char *data, unsigned int size)
{
    FILE* file = (FILE*)closure;

    while (size) {
	size_t ret;

	ret = fread (data, 1, size, file);
	size -= ret;
	data += ret;

	if (size && (feof (file) || ferror (file)))
	    return CAIRO_STATUS_READ_ERROR;
    }

    return CAIRO_STATUS_SUCCESS;
}

class Menu
{
	GtkWidget *window;

	static int indexes[3];
	static const char* screens[3];

	GtkWidget *grid;
	GtkWidget *gameChopper;
	GtkWidget *gameMoto;
	GtkWidget *gamePool;
	GtkWidget *gameEmpty;

	static void draw(GtkWidget *widget, const int iimage, gboolean frame)
	{
		{
			cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(GTK_WIDGET(widget)));

			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

			int width = gtk_widget_get_allocated_width(widget);
			int height = gtk_widget_get_allocated_height(widget);

			if (!image_sources.get())
			{
				fprintf(stderr, "Image sources list is empty\n");
				exit(-1);
			}

			// Load image and get dimensions.
			int ii = 0;
			for (map<string, vector<char> >::iterator i = image_sources->begin(), e = image_sources->end(); i != e; i++)
			{
				if (ii != iimage)
				{
					ii++;
					continue;
				}
			
				const string& filename = i->first;
				vector<char>& image = i->second;
				if (!&image[0])
				{
					fprintf(stderr, "Failed to load image \"%s\"\n", filename.c_str());
					exit(-1);
				}
				FILE* imageFile = fmemopen(&image[0], image.size(), "rb");
				if (!imageFile)
				{
					fprintf(stderr, "Failed to load image \"%s\"\n", filename.c_str());
					exit(-1);
				}
				cairo_surface_t *img = cairo_image_surface_create_from_png_stream(stdio_read_func, imageFile);
				if (cairo_surface_status(img) != CAIRO_STATUS_SUCCESS)
				{
					fprintf(stderr, "Failed to load image \"%s\"\n", filename.c_str());
					exit(-1);
				}
				int imgw = cairo_image_surface_get_width(img);
				int imgh = cairo_image_surface_get_height(img);

				float scaleX = (float)(width - 20) / imgw;
				float scaleY = (float)(height - 20) / imgh;
				cairo_scale(cr, scaleX, scaleY);
				cairo_set_source_surface(cr, img, 10 / scaleX, 10 / scaleY);
				cairo_paint(cr);

				cairo_destroy(cr);
				
				break;
			}
		}

		if (frame)
		{
			cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(GTK_WIDGET(widget)));

			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

			size_t width = gtk_widget_get_allocated_width(widget);
			size_t height = gtk_widget_get_allocated_height(widget);

			cairo_set_source_rgb(cr, 0.9, 0.9, 0);
			cairo_set_line_width(cr, 10);
			cairo_rectangle(cr, 5, 5, width - 10, height - 10);
			cairo_stroke(cr);

			cairo_destroy(cr);
		}
	}

	static gboolean on_draw_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
	{
		int i = *(int*)user_data;

		draw(widget, i, selected_game == i);

		return FALSE;
	}

public :

	Menu(GtkWidget *window_) : window(window_)
	{
		// Construct a container for games.
		grid = gtk_grid_new();
		gtk_grid_set_row_spacing (GTK_GRID (grid), 16);
		gtk_grid_set_column_spacing (GTK_GRID (grid), 16);
		gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
		gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

		gtk_widget_set_margin_start(grid,20);
		gtk_widget_set_margin_end(grid,20);
		gtk_widget_set_margin_top(grid,20);
		gtk_widget_set_margin_bottom(grid,20);

		// Chopper game.
		gameChopper = gtk_drawing_area_new();
		gtk_grid_attach(GTK_GRID(grid), gameChopper, 0, 0, 1, 1);
		g_signal_connect(G_OBJECT(gameChopper), "draw", G_CALLBACK(on_draw_event), &indexes[0]);

		// Moto game.
		gameMoto = gtk_drawing_area_new();
		gtk_grid_attach(GTK_GRID(grid), gameMoto, 1, 0, 1, 1);
		g_signal_connect(G_OBJECT(gameMoto), "draw", G_CALLBACK(on_draw_event), &indexes[1]);

		// Pool game.
		gamePool = gtk_drawing_area_new();
		gtk_grid_attach(GTK_GRID(grid), gamePool, 2, 0, 1, 1);
		g_signal_connect(G_OBJECT(gamePool), "draw", G_CALLBACK(on_draw_event), &indexes[2]);

		// Empty cells.
		gameEmpty = gtk_drawing_area_new();
		gtk_grid_attach(GTK_GRID(grid), gameEmpty, 0, 1, 3, 1);

		gtk_container_add(GTK_CONTAINER(window), grid);
	}
	
	~Menu()
	{
		gtk_container_remove(GTK_CONTAINER(window), grid);
	}
};

int Menu::indexes[3] = { 0, 1, 2 };

const char* Menu::screens[3] =
{
	"games/chopper/chopper.png",
	"games/3dmoto/3dmoto.png",
	"games/pool/pool.png"
};

unique_ptr<Menu> menu = NULL;

extern "C" int machine_init();
extern "C" void z80_do_opcodes();
extern "C" int event_do_events();
extern "C" int tape_read_buffer(unsigned char *buffer, size_t length,
	libspectrum_id_t type, const char *filename, int autoload);
extern "C" int gtkkeyboard_keypress(GtkWidget *widget, GdkEvent *event, gpointer data);
extern "C" int gtkkeyboard_keyrelease(GtkWidget *widget, GdkEvent *event, gpointer data);

#define DISPLAY_SCREEN_WIDTH 320
#define DISPLAY_SCREEN_HEIGHT 240

extern "C"
{
	int fuse_exiting;
	extern int stop_event;

	// A copy of every pixel on the screen
	extern libspectrum_word gtkdisplay_image[2 * DISPLAY_SCREEN_HEIGHT][DISPLAY_SCREEN_WIDTH];
}

class ZX80
{
	GtkWidget *window;
	GtkWidget** gtkui_drawing_area;

	vector<uint32_t> pixels;
	int stride;
	
	cairo_surface_t *surface;

	uint32_t palette[16];

	// Called by gtkui_drawing_area on "draw" event
	static gboolean gtkdisplay_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data)
	{
		ZX80& zx80 = *(ZX80*)user_data;
	
		size_t width = gtk_widget_get_allocated_width(widget);
		size_t height = gtk_widget_get_allocated_height(widget);

		if (zx80.pixels.size() != width * height)
		{
			zx80.pixels.resize(width * height);
	
			// Create a new surface, if size has changed.
			if (zx80.surface) cairo_surface_destroy(zx80.surface);

			zx80.stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, DISPLAY_SCREEN_WIDTH);

			zx80.surface = cairo_image_surface_create_for_data((unsigned char*)&zx80.pixels[0], CAIRO_FORMAT_RGB24,
				DISPLAY_SCREEN_WIDTH, DISPLAY_SCREEN_HEIGHT, zx80.stride);
		}
		
		// Copy from gtkdisplay_image, converting the format
		for (int yy = 0; yy < DISPLAY_SCREEN_HEIGHT; yy++)
		{
			uint32_t *rgb24 = (uint32_t*)((char*)&zx80.pixels[0] + yy * zx80.stride);

			libspectrum_word *display = &gtkdisplay_image[yy * 2][0];

			for (int i = 0; i < DISPLAY_SCREEN_WIDTH; i++)
				rgb24[i] = zx80.palette[display[i]];
		}

		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

		float scaleX = (float)width / DISPLAY_SCREEN_WIDTH;
		float scaleY = (float)height / DISPLAY_SCREEN_HEIGHT;
		cairo_scale(cr, scaleX, scaleY);

		// Repaint the drawing area
		cairo_set_source_surface(cr, zx80.surface, 0, 0);
		cairo_paint(cr);

		return FALSE;
	}

	static void initPalette(libspectrum_dword* palette)
	{
		for (int i = 0; i < 16; i++)
		{
			guchar red, green, blue;

			red   = rgbColors[i][0];
			green = rgbColors[i][1];
			blue  = rgbColors[i][2];

#ifdef WORDS_BIGENDIAN
			palette[i] = blue << 24 | green << 16 |  red << 8;
#else
			palette[i] = blue | green << 8 |  red << 16;
#endif // WORDS_BIGENDIAN
		}
	}
	
	// The color palette
	static const guchar rgbColors[16][3];

public :

	ZX80(GtkWidget *window_, GtkWidget** gtkui_drawing_area_) :
		window(window_), gtkui_drawing_area(gtkui_drawing_area_),
		pixels(0), surface(NULL)
	{
		initPalette((libspectrum_dword*)palette);
	
		*gtkui_drawing_area = gtk_drawing_area_new();

		g_signal_connect(G_OBJECT(*gtkui_drawing_area), "draw", G_CALLBACK(gtkdisplay_draw), this);
		g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(gtkkeyboard_keypress), NULL);
		gtk_widget_add_events(window, GDK_KEY_RELEASE_MASK );
		g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(gtkkeyboard_keyrelease), NULL);

		gtk_container_add(GTK_CONTAINER(window), *gtkui_drawing_area);

		machine_init();

		if (!game_sources.get())
		{
			fprintf(stderr, "Game sources list is empty\n");
			exit(-1);
		}

		int ii = 0;
		for (map<string, vector<char> >::iterator i = game_sources->begin(), e = game_sources->end(); i != e; i++)
		{
			if (ii != selected_game)
			{
				ii++;
				continue;
			}

			const string& filename = i->first;
			vector<char>& game = i->second;

			int error = tape_read_buffer((unsigned char*)&game[0], game.size(), LIBSPECTRUM_ID_TAPE_TZX, filename.c_str(), TRUE);
			if (error)
			{
				fprintf(stderr, "Error loading game \"%s\": errno = %d\n", filename.c_str(), error);
				exit(-1);
			}
			break;
		}
	}
	
	~ZX80()
	{
		gtk_container_remove(GTK_CONTAINER(window), *gtkui_drawing_area);
	}
};

struct Keymap
{
	uint32_t joystick_key;
	uint32_t native_key;
};

extern "C"
{
	Keymap joystick[4][127] =
	{
		{
			{ INPUT_JOYSTICK_LEFT, INPUT_KEY_o },
			{ INPUT_JOYSTICK_RIGHT, INPUT_KEY_p },
			{ 0, INPUT_KEY_m },
			{ 0, INPUT_KEY_Return },
			{ INPUT_JOYSTICK_DOWN, INPUT_KEY_a },
			{ INPUT_JOYSTICK_UP, INPUT_KEY_q },
			{ 3, INPUT_KEY_s },
			{ 8, INPUT_KEY_Escape },
		},
		{
			{ INPUT_JOYSTICK_LEFT, INPUT_KEY_1 },
			{ INPUT_JOYSTICK_RIGHT, INPUT_KEY_0 },
			{ 0, INPUT_KEY_minus },
			{ INPUT_JOYSTICK_DOWN, INPUT_KEY_8 },
			{ INPUT_JOYSTICK_UP, INPUT_KEY_9 },
			{ 3, INPUT_KEY_1 },
			{ 8, INPUT_KEY_Escape },
		},
		{
			{ INPUT_JOYSTICK_LEFT, INPUT_KEY_a },
			{ INPUT_JOYSTICK_RIGHT, INPUT_KEY_s },
			{ INPUT_JOYSTICK_DOWN, INPUT_KEY_s },
			{ INPUT_JOYSTICK_UP, INPUT_KEY_a },
			{ 0, INPUT_KEY_Return },
			{ 3, INPUT_KEY_1 },
			{ 1, INPUT_KEY_2 },
			{ 2, INPUT_KEY_l },
			{ 8, INPUT_KEY_Escape },
		},
		// Keys for main menu screen
		{
			{ 0, INPUT_KEY_Return },
			{ 2 /* INPUT_JOYSTICK_LEFT */, INPUT_KEY_Left },
			{ 1 /* INPUT_JOYSTICK_RIGHT */, INPUT_KEY_Right },
			{ 8, INPUT_KEY_Escape },
		},
	};
}

const guchar ZX80::rgbColors[16][3] =
{
	{   0,   0,   0 },
	{   0,   0, 192 },
	{ 192,   0,   0 },
	{ 192,   0, 192 },
	{   0, 192,   0 },
	{   0, 192, 192 },
	{ 192, 192,   0 },
	{ 192, 192, 192 },
	{   0,   0,   0 },
	{   0,   0, 255 },
	{ 255,   0,   0 },
	{ 255,   0, 255 },
	{   0, 255,   0 },
	{   0, 255, 255 },
	{ 255, 255,   0 },
	{ 255, 255, 255 },
};

unique_ptr<ZX80> zx80 = NULL;

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	return FALSE;
}

/* Another callback */
static void destroy(GtkWidget *widget, gpointer data)
{
	fuse_exiting = 1;
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	if (!is_game_active)
	{
		switch (event->keyval)
		{
		case GDK_KEY_Left :
			selected_game = MAX(0, selected_game - 1);
			gtk_widget_queue_draw(widget);
			break;
		case GDK_KEY_Right :
			selected_game = MIN(2, selected_game + 1);
			gtk_widget_queue_draw(widget);
			break;
		case GDK_KEY_Return :
			is_game_active = TRUE;
			gtk_widget_queue_draw(widget);
			break;
		case GDK_KEY_Escape :
			fuse_exiting = 1;
		}
	}
	else
	{
		switch (event->keyval)
		{
		case GDK_KEY_Escape :
			is_game_active = FALSE;
			stop_event = -1;
			gtk_widget_queue_draw(widget);
			break;
		}
	}

	return FALSE;
}

static gboolean on_draw_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
	if (!is_game_active)
	{
		zx80.reset(NULL);

		if (!music.get()) music.reset(new Music());
		if (!menu.get()) menu.reset(new Menu(widget));
	}
	else
	{
		music.reset(NULL);
		menu.reset(NULL);

		if (!zx80.get()) zx80.reset(new ZX80(widget, &gtkui_drawing_area));
	}

	gtk_widget_show_all(widget);

	return FALSE;
}

extern "C"
{
	GtkWidget *gtkui_window = NULL;
}

static void joystick_button_action(SDL_JoyButtonEvent *buttonevent, input_event_type type)
{
	int menu_keys_index = sizeof(joystick) / sizeof(joystick[0]) - 1;

	input_key joystick_key = (input_key)buttonevent->button;
	for (int i = 0; i < 127; i++)
	{
		if (joystick[menu_keys_index][i].joystick_key == joystick_key)
		{
			input_key native_key = (input_key)joystick[menu_keys_index][i].native_key;

			if (native_key == INPUT_KEY_Left)
			{
				selected_game = MAX(0, selected_game - 1);
				gtk_widget_queue_draw(gtkui_window);
				break;
			}
			else if (native_key == INPUT_KEY_Right)
			{
				selected_game = MIN(2, selected_game + 1);
				gtk_widget_queue_draw(gtkui_window);
				break;
			}
			else if (native_key == INPUT_KEY_Return)
			{
				is_game_active = TRUE;
				gtk_widget_queue_draw(gtkui_window);
				break;
			}
			else if (native_key == INPUT_KEY_Escape)
			{
				fuse_exiting = 1;
				break;
			}
		}
	}
}

static void ui_joystick_poll()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_JOYBUTTONDOWN:
			joystick_button_action(&(event.jbutton), INPUT_EVENT_KEYPRESS);
			break;
		case SDL_JOYBUTTONUP:
//			joystick_button_action(&(event.jbutton), INPUT_EVENT_KEYRELEASE);
			break;
		case SDL_JOYAXISMOTION:
			//sdljoystick_axismove(&(event.jaxis));
			break;
		default:
			break;
		}
	}
}

extern "C" int ui_init(int *argc, char ***argv)
{
	/* This is called in all GTK applications. Arguments are parsed
	 * from the command line and are returned to the application. */
	gtk_init(argc, argv);
	
	/* create a new window */
	GtkWidget *widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtkui_window = widget;

	gtk_window_set_title(GTK_WINDOW(widget), "Anthology");
	
	g_signal_connect(G_OBJECT(widget), "delete-event", G_CALLBACK (delete_event), NULL);
	g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK (on_key_press), NULL);
	g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK (destroy), NULL);
	g_signal_connect(G_OBJECT(widget), "draw", G_CALLBACK(on_draw_event), NULL);

	gtk_window_set_default_size(GTK_WINDOW(widget), 800, 480);
	gtk_window_set_position(GTK_WINDOW(widget), GTK_WIN_POS_CENTER);
	gtk_window_maximize(GTK_WINDOW(widget));

	gtk_widget_show_all(widget);

	while (!fuse_exiting)
	{
		if (zx80.get())
		{
			z80_do_opcodes();
			event_do_events();
		}
		else
		{
			ui_joystick_poll();
		}

		while (gtk_events_pending() && !fuse_exiting)
		{
			gtk_main_iteration();
		}
	}	
	
	return 0;
}

