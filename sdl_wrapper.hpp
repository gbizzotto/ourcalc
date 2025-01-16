
#pragma once

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
		SDL_DestroyWindow(sdl_window);
	}

	virtual void event_redraw() {}

	virtual void event_mouse_button_down([[maybe_unused]]int x, [[maybe_unused]]int y) {}
	virtual void event_mouse_button_up  ([[maybe_unused]]int x, [[maybe_unused]]int y) {}

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

	void set_text(std::string s)
	{
		text = s;
		render();
	}

	void render()
	{
		if ( ! text.size())
			return;

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
	    SDL_RenderDrawRect(renderer, &rect);
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
					case SDL_MOUSEBUTTONUP:
					{
						SDL_Window* sdl_window = SDL_GetWindowFromID(((SDL_MouseButtonEvent&)e).windowID);
						if ( ! sdl_window)
							break;
						for(auto & window : windows)
							if (window->sdl_window == sdl_window)
							{
								SDL_MouseButtonEvent & ev = (SDL_MouseButtonEvent&) e;
								window->event_mouse_button_up(ev.x, ev.y);
								break;
							}
						break;
					}
					case SDL_MOUSEBUTTONDOWN:
					{
						SDL_Window* sdl_window = SDL_GetWindowFromID(((SDL_MouseButtonEvent&)e).windowID);
						if ( ! sdl_window)
							break;
						for(auto & window : windows)
							if (window->sdl_window == sdl_window)
							{
								SDL_MouseButtonEvent & ev = (SDL_MouseButtonEvent&) e;
								window->event_mouse_button_down(ev.x, ev.y);
								break;
							}
						break;
					}
					case SDL_QUIT:
						return;
					case SDL_KEYDOWN:
						break;
					case SDL_KEYUP:
						// can also test individual keys, modifier flags, etc, etc.
						break;
					case SDL_MOUSEMOTION:
						// etc.
						break;
				}
			}

			// wait before processing the next frame
			SDL_Delay(10); 
		}
	}
};