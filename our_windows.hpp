
#pragma once

#include <iostream>
#include "util.hpp"

enum class widget_change_t
{
	resized = 0,
	moved,
};


template<typename WSW>
struct OW
{

	struct Window;

	struct Rect
	{
		int x,y,w,h;
	};

	struct Widget : monitorable<widget_change_t>
	{
		Window * parent_window = nullptr;
		Rect rect;
		WSW::DrawableArea_t drawable_area;

		Widget(Window * window, Rect r)
			: parent_window(window)
			, rect(r)
			, drawable_area(window, r.w, r.h)
		{}

		bool has_focus()
		{
			return parent_window->layout_container->has_focus(this);
		}
		void take_focus()
		{
			parent_window->layout_container->set_focus(this);
		}

		virtual void pack() {}
		virtual void set_size(std::pair<int,int> size)
		{
			rect.w = std::get<0>(size);
			rect.h = std::get<1>(size);
			drawable_area.set_size(size);
			event_redraw();
		}
		virtual bool focusable() { return true; }
		virtual void event_redraw() {}

		virtual bool event_mouse_button_down([[maybe_unused]]int x, [[maybe_unused]]int y) { return false; }
		virtual bool event_mouse_button_up  ([[maybe_unused]]int x, [[maybe_unused]]int y) { return false; }
		virtual bool event_key_down([[maybe_unused]]int key) { return false; }
		virtual bool event_key_up  ([[maybe_unused]]int key) { return false; }
	};
	
	struct LayoutContainer
	{
		Window * parent_window;
		std::vector<Widget*> widgets;
		DrawableArea drawable_area;
		unsigned int focus_idx = 0;

		LayoutContainer(Window * window, int width, int height)
			: parent_window(window)
			, drawable_area(window, width, height)
		{}

		void add_widget(Widget * widget)
		{
			widgets.push_back(widget);
		}

		Widget * get_focused()
		{
			if (focus_idx >= widgets.size())
				return nullptr;
			return widgets[focus_idx];
		}
		bool focus_next()
		{
			focus_idx = (focus_idx+1) % widgets.size();
			return true;
		}

		bool has_focus(Widget * widg)
		{
			return widg == get_focused();
		}
		void set_focus(Widget * widg)
		{
			for (unsigned int i=0 ; i<widgets.size() ; i++)
				if (widgets[i] == widg)
				{
					focus_idx = i;
					break;
				}
		}

		virtual void event_redraw()
		{
			drawable_area.fill(196,196,196);
			for (Widget *  widget : widgets) {
				widget->event_redraw();
				drawable_area.copy_from(widget->drawable_area, widget->rect.x, widget->rect.y);
			}
		}

		Widget * find_widget_at(int x, int y)
		{
			for (Widget * widget : widgets)
				if (   x >= widget->rect.x && x < widget->rect.x + widget->rect.w
					&& y >= widget->rect.y && y < widget->rect.y + widget->rect.h )
					return &*widget;
			return nullptr;
		}
	};





	struct Label : Widget
	{
		Text caption;

		Label(Window * parent_window, std::string text, Rect r)
			: Widget(parent_window, r)
			, caption(std::move(text), parent_window)
		{}

		virtual bool focusable() override { return false; }
		virtual void event_redraw() override
		{
			caption.render();
			int y = (this->rect.h - caption.h) / 2;
			this->drawable_area.copy_from(caption, 0, y);
		}

		virtual void pack() 
		{
			this->set_size(caption.get_size());
		}
	};

	struct Splitter : Widget
	{
		int thickness = 6;
		int border = 1;
		bool is_horizontal;
		int split_position;
		std::unique_ptr<LayoutContainer> one, two;

		Splitter(Window * parent_window, Rect r, bool is_horizontal)
			: Widget(parent_window, r)
			, is_horizontal(is_horizontal)
			, split_position((is_horizontal?r.h:r.w)/2)
			, one(std::make_unique<LayoutContainer>(parent_window, (r.w-4*border-thickness) / (is_horizontal?1:2), (r.h-4*border-thickness) / (is_horizontal?2:1)))
			, two(std::make_unique<LayoutContainer>(parent_window, (r.w-4*border-thickness) / (is_horizontal?1:2), (r.h-4*border-thickness) / (is_horizontal?2:1)))
		{}

