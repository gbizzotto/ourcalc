
#pragma once

#include <iostream>
#include "events.hpp"
#include "util.hpp"

enum class widget_change_t
{
	any = 0,
	resized,
	moved,
	redrawn,
	focus_changed,
};

enum class horizontal_alignment_t
{
	none = 0,
	center,
	left,
	right,
	justify,
	fill,
};
enum class vertical_alignment_t
{
	none = 0,
	center,
	top,
	bottom,
	justify,
	fill,
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
		Container * parent_container = nullptr;
		Rect rect;
		WSW::DrawableArea_t drawable_area;
		bool _focus = false;
		bool _mouse_grabbed = false;

		int border_width = 1;
		bool border_is_sunken = false;
		int border_color_light = 220;
		int border_color_dark  =  64;
		int border_color_black =   0;
		int color_bg = 192;

		int padding = 2;

		Widget(Window * window, Rect r)
			: rect(r)
			, drawable_area(window, r.w, r.h)
		{}

		int w() const { return this->rect.w; }
		int h() const { return this->rect.h; }

		bool has_focus()
		{
			return _focus;
		}
		bool has_mouse()
		{
			return _mouse_grabbed;
		}
		void take_focus()
		{
			if (parent_container)
				parent_container->set_focus(this);
		}
		void set_focus(bool has)
		{
			if (has != _focus)
			{
				_focus = has;
				this->_redraw();
			}
		}
		void set_mouse_grabbed(bool grabbed)
		{
			_mouse_grabbed = grabbed;
		}
		void mouse_grab()
		{
			if (parent_container)
				parent_container->mouse_grab(this);
		}
		void mouse_release()
		{
			if (parent_container)
				parent_container->mouse_release(this);
		}

		virtual void clear_background()
		{
			this->drawable_area.fill(this->color_bg, this->color_bg, this->color_bg);
		}
		virtual void draw_border()
		{
			if (border_width == 0)
				return;
			if (has_focus())
				this->drawable_area.draw_3d_rect(0, 0, rect.w, rect.h, border_color_black, border_color_black, border_is_sunken);
			else
				this->drawable_area.draw_3d_rect(0, 0, rect.w, rect.h, border_color_light, border_color_dark, border_is_sunken);
		}

		virtual bool pack()
		{
			return vpack() | hpack();
		}
		virtual bool vpack() { return false; }
		virtual bool hpack() { return false; }
		virtual bool set_size(std::pair<int,int> size)
		{
			if (rect.w == std::get<0>(size) && rect.h == std::get<1>(size))
				return false;
			rect.w = std::get<0>(size);
			rect.h = std::get<1>(size);
			drawable_area.set_size(size);
			_redraw();
			notify_monitors(widget_change_t::resized);
			return true;
		}
		virtual bool focusable() { return true; }
		virtual int height_packed() =0;
		virtual void _redraw() {}
		virtual bool can_vfill() { return true; }

