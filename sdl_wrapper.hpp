
#pragma once

#include <iostream>

#include <vector>
#include <memory>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <unicode/unistr.h>

#include "events.hpp"
#include "util.hpp"

struct DrawableArea;

enum Scancode
{
	None = 0,
	Up,
	Down,
	Left,
	Right,
	Ctrl,
	Shift,
	Alt,
	Altgr,
};

enum MouseCursorImg
{
    ARROW      = SDL_SYSTEM_CURSOR_ARROW,     /**< Arrow */
    IBEAM      = SDL_SYSTEM_CURSOR_IBEAM,     /**< I-beam */
    WAIT       = SDL_SYSTEM_CURSOR_WAIT,      /**< Wait */
    CROSSHAIR  = SDL_SYSTEM_CURSOR_CROSSHAIR, /**< Crosshair */
    WAITARROW  = SDL_SYSTEM_CURSOR_WAITARROW, /**< Small wait cursor (or Wait if not available) */
    SIZENWSE   = SDL_SYSTEM_CURSOR_SIZENWSE,  /**< Double arrow pointing northwest and southeast */
    SIZENESW   = SDL_SYSTEM_CURSOR_SIZENESW,  /**< Double arrow pointing northeast and southwest */
    SIZEWE     = SDL_SYSTEM_CURSOR_SIZEWE,    /**< Double arrow pointing west and east */
    SIZENS     = SDL_SYSTEM_CURSOR_SIZENS,    /**< Double arrow pointing north and south */
    SIZEALL    = SDL_SYSTEM_CURSOR_SIZEALL,   /**< Four pointed arrow pointing north, south, east, and west */
    NO         = SDL_SYSTEM_CURSOR_NO,        /**< Slashed circle or crossbones */
    HAND       = SDL_SYSTEM_CURSOR_HAND,      /**< Hand */
    NB_CURSOR,
};

struct Window
{
	SDL_Surface* winSurface = NULL;
	SDL_Window* sdl_window = NULL;
	SDL_Renderer* renderer = NULL;
	TTF_Font * font;
	int w, h;
	MouseCursorImg current_cursor = MouseCursorImg::ARROW;
	std::vector<SDL_Cursor*> mouse_cursors;
	
	Window(const char * title, int width, int height)
		: w(width)
		, h(height)
	{
		sdl_window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_RENDERER_PRESENTVSYNC);
		if ( ! sdl_window)
			throw;

		winSurface = SDL_GetWindowSurface( sdl_window );
		if ( ! winSurface)
			throw;

		renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
		if ( ! renderer)
			throw;

		font = TTF_OpenFont("ttf/UbuntuMono-R.ttf", 16);
		if ( ! font)
			throw;

		for (int i=0 ; i<SDL_SystemCursor::SDL_NUM_SYSTEM_CURSORS ; ++i)
			mouse_cursors.push_back(SDL_CreateSystemCursor((SDL_SystemCursor)i));
	}
	virtual ~Window()
	{
		//TTF_CloseFont(font);
		SDL_DestroyWindow(sdl_window);
	}

	// returns the previous cursor
	virtual MouseCursorImg set_cursor(MouseCursorImg n)
	{
		if (n == current_cursor)
			return n;
		auto c = current_cursor;
		SDL_SetCursor(mouse_cursors[n]);
		current_cursor = n;
		return c;
	}
	virtual void _redraw() {}
	virtual bool handle_event(event) { return false; }

	virtual void present()
	{
		SDL_RenderPresent(renderer);
	}
};

