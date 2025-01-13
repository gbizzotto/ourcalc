
#pragma once

#include <vector>
#include <memory>
#include <SDL2/SDL.h>


struct Window
{
	SDL_Surface* winSurface = NULL;
	SDL_Window* sdl_window = NULL;
	SDL_Renderer* renderer = NULL;
	
	Window()
	{
		// Create our window
		sdl_window = SDL_CreateWindow( "Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN );

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
	}
	~Window()
	{
		// This will also destroy the surface
		SDL_DestroyWindow(sdl_window);
	}

	void fill(int r, int g, int b)
	{
	    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	    SDL_RenderFillRect(renderer, nullptr);
	    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	    SDL_RenderPresent(renderer);
	}

	virtual void event_redraw() {}

	virtual void event_mouse_button_down() {}
	virtual void event_mouse_button_up() {}

	void draw_rect(int x, int y, int w, int h, int r, int g, int b)
	{
	    SDL_Rect rect;
	    rect.x = x;
	    rect.y = y;
	    rect.w = w;
	    rect.h = h;

	    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	    SDL_RenderDrawRect(renderer, &rect);
	    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	    SDL_RenderPresent(renderer);
	}
};

struct SDL
{
	using Window_t = Window;

	std::vector<std::unique_ptr<Window_t>> windows;

	SDL()
	{
		// Initialize SDL. SDL_Init will return -1 if it fails.
		if ( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
			throw;
	}
	~SDL()
	{
		// Quit SDL
		SDL_Quit();	
	}

	template<typename W>
	W & make_window()
	{
		windows.emplace_back(std::make_unique<W>());
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
								window->event_mouse_button_up();
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
								window->event_mouse_button_down();
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