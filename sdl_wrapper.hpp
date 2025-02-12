
#pragma once

#include <iostream>

#include <vector>
#include <memory>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#include "events.hpp"
#include "util.hpp"

struct DrawableArea;

struct Window
{
	SDL_Surface* winSurface = NULL;
	SDL_Window* sdl_window = NULL;
	SDL_Renderer* renderer = NULL;
	TTF_Font * font;
	int w, h;
	
	Window(const char * title, int width, int height)
		: w(width)
		, h(height)
	{
		sdl_window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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
	}
	~Window()
	{
		//TTF_CloseFont(font);
		SDL_DestroyWindow(sdl_window);
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
	std::string text;
	std::vector<int> char_pos;
	SDL_Color color;

	Text(std::string s, Window * win, uint8_t r=64, uint8_t g=64, uint8_t b=64, uint8_t a=255)
		: window(win)
		, color{r, g, b, a}
	{
		set_text(s);
	}
	Text(Text & other) = delete;

	~Text()
	{
		if (message_texture)
			SDL_DestroyTexture(message_texture);
		if (message_surface)
			SDL_FreeSurface(message_surface);
	}
	const std::pair<int,int> get_size() { return {w,h}; }
	const std::string & get_text() const { return text; }
	void set_text(std::string s)
	{
		text = s;
		render();
		calculate_char_pos();
		notify_monitors(text_change_t::text_changed);
	}

	void calculate_char_pos()
	{
		char_pos.clear();
		char_pos.reserve(text.size());
		std::string tmp_text;
		tmp_text.reserve(text.size());
		for (int i=0 ; i<(int)text.size() ; i++)
		{
			tmp_text.push_back(text[i]);
			int calculated_width;
			int dummy;
			//TTF_MeasureText(window->font, tmp_text.c_str(), w, &calculated_width, &dummy);
			TTF_SizeText(window->font, tmp_text.c_str(), &calculated_width, &dummy);
			char_pos.push_back(calculated_width);
		}
	}

	void render()
	{
		if ( ! text.size())
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
		message_surface = TTF_RenderText_Blended(window->font, text.c_str(), color);
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
	ourunique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture/* = nullptr*/;
	SDL_Renderer * renderer;
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
		//texture = SDL_CreateTexture(window->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
		texture.reset(SDL_CreateTexture(window->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, wo, ho));
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		rect_src.x = 0;
		rect_src.y = 0;
		rect_src.w = w;
		rect_src.h = h;
	}
	//~DrawableArea()
	//{
	//	if (texture)
	//	{
	//		SDL_DestroyTexture(texture);
	//		texture = nullptr;
	//	}
	//}

	void set_size(std::pair<int,int> size)
	{
		//if (texture)
		//{
		//	SDL_DestroyTexture(texture);
		//	texture = nullptr;
		//}
		//texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
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
	void copy_from(Text & text, int x, int y, int _w, int _h)
	{
		SDL_Rect rect_srca;
		rect_srca.x = 0;
		rect_srca.y = 0;
		rect_srca.w = _w;
		rect_srca.h = _h;
		if (_w < text.w)
			rect_srca.x = (text.w-_w)/2;
		if (_h < text.h)
			rect_srca.y = (text.h-_h)/2;
		SDL_Rect rect_dest;
		rect_dest.x = x;
		rect_dest.y = y;
		rect_dest.w = _w;
		rect_dest.h = _h;
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
	void draw_rect(int x, int y, int w, int h, int r, int g, int b)
	{
	    SDL_Rect rect;
	    rect.x = x;
	    rect.y = y;
	    rect.w = w;
	    rect.h = h;

		SDL_SetRenderTarget(renderer, texture);
	    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	    SDL_RenderFillRect(renderer, &rect);
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
								my_event.data.mouse.x = ev.x;
								my_event.data.mouse.y = ev.y;
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
								my_event.data.mouse.x = ev.x;
								my_event.data.mouse.y = ev.y;
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
						switch (ev.keysym.scancode)
						{
							case SDL_SCANCODE_UP   : ev.keysym.sym = 1; break;
							case SDL_SCANCODE_DOWN : ev.keysym.sym = 2; break;
							case SDL_SCANCODE_LEFT : ev.keysym.sym = 3; break;
							case SDL_SCANCODE_RIGHT: ev.keysym.sym = 4; break;
							default:
								break;
						}
						for(auto & window : windows)
							if (window->sdl_window == sdl_window)
							{
								event my_event{event_type::key, 0};
								my_event.data.key.pressed  = e.type == SDL_KEYDOWN;
								my_event.data.key.released = e.type == SDL_KEYUP;
								my_event.data.key.keycode = ev.keysym.scancode;
								my_event.data.key.charcode = ev.keysym.sym;
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