		virtual bool focusable() override { return false; }
		virtual void event_redraw() override
		{
			one->event_redraw();
			two->event_redraw();

			int color_light = 220;
			int color_dark  = 64;

			this->drawable_area.draw_line(0               , 0               , 0             , this->rect.h-1, color_dark , color_dark , color_dark );
			this->drawable_area.draw_line(0               , 0               , this->rect.w-1, 0             , color_dark , color_dark , color_dark );
			this->drawable_area.draw_line(0+this->rect.w-1, 0               , this->rect.w-1, this->rect.h-1, color_light, color_light, color_light);
			this->drawable_area.draw_line(0               , 0+this->rect.h-1, this->rect.w-1, this->rect.h-1, color_light, color_light, color_light);

			if (is_horizontal)
			{
				this->drawable_area.draw_line(1, split_position - thickness/2, this->rect.w, split_position - thickness/2, color_light, color_light, color_light);
				this->drawable_area.draw_line(1, split_position + thickness/2, this->rect.w, split_position + thickness/2, color_dark	, color_dark , color_dark );
				this->drawable_area.copy_from(one->drawable_area, border, border);
				this->drawable_area.copy_from(two->drawable_area, border, border + one->drawable_area.h + border + thickness + border);
			}
			else
			{
				this->drawable_area.copy_from(one->drawable_area, thickness + one->drawable_area.w, 0);
			}
		}

		template<typename L>
		void set_layout_type(std::unique_ptr<LayoutContainer> & layout_container)
		{
			layout_container = std::make_unique<L>(std::move(*layout_container));
			event_redraw();
		}

		/*virtual void pack() 
		{
			this->set_size(caption.get_size());
		}*/
	};

	struct Button : Widget
	{
		bool pressed = false;
		Text caption;
		int padding = 2;
		int border = 1;

		Button(Window * parent_window, std::string t, Rect r)
			: Widget(parent_window, r)
			, caption(t, parent_window)
		{}

		void set_text(std::string s)
		{
			caption.set_text(s);
			event_redraw();
		}

		virtual void pack()
		{
			this->set_size({2*border + 2*padding + caption.w, 2*border + 2*padding + caption.h});
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
			if (this->has_focus())
				color_light = color_dark = 0;

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

		virtual bool event_mouse_button_down([[maybe_unused]]int x, [[maybe_unused]]int y)
		{
			if (pressed)
				return false;
			pressed = true;
			this->take_focus();
			event_redraw();
			return true;
		}
		virtual bool event_mouse_button_up  ([[maybe_unused]]int x, [[maybe_unused]]int y)
		{
			if ( ! pressed)
				return false;
			pressed = false;
			event_clicked();
			event_redraw();
			return true;
		}

		virtual void event_clicked() {}
	};


	struct TextEdit : Widget
	{
		Text caption;
		int padding = 5;
		int border = 1;
		int text_x;
		int text_y;	
		unsigned int char_pos = 0;
		unsigned int cursor_x = 0;
	
		TextEdit(Window * parent_window, std::string t, Rect r)
			: Widget(parent_window, r)
			, caption(t, parent_window)
		{
			event_redraw();
		}

		void set_text(std::string s)
		{
			caption.set_text(s);
			event_redraw();
		}

		virtual void pack()
		{
			this->set_size({2*border + 2*padding + caption.w, 2*border + 2*padding + caption.h});
		}

		virtual void event_redraw() override
		{
			int color_bg = 255;
			int color_light = 220;
			int color_dark  = 64;

			if (this->has_focus())
				color_light = color_dark = 0;

			// Background
			this->drawable_area.fill(color_bg, color_bg, color_bg);
			// Border
			this->drawable_area.draw_line(0               , 0             , 0             , this->rect.h-1, color_dark , color_dark , color_dark );
			this->drawable_area.draw_line(0               , 0             , this->rect.w-1, 0             , color_dark , color_dark , color_dark );
			this->drawable_area.draw_line(0+this->rect.w-1, 0             , this->rect.w-1, this->rect.h-1, color_light, color_light, color_light);
			this->drawable_area.draw_line(0               , this->rect.h-1, this->rect.w-1, this->rect.h-1, color_light, color_light, color_light);
			// Text
			//caption.render();
			text_x = padding;
			text_y = (this->rect.h - caption.h) / 2;
			this->drawable_area.copy_from(caption, text_x, text_y);

			// cursor
			if (this->has_focus())
			{
				this->drawable_area.draw_line(cursor_x, text_y, cursor_x, text_y + caption.h, 0, 0, 0);
			}
		}

