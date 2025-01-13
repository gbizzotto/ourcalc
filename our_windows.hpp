
#pragma once

template<typename WSW>
struct OW
{

	struct Window;

	struct Rect
	{
		int x,y,w,h;
	};

	struct Widget
	{
		Window & w;
		Rect rect;

		Widget(Window & win, Rect r)
			: w(win)
			, rect(r)
		{}

		virtual void event_redraw() {}

		virtual void event_mouse_button_down([[maybe_unused]]int x, [[maybe_unused]]int y) {}
		virtual void event_mouse_button_up  ([[maybe_unused]]int x, [[maybe_unused]]int y) {}
	};

	struct Button : Widget
	{
		bool pressed = false;
		Text text;

		Button(Window & w, Rect r)
			: Widget{w, r}
			, text(" ", &this->w)
		{}
		
		void set_text(std::string s)
		{
			text = Text(s, &this->w);
			event_redraw();
		}

		virtual void event_redraw() override
		{
			int color_bg = 192;
			int color_light = 220;
			int color_dark  = 64;
			int offset = 0;
			if (pressed)
			{
				std::swap(color_light, color_dark);
				offset = 1;
			}

			// Background
			this->w.fill_rect(this->rect.x, this->rect.y, this->rect.w, this->rect.h, color_bg, color_bg, color_bg);
			// Border
			this->w.draw_line(this->rect.x             , this->rect.y             , this->rect.x             , this->rect.y+this->rect.h, color_light, color_light, color_light);
			this->w.draw_line(this->rect.x             , this->rect.y             , this->rect.x+this->rect.w, this->rect.y             , color_light, color_light, color_light);
			this->w.draw_line(this->rect.x+this->rect.w, this->rect.y             , this->rect.x+this->rect.w, this->rect.y+this->rect.h, color_dark , color_dark , color_dark );
			this->w.draw_line(this->rect.x             , this->rect.y+this->rect.h, this->rect.x+this->rect.w, this->rect.y+this->rect.h, color_dark , color_dark , color_dark );
			// Text
			int x = this->rect.x + this->rect.w/2 - text.w/2;
			int y = this->rect.y + this->rect.h/2 - text.h/2;
			text.render(x+offset, y+offset);
		}

		virtual void event_mouse_button_down([[maybe_unused]]int x, [[maybe_unused]]int y)
		{
			if (pressed)
				return;
			pressed = true;
			event_redraw();
		}
		virtual void event_mouse_button_up  ([[maybe_unused]]int x, [[maybe_unused]]int y)
		{
			if ( ! pressed)
				return;
			pressed = false;
			event_clicked();
			event_redraw();
		}

		virtual void event_clicked() {}
	};

	struct Window : public WSW::Window_t
	{
		std::vector<std::unique_ptr<Widget>> widgets;

		template<typename WDG, typename...P>
		WDG & make_widget(Rect r, P...p)
		{
			widgets.emplace_back(std::make_unique<WDG>(*this, r, p...));
			widgets.back()->event_redraw();
			return (WDG&)*widgets.back();
		}

		Widget * find_widget_at(int x, int y)
		{
			for (auto & widget : widgets)
				if (   x >= widget->rect.x && x < widget->rect.x + widget->rect.w
					&& y >= widget->rect.y && y < widget->rect.y + widget->rect.h )
					return &*widget;
			return nullptr;
		}

		virtual void event_redraw()
		{
			this->fill(196,196,196);
			for (auto & widget : widgets)
				widget->event_redraw();
			this->refresh();
		}

		virtual void event_mouse_button_down(int x, int y)
		{
			Widget * widget = find_widget_at(x, y);
			if (widget) {
				widget->event_mouse_button_down(x, y);
				this->refresh();
			}
		}
		virtual void event_mouse_button_up(int x, int y)
		{
			Widget * widget = find_widget_at(x, y);
			if (widget) {
				widget->event_mouse_button_up(x, y);
				this->refresh();
			}
		}
	};

	struct Manager
	{
		WSW window_system_wrapper;

		template<typename W>
		Window & make_window()
		{
			auto & w = window_system_wrapper.template make_window<W>();
			w.event_redraw();
			return w;
		}

		void loop()
		{
			window_system_wrapper.loop();
		}
	};

}; // struct-namespace
