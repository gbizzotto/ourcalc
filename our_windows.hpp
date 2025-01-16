
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
		Window * parent_window = nullptr;
		Rect rect;
		WSW::DrawableArea_t drawable_area;

		Widget(Window * window, Rect r)
			: parent_window(window)
			, drawable_area(window, r.w, r.h)
			, rect(r)
		{}

		virtual void event_redraw() {}

		virtual void event_mouse_button_down([[maybe_unused]]int x, [[maybe_unused]]int y) {}
		virtual void event_mouse_button_up  ([[maybe_unused]]int x, [[maybe_unused]]int y) {}
	};
	
	struct Label : Widget
	{
		Text caption;

		Label(Window * parent_window, std::string text, Rect r)
			: Widget(parent_window, r)
			, caption(std::move(text), parent_window)
		{}

		virtual void event_redraw() override
		{
			caption.render();
			int y = (this->rect.h - caption.h) / 2;
			this->drawable_area.copy_from(caption, 0, y);
		}
	};

	struct Button : Widget
	{
		bool pressed = false;
		Text caption;

		Button(Window * parent_window, std::string t, Rect r)
			: Widget(parent_window, r)
			, caption(t, parent_window)
		{}

		void set_text(std::string s)
		{
			caption.set_text(s);
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
			this->drawable_area.fill(color_bg, color_bg, color_bg);
			// Border
			this->drawable_area.draw_line(0               , 0               , 0             , this->rect.h-1, color_light, color_light, color_light);
			this->drawable_area.draw_line(0               , 0               , this->rect.w-1, 0             , color_light, color_light, color_light);
			this->drawable_area.draw_line(0+this->rect.w-1, 0               , this->rect.w-1, this->rect.h-1, color_dark , color_dark , color_dark );
			this->drawable_area.draw_line(0               , 0+this->rect.h-1, this->rect.w-1, this->rect.h-1, color_dark , color_dark , color_dark );
			// Text
			caption.render();
			int x = (this->rect.w - caption.w) / 2 + offset;
			int y = (this->rect.h - caption.h) / 2 + offset;
			this->drawable_area.copy_from(caption, x, y);
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


	struct TextEdit : Widget
	{
		Text caption;
		static const int padding = 5;

		TextEdit(Window * parent_window, std::string t, Rect r)
			: Widget(parent_window, r)
			, caption(t, parent_window)
		{}

		void set_text(std::string s)
		{
			caption.set_text(s);
			event_redraw();
		}

		virtual void event_redraw() override
		{
			int color_bg = 255;
			int color_light = 220;
			int color_dark  = 64;

			// Background
			this->drawable_area.fill(color_bg, color_bg, color_bg);
			// Border
			this->drawable_area.draw_line(0               , 0             , 0             , this->rect.h-1, color_dark , color_dark , color_dark );
			this->drawable_area.draw_line(0               , 0             , this->rect.w-1, 0             , color_dark , color_dark , color_dark );
			this->drawable_area.draw_line(0+this->rect.w-1, 0             , this->rect.w-1, this->rect.h-1, color_light, color_light, color_light);
			this->drawable_area.draw_line(0               , this->rect.h-1, this->rect.w-1, this->rect.h-1, color_light, color_light, color_light);
			// Text
			caption.render();
			int x = padding;
			int y = (this->rect.h - caption.h) / 2;
			this->drawable_area.copy_from(caption, x, y);
		}
	};

	struct Layout
	{
		std::vector<Widget*> widgets;
		Window * window;
		Rect rect;

		Layout(Window * w, Rect r)
			: window(w)
			, rect(r)
		{}

		void add_widget(Widget * widget)
		{
			widgets.push_back(widget);
			//widget->event_redraw();
			//this->window->drawable_area.copy_from(widget->drawable_area, widget->rect.x, widget->rect.y);
			//this->window->present();
		}

		Widget * find_widget_at(int x, int y)
		{
			for (Widget * widget : widgets)
				if (   x >= widget->rect.x && x < widget->rect.x + widget->rect.w
					&& y >= widget->rect.y && y < widget->rect.y + widget->rect.h )
					return &*widget;
			return nullptr;
		}

		void event_redraw()
		{
			this->window->drawable_area.fill(196,196,196);
			for (Widget *  widget : widgets) {
				widget->event_redraw();
				this->window->drawable_area.copy_from(widget->drawable_area, widget->rect.x, widget->rect.y);
			}
		}
	};
	struct VLayout : Layout
	{
		int next_y = 0;

		VLayout(Window * w, Rect r)
			: Layout(w, r)
		{}
		VLayout(Layout && other)
			: Layout(other.window, other.rect)
		{
			for (Widget * widget : other.widgets)
				add_widget(widget);
		}

		void add_widget(Widget * widget)
		{
			widget->rect.y = next_y;
			next_y += widget->rect.h;
			widget->rect.x = (this->window->w - widget->rect.w) / 2;
			this->widgets.push_back(widget);
			//widget->event_redraw();
			//this->window->drawable_area.copy_from(widget->drawable_area, widget->rect.x, widget->rect.y);
			//this->window->drawable_area.refresh_window();
			//this->window->present();
		}

		Widget * find_widget_at(int x, int y)
		{
			for (Widget * widget : this->widgets)
				if (   x >= widget->rect.x && x < widget->rect.x + widget->rect.w
					&& y >= widget->rect.y && y < widget->rect.y + widget->rect.h )
					return &*widget;
			return nullptr;
		}

		void event_redraw()
		{
			this->window->drawable_area.fill(196,196,196);
			for (Widget *  widget : this->widgets) {
				widget->event_redraw();
				this->window->drawable_area.copy_from(widget->drawable_area, widget->rect.x, widget->rect.y);
			}
		}
	};

	struct Window : public WSW::Window_t
	{
		int w, h;
		std::unique_ptr<Layout> layout;
		DrawableArea drawable_area;

		Window(const char * title, int width, int height)
			: WSW::Window_t(title, width, height)
			, w(width)
			, h(height)
			, layout(std::make_unique<Layout>(this, Rect{0, 0, width, height}))
			, drawable_area(this, width, height)
		{}

		template<typename L>
		void set_layout_type()
		{
			layout = std::make_unique<L>(std::move(*layout));
			event_redraw();
		}

		virtual void event_redraw()
		{
			layout->event_redraw();
			drawable_area.refresh_window();
			WSW::Window_t::present();
		}

		virtual void event_mouse_button_down(int x, int y)
		{
			Widget * widget = layout->find_widget_at(x, y);
			if (widget) {
				widget->event_mouse_button_down(x, y);
				this->event_redraw();
			}
		}
		virtual void event_mouse_button_up(int x, int y)
		{
			Widget * widget = layout->find_widget_at(x, y);
			if (widget) {
				widget->event_mouse_button_up(x, y);
				this->event_redraw();
			}
		}
	};

	struct Manager
	{
		WSW window_system_wrapper;

		template<typename W>
		Window & make_window(const char * title="", int width=1280, int height=1024)
		{
			auto & w = window_system_wrapper.template make_window<W>(title, width, height);
			w.event_redraw();
			return w;
		}

		void loop()
		{
			window_system_wrapper.loop();
		}
	};

}; // struct-namespace
