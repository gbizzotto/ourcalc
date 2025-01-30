
#pragma once

#include <iostream>
#include "util.hpp"

enum class widget_change_t
{
	all = 0,
	resized,
	moved,
	redrawn,
};


template<typename WSW>
struct OW
{

	struct Window;
	struct Container;

	struct Rect
	{
		int x,y,w,h;
	};

	struct Widget : monitorable<widget_change_t>
	{
		Container * parent_container;
		Rect rect;
		WSW::DrawableArea_t drawable_area;

		int border_width = 1;
		bool border_is_sunken = false;
		int border_color_light = 220;
		int border_color_dark  =  64;
		int border_color_black =   0;

		int padding = 2;

		Widget(Container * container, Rect r)
			: parent_container(container)
			, rect(r)
			, drawable_area(container->parent_window, r.w, r.h)
		{
			if (parent_container)
				parent_container->add_widget(this, false);
		}
		Widget(Window * window, Rect r)
			: rect(r)
			, drawable_area(window, r.w, r.h)
		{}

		int w() const { return this->rect.w; }
		int h() const { return this->rect.h; }

		bool has_focus()
		{
			if (parent_container)
				return parent_container->has_focus(this);
			return
				false;
		}
		void take_focus()
		{
			if (parent_container)
				parent_container->set_focus(this);
		}

		virtual void draw_border()
		{
			if (border_width == 0)
				return;
			if (has_focus())
				this->drawable_area.draw_3d_rect(0, 0, rect.w-1, rect.h-1, border_color_black, border_color_black, border_is_sunken);
			else
				this->drawable_area.draw_3d_rect(0, 0, rect.w-1, rect.h-1, border_color_light, border_color_dark, border_is_sunken);
		}

		virtual void pack() {}
		virtual void set_size(std::pair<int,int> size)
		{
			if (rect.w == std::get<0>(size) && rect.h == std::get<1>(size))
				return;
			rect.w = std::get<0>(size);
			rect.h = std::get<1>(size);
			drawable_area.set_size(size);
			redraw();
			notify_monitors(widget_change_t::resized);
		}
		virtual bool focusable() { return true; }
		virtual void redraw() {}

		virtual bool event_mouse_button_down([[maybe_unused]]int x, [[maybe_unused]]int y) { return false; }
		virtual bool event_mouse_button_up  ([[maybe_unused]]int x, [[maybe_unused]]int y) { return false; }
		virtual bool event_key_down([[maybe_unused]]int key) { return false; }
		virtual bool event_key_up  ([[maybe_unused]]int key) { return false; }
	};
	
	struct Layout
	{
		virtual void rearrange_widgets(const Container &, std::vector<Widget*> &, [[maybe_unused]]int w, [[maybe_unused]]int h)
		{}
	};

	struct Container : Widget, monitor<widget_change_t>
	{
		Window * parent_window;
		std::vector<Widget*> widgets;
		unsigned int focus_idx = 0;
		std::unique_ptr<Layout> layout;

		Container(Window * window, Rect r)
			: Widget(window, r)
			, parent_window(window)
			, layout(std::make_unique<Layout>())
		{
			this->border_width = 0;
		}
		Container(Container * container, Rect r)
			: Widget(container, r)
			, parent_window(container->parent_window)
			, layout(std::make_unique<Layout>())
		{
			this->border_width = 0;
		}
		~Container()
		{
			for (Widget * widget : this->widgets)
				widget->remove_monitor(this);
		}

		void set_layout(std::unique_ptr<Layout> new_layout)
		{
			layout = std::move(new_layout);
			layout->rearrange_widgets(*this, widgets, this->rect.w, this->rect.h);
			redraw();
		}

		void add_widget(Widget * widget, bool do_redraw = false)
		{
			widgets.push_back(widget);
			widget->add_monitor(this);
			layout->rearrange_widgets(*this, widgets, this->rect.w, this->rect.h);
			if (do_redraw)
				redraw();
		}

		virtual void notify(monitorable<widget_change_t> *, widget_change_t & ev) override
		{
			if (ev == widget_change_t::resized)
			{
				layout->rearrange_widgets(*this, widgets, this->rect.w, this->rect.h);
				redraw();
			}
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
					return;
				}
			focus_idx = 0;

		}

		virtual void redraw()
		{
			this->drawable_area.fill(196,196,196);
			for (Widget *  widget : widgets) {
				widget->redraw();
				this->drawable_area.copy_from(widget->drawable_area, widget->rect.x, widget->rect.y);
			}
			this->draw_border();
		}