void save_texture(SDL_Renderer *ren, SDL_Texture *tex, const char *filename)
{
    SDL_Texture *ren_tex;
    SDL_Surface *surf;
    int st;
    int w;
    int h;
    int format;
    void *pixels;

    pixels  = NULL;
    surf    = NULL;
    ren_tex = NULL;
    format  = SDL_PIXELFORMAT_RGBA32;

    /* Get information about texture we want to save */
    st = SDL_QueryTexture(tex, NULL, NULL, &w, &h);
    if (st != 0) {
        SDL_Log("Failed querying texture: %s\n", SDL_GetError());
        goto cleanup;
    }

    ren_tex = SDL_CreateTexture(ren, format, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!ren_tex) {
        SDL_Log("Failed creating render texture: %s\n", SDL_GetError());
        goto cleanup;
    }

    /*
     * Initialize our canvas, then copy texture to a target whose pixel data we 
     * can access
     */
    st = SDL_SetRenderTarget(ren, ren_tex);
    if (st != 0) {
        SDL_Log("Failed setting render target: %s\n", SDL_GetError());
        goto cleanup;
    }

    SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(ren);

    st = SDL_RenderCopy(ren, tex, NULL, NULL);
    if (st != 0) {
        SDL_Log("Failed copying texture data: %s\n", SDL_GetError());
        goto cleanup;
    }

    /* Create buffer to hold texture data and load it */
    pixels = malloc(w * h * SDL_BYTESPERPIXEL(format));
    if (!pixels) {
        SDL_Log("Failed allocating memory\n");
        goto cleanup;
    }

    st = SDL_RenderReadPixels(ren, NULL, format, pixels, w * SDL_BYTESPERPIXEL(format));
    if (st != 0) {
        SDL_Log("Failed reading pixel data: %s\n", SDL_GetError());
        goto cleanup;
    }

    /* Copy pixel data over to surface */
    surf = SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h, SDL_BITSPERPIXEL(format), w * SDL_BYTESPERPIXEL(format), format);
    if (!surf) {
        SDL_Log("Failed creating new surface: %s\n", SDL_GetError());
        goto cleanup;
    }

    /* Save result to an image */
    st = SDL_SaveBMP(surf, filename);
    if (st != 0) {
        SDL_Log("Failed saving image: %s\n", SDL_GetError());
        goto cleanup;
    }

    SDL_Log("Saved texture as BMP to \"%s\"\n", filename);

cleanup:
    SDL_FreeSurface(surf);
    free(pixels);
    SDL_DestroyTexture(ren_tex);
}


enum class text_change_t
{
	any = 0,
	resized,
	text_changed,
};
struct Text : monitorable<text_change_t>
{
	Window * window = nullptr;
	SDL_Surface* message_surface = nullptr;
	SDL_Texture* message_texture = nullptr;
	int w, h;
	icu::UnicodeString text;
	std::vector<int> char_pos;
	SDL_Color color;

	Text(Window * win, uint8_t r=64, uint8_t g=64, uint8_t b=64, uint8_t a=255)
		: Text(icu::UnicodeString(), win, r, g, b, a)
	{}
	Text(std::string s, Window * win, uint8_t r=64, uint8_t g=64, uint8_t b=64, uint8_t a=255)
		: Text(icu::UnicodeString::fromUTF8(s), win, r, g, b, a)
	{}
	Text(icu::UnicodeString s, Window * win, uint8_t r=64, uint8_t g=64, uint8_t b=64, uint8_t a=255)
		: window(win)
		, color{r, g, b, a}
	{
		set_text(s);
	}
	Text(const Text & other)
		: window(other.window)
		, w(other.w)
		, h(other.h)
		, color(other.color)
	{
		set_text(other.text);
	}
	~Text()
	{
		if (message_texture)
			SDL_DestroyTexture(message_texture);
		if (message_surface)
			SDL_FreeSurface(message_surface);
	}

	Text & operator=(const Text & other)
	{
		set_text(other.text);
		return *this;
	}

	const std::pair<int,int> get_size() { return {w,h}; }
	const icu::UnicodeString & get_text() const { return text; }
	void set_text(icu::UnicodeString s)
	{
		text = s;
		render();
		calculate_char_pos();
		notify_monitors(text_change_t::text_changed);
	}
	void set_text(std::string s)
	{
		set_text(icu::UnicodeString::fromUTF8(s));
	}

	void calculate_char_pos()
	{
		char_pos.clear();
		char_pos.reserve(text.length());
		icu::UnicodeString tmp_text;
		//tmp_text.reserve(text.length());
		for (int i=0 ; i<(int)text.length() ; i++)
		{
			tmp_text.append(text[i]);
			int calculated_width;
			int dummy;
			//TTF_MeasureText(window->font, tmp_text.c_str(), w, &calculated_width, &dummy);
			std::string converted;
			tmp_text.toUTF8String(converted);
			TTF_SizeText(window->font, converted.c_str(), &calculated_width, &dummy);
			char_pos.push_back(calculated_width);
		}
	}