		virtual bool handle_event(event &) { return false; }
	};
	
	struct Layout
	{
		virtual bool rearrange_widgets(const Container &, [[maybe_unused]]int w, [[maybe_unused]]int h) { return false; }
		virtual bool vpack(Container &) { return false; }
		virtual bool hpack(Container &) { return false; }
	};

	struct Container : Widget, monitor<widget_change_t>
	{
		Window * parent_window;
		std::vector<Widget*> widgets;
		std::unique_ptr<Layout> layout;

		Container(Window * window, Rect r)
			: Widget(window, r)
			, parent_window(window)
			, layout(std::make_unique<Layout>())
		{
			this->border_width = 0;
			_redraw();
		}
		/*Container(Container * container, Rect r)
			: Widget(container, r)
			, parent_window(container->parent_window)
			, layout(std::make_unique<Layout>())
		{
			this->border_width = 0;
			_redraw();
		}*/
		~Container()
		{
			for (Widget * widget : this->widgets)
				widget->remove_monitor(this);
		}

		virtual bool set_size(std::pair<int,int> size) override
		{
			if (std::get<0>(size) == this->rect.w && std::get<1>(size) == this->rect.h)
				return false;
			return layout->rearrange_widgets(*this, size.first, size.second)
				| Widget::set_size(size);
		}

		void set_layout(std::unique_ptr<Layout> new_layout)
		{
			layout = std::move(new_layout);
			layout->rearrange_widgets(*this, this->rect.w, this->rect.h);
			_redraw();
		}

		virtual void add_widget(Widget * widget, bool do_redraw = false)
		{
			widget->parent_container = this;
			widgets.push_back(widget);
			widget->add_monitor(this);
			if (layout->rearrange_widgets(*this, this->rect.w, this->rect.h) && do_redraw)
				_redraw();
		}
		void add_widget(Widget & widget, bool do_redraw = false)
		{
			add_widget(&widget, do_redraw);
		}

		virtual void notify(monitorable<widget_change_t> *, widget_change_t & ev) override
		{
			if (ev == widget_change_t::resized)
			{
				if (layout->rearrange_widgets(*this, this->rect.w, this->rect.h))
					_redraw();
			}
			else
			{
				_redraw();
			}
		}

		void set_focus(Widget * widg)
		{
			parent_window->set_focus(widg);
		}
		void mouse_grab(Widget * widg)
		{
			parent_window->mouse_grab(widg);
		}
		void mouse_release(Widget * widg)
		{
			parent_window->mouse_release(widg);
		}

		virtual void _redraw()
		{
			this->clear_background();
			for (Widget *  widget : widgets) {
				//widget->_redraw();
				this->drawable_area.copy_from(widget->drawable_area, widget->rect.x, widget->rect.y);
			}
			this->draw_border();
			this->notify_monitors(widget_change_t::redrawn);
		}

		Widget * find_widget_at(int x, int y)
		{
			for (Widget * widget : widgets)
				if (   x >= widget->rect.x && x < widget->rect.x + widget->rect.w
					&& y >= widget->rect.y && y < widget->rect.y + widget->rect.h )
					return &*widget;
			return nullptr;
		}

		virtual int height_packed() override
		{
			int h = 2*(this->border_width + this->padding);
			for (Widget * widget : widgets)
				h += widget->height_packed();
			return h;
		}

		virtual bool handle_event(event & ev) override
		{
			switch(ev.type)
			{
				case nop:
					return true;
				case window_shown:
					return false;
				case mouse:
				{
					Widget * widget = find_widget_at(ev.data.mouse.x, ev.data.mouse.y);
					if (widget)
					{
						ev.data.mouse.x -= widget->rect.x;
						ev.data.mouse.y -= widget->rect.y;
						return widget->handle_event(ev);
					}
					break;
				}
				case key:
				{
				}
			}
			return false;
		}

		virtual bool vpack() override
		{
			return layout->vpack(*this);
		}
	};





	struct Label : Widget, monitor<text_change_t>
	{
		Text caption;

		Label(Window * window, std::string text, Rect r)
			: Widget(window, r)
			, caption(std::move(text), window)
		{
			caption.add_monitor(this);
			_redraw();
		}

		virtual void notify(monitorable<text_change_t> *, text_change_t &) override
		{
			_redraw();
		}

		virtual bool focusable() override { return false; }
		virtual void _redraw() override
		{
			caption.render();
			int y = (this->rect.h - caption.h) / 2;
			this->drawable_area.fill(128,128,128);
			this->drawable_area.copy_from(caption, 0, y);
			this->notify_monitors(widget_change_t::redrawn);
		}

		virtual bool pack() override
		{
			return this->set_size(caption.get_size());
		}
		virtual int height_packed() override
		{
			return std::get<1>(caption.get_size());
		}
	};

	struct Splitter : Widget, monitor<widget_change_t>
	{
		int thickness = 6;
		bool is_horizontal;
		int split_position;
		Container one, two;

		Splitter(Window * window, Rect r, bool is_horizontal)
			: Widget(window, r)
			, is_horizontal(is_horizontal)
			, split_position((is_horizontal?r.h:r.w)/2)
			, one(window, Rect{                                  0,                                   0, is_horizontal?r.w:((r.w-thickness)/2), is_horizontal?((r.h-thickness)/2):r.h})
			, two(window, Rect{is_horizontal?0:((r.w+thickness)/2), is_horizontal?((r.h+thickness)/2):0, is_horizontal?r.w:((r.w-thickness)/2), is_horizontal?((r.h-thickness)/2):r.h})
		{
			one.border_width = 1;
			two.border_width = 1;
			one.border_is_sunken = true;
			two.border_is_sunken = true;
			this->border_width = 0;
			one.add_monitor(this);
			two.add_monitor(this);
			_redraw();
		}

		virtual void notify(monitorable<widget_change_t> *, widget_change_t &) override
		{
			_redraw();
		}

		virtual bool focusable() override { return false; }
		virtual int height_packed() override
		{
			if (is_horizontal)
				return one.height_packed() + two.height_packed() + thickness;
			else
				return this->rect.h;
		}
		virtual void _redraw() override
		{
			if (is_horizontal)
				this->drawable_area.fill_rect(0, split_position-thickness, this->rect.w, split_position+thickness, this->color_bg, this->color_bg, this->color_bg);
			else
				this->drawable_area.fill_rect(split_position-thickness, 0, split_position+thickness, this->rect.h, this->color_bg, this->color_bg, this->color_bg);

			this->drawable_area.copy_from(one.drawable_area, 0, 0);
			if (is_horizontal)
				this->drawable_area.copy_from(two.drawable_area, 0, split_position+thickness/2);
			else
				this->drawable_area.copy_from(two.drawable_area, split_position+thickness/2, 0);
			this->notify_monitors(widget_change_t::redrawn);
		}

		/*virtual void pack() 
		{
			this->set_size(caption.get_size());
		}*/

		void set_split_position(int p)
		{
			if (p == split_position)
				return;

			split_position = p;

			this->expect(&one, widget_change_t::resized, [](){});
			this->expect(&one, widget_change_t::redrawn, [](){});
			this->expect(&two, widget_change_t::resized, [](){});
			this->expect(&two, widget_change_t::redrawn, [](){});
			if (is_horizontal)
			{
				one.set_size({this->rect.w, split_position-thickness/2});
				two.rect.y = split_position+thickness/2;
				two.set_size({this->rect.w, this->rect.h-split_position-thickness/2});
			}
			else
			{
				one.set_size({split_position-thickness/2, this->rect.h});
				two.rect.x = split_position+thickness/2;
				two.set_size({this->rect.w-split_position-thickness/2, this->rect.h});
			}
			_redraw();
		}

		virtual bool handle_event(event & ev) override
		{
			switch(ev.type)
			{
				case nop:
					return true;
				case window_shown:
					return false;
				case mouse:
				{
					if (this->has_mouse())
					{
						if (ev.data.mouse.released)
						{
							this->mouse_release();
							break;
						}
						else if ( ! ev.data.mouse.pressed)
						{
							int new_pos;
							// mouse moved
							if (is_horizontal)
							{
								new_pos = ev.data.mouse.y;
								if (ev.data.mouse.y < 15)
									new_pos = 15;
								else if (ev.data.mouse.y > this->rect.h-15)
									new_pos = this->rect.h-15;
							}
							else if ( ! is_horizontal)
							{
								new_pos = ev.data.mouse.x;
								if (ev.data.mouse.x < 15)
									new_pos = 15;
								else if (ev.data.mouse.x > this->rect.w-15)
									new_pos = this->rect.w-15;
							}
							set_split_position(new_pos);
						}
						return false;
					}

					if (ev.data.mouse.pressed)
					{
						if (   (!is_horizontal && (ev.data.mouse.x > split_position - thickness/2) && (ev.data.mouse.x < split_position + thickness/2))
							|| ( is_horizontal && (ev.data.mouse.y > split_position - thickness/2) && (ev.data.mouse.y < split_position + thickness/2))  )
						{
							// grabbing the splitter
							if (this->parent_container)
							{
								this->parent_container->mouse_grab(this);
								return true;
							}
						}
					}

					// regular click in sub container
					auto & sub_container = [&]() -> auto & {
							if (is_horizontal)
							{
								if (ev.data.mouse.y < split_position)
									return one;
								else
									return two;
							}
							else
							{
								if (ev.data.mouse.x < split_position)
									return one;
								else
									return two;
							}
						}();
					ev.data.mouse.x -= sub_container.rect.x;
					ev.data.mouse.y -= sub_container.rect.y;
					bool processed = sub_container.handle_event(ev);
					if (processed)
						_redraw();
					return processed;
				}
				case key:
				{
					return false;
				}
			}
			return false;
		}
	};

	struct Button : Widget, monitor<text_change_t>
	{
		bool pressed = false;
		Text caption;
		std::function<void()> func;

		Button(Window * window, std::string t, Rect r, std::function<void()> f)
			: Widget(window, r)
			, caption(t, window)
			, func(std::move(f))
		{
			caption.add_monitor(this);
			this->border_is_sunken = false;
			_redraw();
		}

		virtual void notify(monitorable<text_change_t> *, text_change_t &) override
		{
			_redraw();
		}

		void set_text(std::string s)
		{
			caption.set_text(s);
		}

		virtual bool pack() override
		{
			return this->set_size({2*this->border_width + 2*this->padding + caption.w, 2*this->border_width + 2*this->padding + caption.h});
		}

		virtual void _redraw() override
		{			
			int offset = pressed;

			this->clear_background();

			// Text
			caption.render();
			int x = (this->rect.w - caption.w) / 2 + offset;
			int y = (this->rect.h - caption.h) / 2 + offset;
			this->drawable_area.copy_from(caption, x, y);

			// Border
			this->draw_border();

			this->notify_monitors(widget_change_t::redrawn);
		}

		virtual int height_packed() override
		{
			return 2*(this->border_width + this->padding) + caption.h;
		}

		virtual bool handle_event(event & ev) override
		{
			switch(ev.type)
			{
				case nop:
					return true;
				case window_shown:
					return false;
				case mouse:
				{
					if (ev.data.mouse.pressed)
					{
						if (pressed)
							return false;
						pressed = true; 
						this->border_is_sunken = true;
						this->take_focus();
						_redraw();
						return true;
					}
					else
					{
						if ( ! pressed)
							return false;
						pressed = false;
						this->border_is_sunken = false;
						event_clicked();
						_redraw();
						return true;
					}
					return true;
				}
				case key:
				{
					// TODO: event_clicked if enter or space
				}
			}
			return false;
		}

		virtual void event_clicked()
		{
			if (func)
				func();
		}
	};


	struct TextEdit : Widget, monitor<text_change_t>
	{
		Text caption;
		int text_x;
		int text_y;	
		unsigned int char_pos = 0;
		unsigned int cursor_x = 0;
	
		TextEdit(Window * window, std::string t, Rect r)
			: Widget(window, r)
			, caption(t, window)
		{
			this->border_is_sunken = true;
			caption.add_monitor(this);
			this->padding = 5;
			this->color_bg = 255;
			_redraw();
		}

		virtual void notify(monitorable<text_change_t> *, text_change_t &) override
		{
			_redraw();
		}

		void set_text(std::string s)
		{
			caption.set_text(s);
		}

		virtual bool pack() override
		{
			return this->set_size({2*this->border_width + 2*this->padding + caption.w, 2*this->border_width + 2*this->padding + caption.h});
		}
		virtual int height_packed() override
		{
			return 2*(this->border_width + this->padding) + caption.h;
		}
		virtual void _redraw() override
		{
			// Background
			this->clear_background();
			// Text
			//caption.render();
			text_x = this->padding;
			text_y = (this->rect.h - caption.h) / 2;
			this->drawable_area.copy_from(caption, text_x, text_y);

			// cursor
			if (this->has_focus())
				this->drawable_area.draw_line(cursor_x, text_y, cursor_x, text_y + caption.h, 0, 0, 0);

			// Border
			this->draw_border();

			this->notify_monitors(widget_change_t::redrawn);
		}


		virtual bool handle_event(event & ev) override
		{
			switch(ev.type)
			{
				case nop:
					return true;
				case window_shown:
					return false;
				case mouse:
				{
					if (ev.data.mouse.pressed)
					{
						this->take_focus();
						char_pos = caption.get_pos_at(ev.data.mouse.x - text_x);
						cursor_x = text_x + caption.get_char_x(char_pos);
						_redraw();
						return true;
					}
					return false;
				}
				case key:
				{
					if (ev.data.key.pressed)
						event_key_down(ev.data.key.charcode);
				}
			}
			return false;
		}

		bool event_key_down(int key)
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
				_redraw();
				return true;
			}
			return false;
		}
	};


	struct VLayout : Layout
	{
		horizontal_alignment_t h_align = horizontal_alignment_t::center;
		  vertical_alignment_t v_align =   vertical_alignment_t::center;

		VLayout() {}
		VLayout(vertical_alignment_t valign)
			: v_align(valign)
		{}

		virtual bool rearrange_widgets(const Container & container, [[maybe_unused]]int w, [[maybe_unused]]int h) override
		{
			auto & widgets = container.widgets;

			int next_y = [&]() {
					if (   v_align == vertical_alignment_t::top
						|| v_align == vertical_alignment_t::justify  )
						return container.border_width + container.padding;

					if (v_align == vertical_alignment_t::fill)
					{
						// resize vfill widgets so they fill the container
						int remaining_height = h;
						int fill_widgets_count = 0;
						for (Widget * widget : widgets)
						{
							remaining_height -= widget->height_packed();
							fill_widgets_count += widget->can_vfill();
						}
						if (fill_widgets_count == 0)
							return 0;
						int distribute_height = remaining_height / fill_widgets_count;
						for (Widget * widget : widgets)
							if (widget->can_vfill())
							{
								widget->set_size({widget->rect.h, widget->height_packed()+distribute_height});
								remaining_height -= distribute_height;
							}
							else
								widget->vpack();
						// TODO: when remaining_height / fill_widgets_count has a remainder
						return 0;
					}

					int total_height = 0;
					for (Widget * widget : widgets)
						total_height += widget->rect.h;

					if (v_align == vertical_alignment_t::center)
						return (h - total_height)/2;
					if (v_align == vertical_alignment_t::bottom)
						return h - container.border_width - container.padding - total_height;

					return 0;
				}();
			
			for (Widget * widget : widgets)
			{
				widget->rect.y = next_y;
				next_y += widget->rect.h;
				widget->rect.x = (w - widget->rect.w) / 2;
			}
			return true; // suboptimal	
		}
	};
	struct HLayout : Layout
	{
		horizontal_alignment_t h_align = horizontal_alignment_t::center;
		  vertical_alignment_t v_align =   vertical_alignment_t::center;

		HLayout() {}
		HLayout(horizontal_alignment_t align)
			: h_align(align)
		{}

		virtual bool rearrange_widgets(const Container & container, [[maybe_unused]]int w, [[maybe_unused]]int h) override
		{
			auto & widgets = container.widgets;

			int next_x = [&]() {
					if (   v_align == vertical_alignment_t::top
						|| v_align == vertical_alignment_t::justify  )
						return container.border_width + container.padding;

					int total_height = 0;
					for (Widget * widget : widgets)
						total_height += widget->rect.h;

					if (v_align == vertical_alignment_t::center)
						return (h - total_height)/2;
					if (v_align == vertical_alignment_t::bottom)
						return h - container.border_width - container.padding - total_height;

					return 0;
				}();
			
			for (Widget * widget : widgets)
			{
				widget->rect.x = next_x;
				next_x += widget->rect.w;
				widget->rect.y = (h - widget->rect.h) / 2;
			}
			return true; // suboptimal
		}

		// returns true if any change happened
		virtual bool vpack(Container & container) override
		{
			bool changed = false;

			int min_height = 0;
			for (Widget * widget : container.widgets)
				min_height = std::max(min_height, widget->height_packed());
			
			if (v_align == vertical_alignment_t::justify || v_align == vertical_alignment_t::fill)
			{
				for (Widget * widget : container.widgets)
				{
					widget->set_size({widget->rect.w,min_height});
					widget->rect.y = container.border_width + container.padding + (min_height-widget->rect.h)/2;
					changed = true;
				}
				container.set_size({container.rect.w, min_height + 2*(container.border_width + container.padding)});
			}
			else if (v_align == vertical_alignment_t::bottom)
			{
				for (Widget * widget : container.widgets)
					if (widget->rect.h > min_height)
					{
						int height_diff = widget->rect.h - min_height;
						widget->rect.y += height_diff;
						widget->set_size({widget->rect.w,min_height});
						changed = true;
					}
			}
			else
			{
				for (Widget * widget : container.widgets)
					if (widget->rect.h > min_height)
					{
						widget->set_size({widget->rect.w,min_height});
						changed = true;
					}
			}
			return changed;
		}
	};




	struct MenuItem : Button
	{
		MenuItem(Window * window, std::string text, Rect r, std::function<void()> f)
			: Button(window, text, r, f)
		{
			this->border_width = 0;
			this->pack(); // will redraw
		}
	};

	struct MenuBar : Container
	{
		MenuBar(Window * window)
			: Container(window, {0,0,window->w,50})
		{
			this->padding = 0;
			this->border_width = 1;
			this->border_is_sunken = false;

			auto this_layout = std::make_unique<HLayout>();
			this_layout->h_align = horizontal_alignment_t::left;
			this_layout->v_align = vertical_alignment_t::justify;
			this->set_layout(std::move(this_layout));

			this->_redraw();
		}

		virtual void add_widget(Widget * widget, bool do_redraw = false) override
		{
			Container::add_widget(widget, do_redraw);
			//if (this->vpack() && do_redraw)
			//	this->_redraw();
		}
		void add_widget(Widget & widget, bool do_redraw = false)
		{
			add_widget(&widget, do_redraw);
		}
		virtual bool can_vfill() override { return false; }	
	};


	struct focus_holder
	{
		Widget * widget = nullptr;

		void operator()(Widget * w)
		{
			if (widget)
				widget->set_focus(false);
			widget = w;
			if (widget)
				widget->set_focus(true);
		}
		operator Widget*() { return widget; }
	};
	struct mouse_grabber
	{
		Widget * widget = nullptr;

		void operator()(Widget * w)
		{
			if (widget)
				widget->set_mouse_grabbed(false);
			widget = w;
			if (widget)
				widget->set_mouse_grabbed(true);
		}
		operator Widget*() { return widget; }
	};

	struct Window : public WSW::Window_t, monitor<widget_change_t>
	{
		Container container;
		focus_holder focus;
		mouse_grabber mousegrab;

		Window(const char * title, int width, int height)
			: WSW::Window_t(title, width, height)
			, container(this, Rect{0, 0, width, height})
		{
			container.add_monitor(this);
		}

		void set_focus(Widget * widg)
		{
			focus(widg);
		}
		void mouse_grab(Widget * widg)
		{
			mousegrab(widg);
		}
		void mouse_release(Widget * widg)
		{
			if (widg == mousegrab)
				mousegrab(nullptr);
		}

		virtual bool handle_event(event ev) override
		{
			switch(ev.type)
			{
				case nop:
					return true;
				case window_shown:
					_redraw();
					return true;
				case mouse:
				{
					Widget * widget = nullptr;
					if (mousegrab)
						widget = mousegrab;
					else
						widget = this->container.find_widget_at(ev.data.mouse.x, ev.data.mouse.y);
					if (widget)
					{
						ev.data.mouse.x -= widget->rect.x;
						ev.data.mouse.y -= widget->rect.y;
						return widget->handle_event(ev);
					}
					break;
				}
				case key:
				{
					Widget * widget = focus;
					if (widget)
						return widget->handle_event(ev);
					break;
				}
			}
			return false;
		}

		virtual void notify(monitorable<widget_change_t> *, widget_change_t & ev) override
		{
			if (ev == widget_change_t::resized)
			{
				_redraw();
			}
			else if (ev == widget_change_t::redrawn)
			{
				_redraw();
			}
		}

		virtual void _redraw()
		{
			//container._redraw();
			container.drawable_area.refresh_window();
			WSW::Window_t::present();
		}	
	};

	struct Manager
	{
		WSW window_system_wrapper;

		template<typename W>
		Window & make_window(const char * title="", int width=1280, int height=1024)
		{
			auto & w = window_system_wrapper.template make_window<W>(title, width, height);
			w._redraw();
			return w;
		}

		void loop()
		{
			window_system_wrapper.loop();
		}
	};

}; // struct-namespace
