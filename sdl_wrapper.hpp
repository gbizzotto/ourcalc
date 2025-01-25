
#pragma once

#include <iostream>

#include <vector>
#include <memory>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

struct DrawableArea;

struct Window
{
	SDL_Surface* winSurface = NULL;
	SDL_Window* sdl_window = NULL;
	SDL_Renderer* renderer = NULL;
	TTF_Font * font;
	int w, h;
	
	Window(const char * title, int width, int height)
	{
		// Create our window
		sdl_window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);

		// Make sure creating the window succeeded
		if ( ! sdl_window)
			throw;

		// Get the surface from the window
		winSurface = SDL_GetWindowSurface( sdl_window );

		// Make sure getting the surface succeeded
		if ( ! winSurface)
			throw;

		renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);
		if ( ! renderer)
			throw;

		font = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf", 24);
		if ( ! font)
			throw;
	}
	~Window()
	{
		// This will also destroy the surface
		//TTF_CloseFont(font);
		SDL_DestroyWindow(sdl_window);
	}

	virtual void event_redraw() {}

	virtual bool event_mouse_button_down([[maybe_unused]]int x, [[maybe_unused]]int y) { return false; }
	virtual bool event_mouse_button_up  ([[maybe_unused]]int x, [[maybe_unused]]int y) { return false; }
	virtual bool event_key_down([[maybe_unused]]int key) { return false; }
	virtual bool event_key_up  ([[maybe_unused]]int key) { return false; }

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

struct Text
{
	Window * window = nullptr;
	SDL_Surface* message_surface = nullptr;
	SDL_Texture* message_texture = nullptr;
	int w, h;
	std::string text;
	std::vector<int> char_pos;

	Text(std::string s, Window * win)
		: window(win)
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
			message_texture = SDL_CreateTexture(window->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 0, 0);
			//SDL_RenderFillRect
			SDL_SetTextureBlendMode(message_texture, SDL_BLENDMODE_BLEND);
			return;
		}

		if (message_texture)
			SDL_DestroyTexture(message_texture);
		if (message_surface)
			SDL_FreeSurface(message_surface);

		// pre-render text
		SDL_Color color = {64,64,64,0};

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

struct DrawableArea
{
	SDL_Texture * texture = nullptr;
	SDL_Renderer * renderer;
	int w, h;

	DrawableArea(Window * window, int width, int height)
		: renderer(window->renderer)
		, w(width)
		, h(height)
	{
		texture = SDL_CreateTexture(window->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	}
	~DrawableArea()
	{
		if (texture)
		{
			SDL_DestroyTexture(texture);
			texture = nullptr;
		}
	}

	void set_size(std::pair<int,int> size)
	{
		w = std::get<0>(size);
		h = std::get<1>(size);
		if (texture)
		{
			SDL_DestroyTexture(texture);
			texture = nullptr;
		}
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	}

	void copy_from(DrawableArea & other, int x, int y)
	{
		SDL_Rect rect;
		rect.x = x;
		rect.y = y;
		rect.w = other.w;
		rect.h = other.h;
		SDL_SetRenderTarget(renderer, texture);
		SDL_RenderCopy(renderer, other.texture, NULL, &rect);
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
	void refresh_window()
	{
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
	}

	void fill(int r, int g, int b)
	{
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = w;
		rect.h = h;
		SDL_SetRenderTarget(renderer, texture);
	    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	    SDL_RenderFillRect(renderer, &rect);
		SDL_SetRenderTarget(renderer, NULL);
	}
	void draw_line(int x1, int y1, int x2, int y2, int r, int g, int b)
	{
		SDL_SetRenderTarget(renderer, texture);
	    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
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
		SDL_SetRenderTarget(renderer, NULL);
	}
	void draw_3d_rect(int x, int y, int w, int h, int light, int dark, bool sunken)
	{
	    SDL_Rect rect;
	    rect.x = x;
	    rect.y = y;
	    rect.w = w;
	    rect.h = h;

	    if (!sunken)
	    	std::swap(dark, light);

		SDL_SetRenderTarget(renderer, texture);
	    SDL_SetRenderDrawColor(renderer, dark, dark, dark, 255);
	    SDL_RenderDrawLine(renderer, x, y, x+w-1, y);
	    SDL_RenderDrawLine(renderer, x, y, x, y+h-1);
	    SDL_SetRenderDrawColor(renderer, light, light, light, 255);
	    SDL_RenderDrawLine(renderer, x, y+h-1, x+w-1, y+h-1);
	    SDL_RenderDrawLine(renderer, x+w-1, y, x+w-1, y+h-1);
		SDL_SetRenderTarget(renderer, NULL);
	}
	void fill_rect(int x, int y, int w, int h, int r, int g, int b)
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
										window->event_redraw();
										break;
									case SDL_WINDOWEVENT_MOVED:
										break;
									case SDL_WINDOWEVENT_ENTER:
										break;
								}
								break;
							}
						break;
					}
					case SDL_MOUSEMOTION:
						// etc.
						break;
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
								if (e.type == SDL_MOUSEBUTTONDOWN)
									window->event_mouse_button_down(ev.x, ev.y);
								else if (e.type == SDL_MOUSEBUTTONUP)
									window->event_mouse_button_up(ev.x, ev.y);
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
								if (e.type == SDL_KEYDOWN)
									window->event_key_down(ev.keysym.sym);
								else if (e.type == SDL_KEYUP)
									window->event_key_up(ev.keysym.sym);
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