	void render()
	{
		if ( ! text.length())
		{
			if (message_texture)
				SDL_DestroyTexture(message_texture);
			message_texture = SDL_CreateTexture(window->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 0, 0);
			//SDL_RenderFillRect
			SDL_SetTextureBlendMode(message_texture, SDL_BLENDMODE_BLEND);
			return;
		}

		if (message_surface)
			SDL_FreeSurface(message_surface);

		// pre-render text
		std::string converted;
		text.toUTF8String(converted);
		message_surface = TTF_RenderUTF8_Blended(window->font, converted.c_str(), color);
		message_texture = SDL_CreateTextureFromSurface(window->renderer, message_surface);
		SDL_SetTextureBlendMode(message_texture, SDL_BLENDMODE_BLEND);

	    w = message_surface->w;
	    h = message_surface->h;

		if (message_surface)
		{
			SDL_FreeSurface(message_surface);
			message_surface = nullptr;
		}
	}
	int get_pos_at(int x)
	{
		return std::lower_bound(std::begin(char_pos), std::end(char_pos), x) - std::begin(char_pos);
	}
	int get_char_x(int i)
	{
		if (i == 0)
			return 0;
		else
			return char_pos[i-1];
	}
};

template<typename T, typename D=std::default_delete<T>>
class ourunique_ptr : public std::unique_ptr<T,D>
{
public:
	ourunique_ptr(T*t)
		: std::unique_ptr<T,D>(t)
	{}
	ourunique_ptr(T*t, D d)
		: std::unique_ptr<T,D>(t, d)
	{}
	operator T*()
	{
		return this->get();
	}
};

struct DrawableArea
{
	SDL_Renderer * renderer;
	ourunique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture;
	int w, h;
	int wo, ho;
	SDL_Rect rect_src;

	DrawableArea(Window * window, int width, int height)
		: renderer(window->renderer)
		, texture(nullptr, &SDL_DestroyTexture)
		, w(width)
		, h(height)
		, wo(width*2)
		, ho(height*2)
	{
		texture.reset(SDL_CreateTexture(window->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, wo, ho));
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		rect_src.x = 0;
		rect_src.y = 0;
		rect_src.w = w;
		rect_src.h = h;
	}

	void set_size(std::pair<int,int> size)
	{
		if (std::get<0>(size) > wo || std::get<1>(size) > ho)
		{
			texture.reset(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, std::get<0>(size)*2, std::get<1>(size)*2));
			SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
			wo = std::get<0>(size)*2;
			ho = std::get<1>(size)*2;
		}

		w = std::get<0>(size);
		h = std::get<1>(size);
		rect_src.w = w;
		rect_src.h = h;
	}

	void copy_from(DrawableArea & other, int x, int y)
	{
		SDL_Rect rect;
		rect.x = x;
		rect.y = y;
		rect.w = other.w;
		rect.h = other.h;
		SDL_SetRenderTarget(renderer, texture);
		SDL_RenderCopy(renderer, other.texture, &other.rect_src, &rect);
		SDL_SetRenderTarget(renderer, NULL);
	}
	void copy_from(Text & text, int x, int y)
	{
		SDL_Rect rect;
		rect.x = x;
		rect.y = y;
		rect.w = text.w;
		rect.h = text.h;
		SDL_SetRenderTarget(renderer, texture);
		SDL_RenderCopy(renderer, text.message_texture, NULL, &rect);
		SDL_SetRenderTarget(renderer, NULL);
	}
	void copy_from_text_to_rect(Text & text, int dest_x, int dest_y, int dest_w, int dest_h)
	{
		SDL_Rect rect_srca;
		rect_srca.x = 0;
		rect_srca.y = 0;
		rect_srca.w = text.w;
		rect_srca.h = text.h;
		SDL_Rect rect_dest;
		rect_dest.x = dest_x;
		rect_dest.y = dest_y;
		rect_dest.w = dest_w;
		rect_dest.h = dest_h;
		if (dest_w < text.w)
		{
			rect_srca.x += (text.w-dest_w)/2;
			rect_srca.w = dest_w;
		}
		else if (dest_w > text.w)
		{
			rect_dest.x += (dest_w-text.w)/2;
			rect_dest.w = text.w;
		}
		if (dest_h < text.h)
		{
			rect_srca.y += (text.h-dest_h)/2;
			rect_srca.h = dest_h;
		}
		else if (dest_h > text.h)
		{
			rect_dest.y += (dest_h-text.h)/2;
			rect_dest.h = text.h;
		}
		SDL_SetRenderTarget(renderer, texture);
		SDL_RenderCopy(renderer, text.message_texture, &rect_srca, &rect_dest);
		SDL_SetRenderTarget(renderer, NULL);
	}
	void refresh_window()
	{
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopy(renderer, texture, &rect_src, NULL);
	}

	void fill(int r, int g, int b, int a=255)
	{
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = w;
		rect.h = h;
		SDL_SetRenderTarget(renderer, texture);
	    SDL_SetRenderDrawColor(renderer, r, g, b, a);
	    SDL_RenderFillRect(renderer, &rect);
		SDL_SetRenderTarget(renderer, NULL);
	}
	void draw_line(int x1, int y1, int x2, int y2, int r, int g, int b, int a=255)
	{
		SDL_SetRenderTarget(renderer, texture);
	    SDL_SetRenderDrawColor(renderer, r, g, b, a);
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
		SDL_SetRenderTarget(renderer, NULL);
	}
	void draw_rect(int x, int y, int w, int h, int r, int g, int b, int a=255)
	{
	    SDL_Rect rect;
	    rect.x = x;
	    rect.y = y;
	    rect.w = w;
	    rect.h = h;

		SDL_SetRenderTarget(renderer, texture);
	    SDL_SetRenderDrawColor(renderer, r, g, b, a);
	    SDL_RenderDrawRect(renderer, &rect);
		SDL_SetRenderTarget(renderer, NULL);
	}
	void draw_3d_rect(int x, int y, int w, int h, int light, int dark, bool sunken)
	{
	    if ( ! sunken)
	    {
	    	std::swap(dark, light);
	    }

		SDL_SetRenderTarget(renderer, texture);
	    SDL_SetRenderDrawColor(renderer, dark, dark, dark, 255);
	    SDL_RenderDrawLine(renderer, x, y, x+w-1, y);
	    SDL_RenderDrawLine(renderer, x, y, x, y+h-1);
	    SDL_SetRenderDrawColor(renderer, light, light, light, 255);
	    SDL_RenderDrawLine(renderer, x, y+h-1, x+w-1, y+h-1);
	    SDL_RenderDrawLine(renderer, x+w-1, y, x+w-1, y+h);
		SDL_SetRenderTarget(renderer, NULL);
	}
	void fill_rect(int x, int y, int w, int h, int r, int g, int b, int a=255)
	{
	    SDL_Rect rect;
	    rect.x = x;
	    rect.y = y;
	    rect.w = w;
	    rect.h = h;

		SDL_SetRenderTarget(renderer, texture);
	    SDL_SetRenderDrawColor(renderer, r, g, b, a);
	    SDL_RenderFillRect(renderer, &rect);
		SDL_SetRenderTarget(renderer, NULL);
	}
};