		virtual bool event_mouse_button_down([[maybe_unused]]int x, [[maybe_unused]]int y) override
		{
			this->take_focus();
			char_pos = caption.get_pos_at(x - this->rect.x - text_x);
			cursor_x = text_x + caption.get_char_x(char_pos);
			return true;
		}

		virtual bool event_key_down(int key) override
		{
			bool caption_changed = false;
			bool cursor_changed = false;
			auto str = caption.get_text();
			if (key == '\b')
			{
				// backspace
				if (char_pos == 0)
					return true;
				str.erase(std::begin(str)+(char_pos-1), std::begin(str)+char_pos);
				--char_pos;
				caption_changed = true;
				cursor_changed = true;
			}
			else if (key == 127)
			{
				// suppress
				if (char_pos == caption.text.size())
					return true;
				str.erase(std::begin(str)+(char_pos), std::begin(str)+(char_pos+1));
				caption_changed = true;
			}
			else if (key == '\t')
			{
				// tab
				return false;
			}
			else if (key == 1)
			{
				// up, do nothing
			}
			else if (key == 2)
			{
				// right, do nothing
			}
			else if (key == 3)
			{
				// left
				if (char_pos == 0)
					return true;
				--char_pos;
				cursor_changed = true;
			}
			else if (key == 4)
			{
				// left
				if (char_pos == caption.text.size())
					return true;
				++char_pos;
				cursor_changed = true;
			}
			else
			{
				// printable char
				str.insert(str.begin()+char_pos, key);
				++char_pos;
				caption_changed = true;
				cursor_changed = true;
			}

			if (caption_changed)
				caption.set_text(str);
			if (cursor_changed)
				cursor_x = text_x + caption.get_char_x(char_pos);

			if (caption_changed || cursor_changed)
			{
				event_redraw();
				return true;
			}
			return false;
		}
	};


	struct VLayout : LayoutContainer
	{
		int next_y = 0;

		VLayout(Window * w, Rect r)
			: LayoutContainer(w, r)
		{}
		VLayout(LayoutContainer && other)
			: LayoutContainer(other.parent_window, other.drawable_area.w, other.drawable_area.h)
		{
			for (Widget * widget : other.widgets)
				add_widget(widget);
		}

		void add_widget(Widget * widget)
		{
			widget->rect.y = next_y;
			next_y += widget->rect.h;
			widget->rect.x = (this->drawable_area.w - widget->rect.w) / 2;
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
			this->drawable_area.fill(196,196,196);
			for (Widget *  widget : this->widgets) {
				widget->event_redraw();
				this->drawable_area.copy_from(widget->drawable_area, widget->rect.x, widget->rect.y);
			}
		}
	};

	struct Window : public WSW::Window_t
	{
		/*
		int w, h;
		std::unique_ptr<Layout> layout;
		DrawableArea drawable_area;
		*/
		std::unique_ptr<LayoutContainer> layout_container;

		Window(const char * title, int width, int height)
			: WSW::Window_t(title, width, height)
			, layout_container(std::make_unique<LayoutContainer>(this, width, height))
		{}

		virtual void event_redraw() override
		{
			layout_container->event_redraw();
			layout_container->drawable_area.refresh_window();
			WSW::Window_t::present();
		}	

		template<typename L>
		void set_layout_type()
		{
			layout_container = std::make_unique<L>(std::move(*layout_container));
			event_redraw();
		}

		virtual bool event_mouse_button_down(int x, int y)
		{
			Widget * widget = this->layout_container->find_widget_at(x, y);
			if (widget) {
				if (widget->event_mouse_button_down(x, y))
				{
					this->event_redraw();
					return true;
				}
			}
			return false;
		}
		virtual bool event_mouse_button_up(int x, int y)
		{
			Widget * widget = this->layout_container->find_widget_at(x, y);
			if (widget) {
				if (widget->event_mouse_button_up(x, y))
				{
					this->event_redraw();
					return true;
				}
			}
			return false;
		}
		virtual bool event_key_down(int key)
		{
			Widget * widget = this->layout_container->get_focused();
			if (widget) {
				if (widget->event_key_down(key))
				{
					this->event_redraw();
					return true;
				}
			}
			// not processed by a widget
			if (key == SDLK_TAB)
			{
				bool handled = this->layout_container->focus_next();
				this->event_redraw();
				return handled;
			}

			return false;
		}
		virtual bool event_key_up(int key)
		{
			Widget * widget = this->layout_container->get_focused();
			if (widget) {
				if (widget->event_key_up(key))
				{
					this->event_redraw();
					return true;
				}
			}
			return false;
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
