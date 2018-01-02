#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libspectrum.h>
#include <memory>
#include <vector>

extern "C"
{
	GtkWidget *gtkui_drawing_area = NULL;
}

using namespace std;

static int selected_game = 0;
static gboolean is_game_active = FALSE;

class Music
{
	static const char* tracks[2];

	ALLEGRO_SAMPLE* sample;
	ALLEGRO_SAMPLE_ID sampleId;

public :

	Music()
	{
		srand(time(NULL));

		if (!al_install_system(ALLEGRO_VERSION_INT, NULL))
		{
			fprintf(stderr, "Failed to initialize allegro\n");
			exit(1);
		}

		if (!al_install_audio())
		{
			fprintf(stderr, "Failed to initialize audio\n");
			exit(1);
		}

		if (!al_init_acodec_addon())
		{
			fprintf(stderr, "Failed to initialize audio codecs\n");
			exit(1);
		}

		if (!al_reserve_samples(1))
		{
			fprintf(stderr, "Failed to reserve samples\n");
			exit(1);
		}
	   
		sample = al_load_sample(tracks[rand() % (sizeof(tracks) / sizeof(tracks[0]))]);
	
		al_play_sample(sample, 2.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &sampleId);
	}
	
	~Music()
	{
		al_stop_sample(&sampleId);
		al_destroy_sample(sample);
		al_uninstall_system();
	}
};

const char* Music::tracks[] =
{
	"music/evelynn.ogg",
	"music/adventure.ogg"
};

unique_ptr<Music> music = NULL;

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

	static void draw(GtkWidget *widget, const char* imgpath, gboolean frame)
	{
		{
			cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(GTK_WIDGET(widget)));

			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

			int width = gtk_widget_get_allocated_width(widget);
			int height = gtk_widget_get_allocated_height(widget);

			/* load image and get dimantions */
			cairo_surface_t *img = cairo_image_surface_create_from_png(imgpath);
			if (cairo_surface_status(img) != CAIRO_STATUS_SUCCESS)
			{
				fprintf(stderr, "Failed to load image %s", imgpath);
				exit(1);
			}
			int imgw = cairo_image_surface_get_width(img);
			int imgh = cairo_image_surface_get_height(img);

			float scaleX = (float)(width - 20) / imgw;
			float scaleY = (float)(height - 20) / imgh;
			cairo_scale(cr, scaleX, scaleY);
			cairo_set_source_surface(cr, img, 10 / scaleX, 10 / scaleY);
			cairo_paint(cr);

			cairo_destroy(cr);
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

		draw(widget, screens[i], selected_game == i);

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
extern "C" int utils_open_file(const char *filename, int autoload, void *type);

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

	static const char* games[3];
	
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

		gtk_container_add(GTK_CONTAINER(window), *gtkui_drawing_area);

		machine_init();

		utils_open_file(games[selected_game], TRUE, NULL);
	}
	
	~ZX80()
	{
		gtk_container_remove(GTK_CONTAINER(window), *gtkui_drawing_area);
	}
};

const char* ZX80::games[3] =
{
	"games/chopper/chopper.tzx",
	"games/3dmoto/3dmoto.tzx",
	"games/pool/pool.tzx"
};

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

		while (gtk_events_pending() && !fuse_exiting)
			gtk_main_iteration();
	}	
	
	return 0;
}