struct SDL
{
	using Window_t = Window;
	using DrawableArea_t = DrawableArea;

	std::vector<std::unique_ptr<Window_t>> windows;

	SDL()
	{
		// Initialize SDL. SDL_Init will return -1 if it fails.
		if ( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
			throw;
	    TTF_Init();
		SDL_StartTextInput();
	}
	~SDL()
	{
		// Quit SDL
		TTF_Quit();
		SDL_Quit();	
	}

	template<typename W>
	W & make_window(const char * title, int width, int height)
	{
		windows.emplace_back(std::make_unique<W>(title, width, height));
		return (W&)*windows.back();
	}

	void loop()
	{
		while (true)
		{
			SDL_Event e;
			while( SDL_PollEvent( &e ) != 0 )
			{
				switch (e.type)
				{
					case SDL_WINDOWEVENT:
					{
						SDL_Window* sdl_window = SDL_GetWindowFromID(((SDL_WindowEvent&)e).windowID);
						if ( ! sdl_window)
							break;
						for(auto & window : windows)
							if (window->sdl_window == sdl_window)
							{
								switch(((SDL_WindowEvent&)e).event)
								{
									//case SDL_WINDOWEVENT_SHOWN:
									case SDL_WINDOWEVENT_EXPOSED:
										window->handle_event(event{event_type::window_shown, 0});
										break;
									case SDL_WINDOWEVENT_MOVED:
										break;
									case SDL_WINDOWEVENT_ENTER:
										break;
									case SDL_WINDOWEVENT_RESIZED:
										event my_event{event_type::window_resized, 0};
										my_event.data.window_resized.w = ((SDL_WindowEvent&)e).data1;
										my_event.data.window_resized.h = ((SDL_WindowEvent&)e).data2;
										window->handle_event(my_event);
										break;
								}
								break;
							}
						break;
					}
					case SDL_MOUSEMOTION:
					{
						SDL_MouseMotionEvent & ev = (SDL_MouseMotionEvent &)e;

						SDL_Window * sdl_window = SDL_GetWindowFromID(ev.windowID);
						if ( ! sdl_window)
							break;
						for(auto & window : windows)
							if (window->sdl_window == sdl_window)
							{
								event my_event{event_type::mouse, 0};
								my_event.data.mouse.pressed  = false;
								my_event.data.mouse.released = false;
								my_event.data.mouse.button = 0;
								my_event.data.mouse.x = ev.x-1; // circumventing sdl bug (?)
								my_event.data.mouse.y = ev.y-2; // circumventing sdl bug (?)
								window->handle_event(my_event);
								break;
							}

						break;
					}
					case SDL_MOUSEBUTTONDOWN:
					case SDL_MOUSEBUTTONUP:
					{
						SDL_MouseButtonEvent & ev = (SDL_MouseButtonEvent&) e;

						SDL_Window * sdl_window = SDL_GetWindowFromID(ev.windowID);
						if ( ! sdl_window)
							break;
						for(auto & window : windows)
							if (window->sdl_window == sdl_window)
							{
								event my_event{event_type::mouse, 0};
								my_event.data.mouse.pressed  = e.type == SDL_MOUSEBUTTONDOWN;
								my_event.data.mouse.released = e.type == SDL_MOUSEBUTTONUP;
								my_event.data.mouse.button = ev.button;
								my_event.data.mouse.x = ev.x-1; // circumventing sdl bug (?)
								my_event.data.mouse.y = ev.y-2; // circumventing sdl bug (?)
								window->handle_event(my_event);
								break;
							}
						break;
					}
					case SDL_KEYDOWN:
					case SDL_KEYUP:
					{
						SDL_KeyboardEvent & ev = (SDL_KeyboardEvent&) e;
						SDL_Window * sdl_window = SDL_GetWindowFromID(ev.windowID);
						if ( ! sdl_window)
							break;
						event my_event{event_type::key, 0};
						my_event.data.key.pressed  = e.type == SDL_KEYDOWN;
						my_event.data.key.released = e.type == SDL_KEYUP;
						my_event.data.key.charcode = ev.keysym.sym;
						switch (ev.keysym.scancode)
						{
							case SDL_SCANCODE_UP    : my_event.data.key.keycode = Scancode::Up   ; break;
							case SDL_SCANCODE_DOWN  : my_event.data.key.keycode = Scancode::Down ; break;
							case SDL_SCANCODE_LEFT  : my_event.data.key.keycode = Scancode::Left ; break;
							case SDL_SCANCODE_RIGHT : my_event.data.key.keycode = Scancode::Right; break;
							case SDL_SCANCODE_LCTRL : my_event.data.key.keycode = Scancode::Ctrl ; break;
							case SDL_SCANCODE_RCTRL : my_event.data.key.keycode = Scancode::Ctrl ; break;
							case SDL_SCANCODE_LSHIFT: my_event.data.key.keycode = Scancode::Shift; break;
							case SDL_SCANCODE_RSHIFT: my_event.data.key.keycode = Scancode::Shift; break;
							case SDL_SCANCODE_LALT  : my_event.data.key.keycode = Scancode::Alt  ; break;
							case SDL_SCANCODE_RALT  : my_event.data.key.keycode = Scancode::Altgr; break;
							default:
								break;
						}
						for(auto & window : windows)
							if (window->sdl_window == sdl_window)
							{
								window->handle_event(my_event);
								break;
							}
						break;
					}
					case SDL_TEXTINPUT:
					{
						SDL_TextInputEvent & ev = (SDL_TextInputEvent&) e;
						SDL_Window * sdl_window = SDL_GetWindowFromID(ev.windowID);
						if ( ! sdl_window)
							break;
						event my_event{event_type::text, 0};
						my_event.data.text.composition   = ev.text;
						my_event.data.text.cursor_pos    = strlen(ev.text);
						my_event.data.text.selection_len = 0;
						for(auto & window : windows)
							if (window->sdl_window == sdl_window)
							{
								window->handle_event(my_event);
								break;
							}
						break;
					}
					case SDL_TEXTEDITING:
					{
						SDL_TextEditingEvent & ev = (SDL_TextEditingEvent&) e;
						SDL_Window * sdl_window = SDL_GetWindowFromID(ev.windowID);
						if ( ! sdl_window)
							break;
						event my_event{event_type::text, 0};
						my_event.data.text.composition   = ev.text;
						my_event.data.text.cursor_pos    = ev.start;
						my_event.data.text.selection_len = ev.length;
						for(auto & window : windows)
							if (window->sdl_window == sdl_window)
							{
								window->handle_event(my_event);
								break;
							}
						break;
					}
					case SDL_QUIT:
						return;
				}
			}

			// wait before processing the next frame
			SDL_Delay(10); 
		}
	}
};