		Widget * find_widget_at(int x, int y)
		{
			for (Widget * widget : widgets)
				if (   x >= widget->rect.x && x < widget->rect.x + widget->rect.w
					&& y >= widget->rect.y && y < widget->rect.y + widget->rect.h )
					return &*widget;
			return nullptr;
		}

		virtual bool event_mouse_button_down(int x, int y) override
		{
			Widget * target = find_widget_at(x, y);
			if (target == nullptr)
			{
				return false;
			}
			else if (target->event_mouse_button_down(x-target->rect.x, y-target->rect.y))
			{
				redraw();
				return true;
			}
			else
				return false;
		}
		virtual bool event_mouse_button_up(int x, int y) override
		{
			Widget * target = find_widget_at(x, y);
			if (target == nullptr)
			{
				return false;
			}
			else if (target->event_mouse_button_up(x-target->rect.x, y-target->rect.y))
			{
				redraw();
				return true;
			}
			else
				return false;
		}
	};





	struct Label : Widget
	{
		Text caption;

		Label(Container * container, std::string text, Rect r)
			: Widget(container, r)
			, caption(std::move(text), container->parent_window)
		{}

		virtual bool focusable() override { return false; }
		virtual void redraw() override
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
		bool is_horizontal;
		int split_position;
		Container one, two;

		Splitter(Container * container, Rect r, bool is_horizontal)
			: Widget(container, r)
			, is_horizontal(is_horizontal)
			, split_position((is_horizontal?r.h:r.w)/2)
			, one(container->parent_window, Rect{                                  0,                                   0, is_horizontal?r.w:((r.w-thickness)/2), is_horizontal?((r.h-thickness)/2):r.h})
			, two(container->parent_window, Rect{is_horizontal?0:((r.w+thickness)/2), is_horizontal?((r.h+thickness)/2):0, is_horizontal?r.w:((r.w-thickness)/2), is_horizontal?((r.h-thickness)/2):r.h})
		{
			one.border_width = 1;
			two.border_width = 1;
			one.border_is_sunken = true;
			two.border_is_sunken = true;
			this->border_width = 0;
		}

		virtual bool focusable() override { return false; }
		virtual void redraw() override
		{
			one.redraw();
			two.redraw();

			this->drawable_area.copy_from(one.drawable_area, 0, 0);
			if (is_horizontal)
				this->drawable_area.copy_from(two.drawable_area, 0, (this->rect.h+thickness)/2);
			else
				this->drawable_area.copy_from(two.drawable_area, (this->rect.w+thickness)/2, 0);
		}

		/*virtual void pack() 
		{
			this->set_size(caption.get_size());
		}*/

		virtual bool event_mouse_button_down(int x, int y) override
		{
			bool processed = false;
			if (is_horizontal)
			{
				if (y < this->rect.h/2)
					processed = one.event_mouse_button_down(x - one.rect.x, y - one.rect.y);
				else
					processed = two.event_mouse_button_down(x - two.rect.x, y - two.rect.y);
			}
			else
			{
				if (x < this->rect.w/2)
					processed = one.event_mouse_button_down(x - one.rect.x, y - one.rect.y);
				else
					processed = two.event_mouse_button_down(x - two.rect.x, y - two.rect.y);
			}
			if (processed)
				redraw();
			return processed;
		}
		virtual bool event_mouse_button_up(int x, int y) override
		{
			bool processed = false;
			if (is_horizontal)
			{
				if (y < this->rect.h/2)
					processed = one.event_mouse_button_up(x - one.rect.x, y - one.rect.y);
				else
					processed = two.event_mouse_button_up(x - two.rect.x, y - two.rect.y);
			}
			else
			{
				if (x < this->rect.w/2)
					processed = one.event_mouse_button_up(x - one.rect.x, y - one.rect.y);
				else
					processed = two.event_mouse_button_up(x - two.rect.x, y - two.rect.y);
			}
			if (processed)
				redraw();
			return processed;
		}
	};

	struct Button : Widget
	{
		bool pressed = false;
		Text caption;

		Button(Container * container, std::string t, Rect r)
			: Widget(container, r)
			, caption(t, container->parent_window)
		{
			this->border_is_sunken = false;
		}

		void set_text(std::string s)
		{
			caption.set_text(s);
			redraw();
		}

		virtual void pack()
		{
			this->set_size({2*this->border_width + 2*this->padding + caption.w, 2*this->border_width + 2*this->padding + caption.h});
		}

		virtual void redraw() override
		{			
			int color_bg = 192;
			int offset = 0;
			if (pressed)
			{
				offset = 1;
			}

			// Background
			this->drawable_area.fill(color_bg, color_bg, color_bg);
			// Border
			this->draw_border();
			// Text
			caption.render();
			int x = (this->rect.w - caption.w) / 2 + offset;
			int y = (this->rect.h - caption.h) / 2 + offset;
			this->drawable_area.copy_from(caption, x, y);
		}

		virtual bool event_mouse_button_down([[maybe_unused]]int x, [[maybe_unused]]int y) override
		{
			if (pressed)
				return false;
			pressed = true; 
			this->border_is_sunken = true;
			this->take_focus();
			redraw();
			return true;
		}
		virtual bool event_mouse_button_up  ([[maybe_unused]]int x, [[maybe_unused]]int y) override
		{
			if ( ! pressed)
				return false;
			pressed = false;
			this->border_is_sunken = false;
			event_clicked();
			redraw();
			return true;
		}

		virtual void event_clicked() {}
	};


	struct TextEdit : Widget
	{
		Text caption;
		int text_x;
		int text_y;	
		unsigned int char_pos = 0;
		unsigned int cursor_x = 0;
	
		TextEdit(Container * container, std::string t, Rect r)
			: Widget(container, r)
			, caption(t, container->parent_window)
		{
			this->padding = 5;
		}

		void set_text(std::string s)
		{
			caption.set_text(s);
			redraw();
		}

		virtual void pack()
		{
			this->set_size({2*this->border_width + 2*this->padding + caption.w, 2*this->border_width + 2*this->padding + caption.h});
		}

		virtual void redraw() override
		{
			int color_bg = 255;
			int color_light = 220;
			int color_dark  = 64;

			if (this->has_focus())
				color_light = color_dark = 0;

			// Background
			this->drawable_area.fill(color_bg, color_bg, color_bg);
			// Border
			this->drawable_area.draw_3d_rect(0, 0, this->rect.w, this->rect.h, color_light, color_dark, true);
			// Text
			//caption.render();
			text_x = this->padding;
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
			char_pos = caption.get_pos_at(x - text_x);
			cursor_x = text_x + caption.get_char_x(char_pos);
			redraw();
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
				redraw();
				return true;
			}
			return false;
		}
	};


	struct VLayout : Layout
	{
		virtual void rearrange_widgets(const Container & container, std::vector<Widget*> & widgets, [[maybe_unused]]int w, [[maybe_unused]]int h) override
		{
			// reorganize widgets
			int next_y = container.border_width + container.padding;
			for (Widget * widget : widgets)
			{
				widget->rect.y = next_y;
				next_y += widget->rect.h;
				widget->rect.x = (w - widget->rect.w) / 2;
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
		Container container;

		Window(const char * title, int width, int height)
			: WSW::Window_t(title, width, height)
			, container(this, Rect{0, 0, width, height})
		{}

		virtual void redraw() override
		{
			container.redraw();
			container.drawable_area.refresh_window();
			WSW::Window_t::present();
		}	

		virtual bool event_mouse_button_down(int x, int y)
		{
			Widget * widget = this->container.find_widget_at(x, y);
			if (widget && widget->event_mouse_button_down(x-widget->rect.x, y-widget->rect.y))
			{
				this->redraw();
				return true;
			}
			return false;
		}
		virtual bool event_mouse_button_up(int x, int y)
		{
			Widget * widget = this->container.find_widget_at(x, y);
			if (widget && widget->event_mouse_button_up(x-widget->rect.x, y-widget->rect.y))
			{
				this->redraw();
				return true;
			}
			return false;
		}
		virtual bool event_key_down(int key)
		{
			Widget * widget = this->container.get_focused();
			if (widget && widget->event_key_down(key))
			{
				this->redraw();
				return true;
			}
			// not processed by a widget
			if (key == SDLK_TAB)
			{
				bool handled = this->container.focus_next();
				this->redraw();
				return handled;
			}

			return false;
		}
		virtual bool event_key_up(int key)
		{
			Widget * widget = this->container.get_focused();
			if (widget && widget->event_key_up(key))
			{
				this->redraw();
				return true;
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
			w.redraw();
			return w;
		}

		void loop()
		{
			window_system_wrapper.loop();
		}
	};

}; // struct-namespace
