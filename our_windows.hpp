
#pragma once

#include <iostream>
#include <vector>
#include <deque>
#include <set>
#include <assert.h>

#include "events.hpp"
//#include "util.hpp"

std::string number_to_column_code(int zero_based_value)
{
	char dict[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const int dict_size = sizeof(dict) - 1;



	std::string result;
	do
	{
		unsigned int c = zero_based_value % dict_size;
		zero_based_value /= dict_size;
		--zero_based_value;
		result.push_back(dict[c]);
	}while(zero_based_value >= 0);
	std::reverse(std::begin(result), std::end(result));
	return result;
}

struct color_t
{
	char r, g, b, a;
	color_t(char x)
		: r(x)
		, g(x)
		, b(x)
		, a(255)
	{}
	color_t(char _r, char _g, char _b, char _a)
		: r(_r)
		, g(_g)
		, b(_b)
		, a(_a)
	{}
	const color_t & operator=(int x)
	{
		r = g = b = x;
		a = 255;
		return *this;
	}
	operator int() const
	{
		return (r+g+b)/3;
	}
};

enum class widget_change_t
{
	any = 0,
	resized,
	moved,
	redrawn,
	focus_changed,
};

struct horizontal_policy
{
	enum class alignment_t
	{
		none = 0,
		center,
		left,
		right,
	};
	enum class sizing_t
	{
		none = 0,
		pack,
		justify,
		fill,
	};
	alignment_t alignment;
	sizing_t sizing;
};
struct vertical_policy
{
	enum class alignment_t
	{
		none = 0,
		center,
		top,
		bottom,
	};
	enum class sizing_t
	{
		none = 0,
		pack,
		justify,
		fill,
	};
	alignment_t alignment;
	sizing_t sizing;
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

	struct Widget
	{
		Container * parent_container = nullptr;
		Rect rect;
		WSW::DrawableArea_t drawable_area;

		bool needs_redraw = true;

		bool _focus = false;
		bool _mouse_grabbed = false;

		int border_width = 1;
		bool border_is_sunken = false;
		int border_color_light = 220;
		int border_color_dark  =  64;
		color_t border_color_black = color_t(  0);
		color_t color_bg = color_t(192);

		int border_padding = 2;
		int inter_padding = 4;

		Widget(Window * window, Rect r)
			: rect(r)
			, drawable_area(window, r.w, r.h)
		{}

		int w() const { return this->rect.w; }
		int h() const { return this->rect.h; }

		int absolute_x() const
		{
			if (parent_container)
				return rect.x + parent_container->absolute_x();
			else
				return rect.x;
		}
		int absolute_y() const
		{
			if (parent_container)
				return rect.y + parent_container->absolute_y();
			else
				return rect.y;
		}

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
			{
				parent_container->set_focus(this);
				set_needs_redraw();
			}
		}
		void set_focus(bool has)
		{
			if (has != _focus)
			{
				_focus = has;
				set_needs_redraw();
			}
		}
		void set_mouse_grabbed(bool grabbed)
		{
			_mouse_grabbed = grabbed;
		}
		void mouse_grab(Widget * w,
		                std::function<void(Widget *, int /*grab_x*/, int /*grab_y*/, int /*drag_x*/, int /*drag_y*/)> drag_f,
		                std::function<void(Widget *, int /*grab_x*/, int /*grab_y*/, int /*drag_x*/, int /*drag_y*/)> ungrab_f)
		{
			if (parent_container)
				parent_container->mouse_grab(w, drag_f, ungrab_f);
		}
		//void mouse_release()
		//{
		//	if (parent_container)
		//		parent_container->mouse_release(this);
		//}

		void set_needs_redraw()
		{
			this->needs_redraw = true;
			if (parent_container)
				parent_container->set_needs_redraw();
		}

		virtual void clear_background()
		{
			this->drawable_area.fill(this->color_bg.r, this->color_bg.g, this->color_bg.b, this->color_bg.a);
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

		virtual bool on_width_set() { return false; }
		virtual bool on_height_set() { return false; }
		virtual bool on_size_set() { return false; }
		virtual bool set_width(int width)
		{
			if (rect.w == width)
				return false;
			rect.w = width;
			drawable_area.set_size({rect.w, rect.h});
			on_width_set();
			set_needs_redraw();
			return true;
		}
		virtual bool set_height(int height)
		{
			if (rect.h == height)
				return false;
			rect.h = height;
			drawable_area.set_size({rect.w, rect.h});
			on_height_set();
			set_needs_redraw();
			return true;
		}
		virtual bool set_size(std::pair<int,int> size)
		{
			if (rect.w == std::get<0>(size) && rect.h == std::get<1>(size))
				return false;
			rect.w = std::get<0>(size);
			rect.h = std::get<1>(size);
			drawable_area.set_size(size);
			on_size_set();
			set_needs_redraw();
			return true;
		}
		virtual bool focusable() { return true; }
		virtual int width_packed() =0;
		virtual int height_packed() =0;
		virtual void _redraw() {}
		virtual bool can_vfill() { return true; }
		virtual bool can_hfill() { return true; }

		virtual bool handle_event(event &) { return false; }
	};
	
	struct Layout
	{
		virtual bool rearrange_widgets(Container &) { return false; }
		virtual bool vpack(Container &) { return false; }
		virtual bool hpack(Container &) { return false; }
		virtual int  width_packed(Container &) { return 0; }
		virtual int height_packed(Container &) { return 0; }
	};

	struct Container : Widget
	{
		Window * parent_window;
		std::vector<Widget*> widgets;
		std::unique_ptr<Layout> layout;

		Container(Window * window, Rect r = Rect{0,0,100,100})
			: Widget(window, r)
			, parent_window(window)
			, layout(std::make_unique<Layout>())
		{
			this->border_width = 0;
			this->border_padding = 0;
		}

		virtual bool on_width_set() override
		{
			return layout->rearrange_widgets(*this);
		}
		virtual bool on_height_set() override
		{
			return layout->rearrange_widgets(*this);
		}
		virtual bool on_size_set() override
		{
			return layout->rearrange_widgets(*this);
		}

		virtual bool pack() override
		{
			bool changed = Widget::pack();
			if (changed)
				layout->rearrange_widgets(*this);
			return changed;
		}

		virtual bool vpack() override
		{
			int contents_height = 0;
			if (widgets.size() > 0)
			{
				int min_y = widgets[0]->rect.y;
				int max_y = 0;
				for (Widget * widget : widgets)
				{
					min_y = std::min(min_y, widget->rect.y);
					max_y = std::max(max_y, widget->rect.y + widget->rect.h);
				}
				contents_height = max_y - min_y;
			}
			return this->set_height(contents_height + 2*(this->border_width + this->border_padding));
		}
		virtual bool hpack() override
		{
			int contents_width = 0;
			if (widgets.size() > 0)
			{
				int min_x = widgets[0]->rect.x;
				int max_x = 0;
				for (Widget * widget : widgets)
				{
					min_x = std::min(min_x, widget->rect.x);
					max_x = std::max(max_x, widget->rect.x + widget->rect.w);
				}
				contents_width = max_x - min_x;
			}
			return this->set_width(contents_width + 2*(this->border_width + this->border_padding));
		}

		void set_layout(std::unique_ptr<Layout> new_layout)
		{
			layout = std::move(new_layout);
			layout->rearrange_widgets(*this);
			this->set_needs_redraw();
		}

		virtual void add_widget(Widget * widget)
		{
			widget->parent_container = this;
			widgets.push_back(widget);
			layout->rearrange_widgets(*this);
			this->set_needs_redraw();
		}
		void add_widget(Widget & widget)
		{
			add_widget(&widget);
		}

		void set_focus(Widget * widg)
		{
			parent_window->set_focus(widg);
		}
		void mouse_grab(Widget * widg,
		                std::function<void(Widget *, int /*grab_x*/, int /*grab_y*/, int /*drag_x*/, int /*drag_y*/)> drag_f,
		                std::function<void(Widget *, int /*grab_x*/, int /*grab_y*/, int /*drag_x*/, int /*drag_y*/)> ungrab_f)
		{
			parent_window->mouse_grab(widg, drag_f, ungrab_f);
		}
		//void mouse_release(Widget * widg)
		//{
		//	parent_window->mouse_release(widg);
		//}

		virtual void _redraw()
		{
			layout->rearrange_widgets(*this);
			this->clear_background();
			for (Widget *  widget : widgets)
			{
				if (widget->needs_redraw)
					widget->_redraw();
				this->drawable_area.copy_from(widget->drawable_area, widget->rect.x, widget->rect.y);
			}
			this->draw_border();
			this->needs_redraw = false;
		}

		Widget * find_widget_at(int x, int y)
		{
			for (Widget * widget : widgets)
				if (   x >= widget->rect.x && x < widget->rect.x + widget->rect.w
					&& y >= widget->rect.y && y < widget->rect.y + widget->rect.h )
					return &*widget;
			return nullptr;
		}
		virtual int width_packed() override
		{
			return layout->width_packed(*this);
		}
		virtual int height_packed() override
		{
			return layout->height_packed(*this);
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
					break;
				default:
					break;
			}
			return false;
		}
	};





	struct Label : Widget
	{
		Text caption;

		Label(Window * window, std::string text, Rect r = Rect{0,0,100,20})
			: Widget(window, r)
			, caption(std::move(text), window)
		{}

		virtual bool focusable() override { return false; }
		virtual void _redraw() override
		{
			caption.render();
			int y = (this->rect.h - caption.h) / 2;
			this->drawable_area.fill(128,128,128);
			this->drawable_area.copy_from(caption, 0, y);
			this->needs_redraw = false;
		}

		virtual bool pack() override
		{
			return this->set_size(caption.get_size());
		}
		virtual bool hpack() override
		{
			return this->set_width(std::get<0>(caption.get_size()));
		}
		virtual bool vpack() override
		{
			return this->set_height(std::get<1>(caption.get_size()));
		}
		virtual int width_packed() override
		{
			return std::get<0>(caption.get_size());
		}
		virtual int height_packed() override
		{
			return std::get<1>(caption.get_size());
		}
	};

	struct Splitter : Container
	{
		int thickness = 6;
		bool is_horizontal;
		int split_position;
		Container one, two;

		Splitter(Window * window, Rect r, bool is_horizontal)
			: Container(window, r)
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
			this->border_padding = 0;

			this->add_widget(&one);
			this->add_widget(&two);
		}

		virtual bool on_width_set() override
		{
			bool changed = false;
			if (is_horizontal)
			{
				changed |= one.set_width(this->rect.w);
				changed |= two.set_width(this->rect.w);
			}
			else
			{
				changed |= one.set_width(split_position-thickness/2);
				changed |= (two.rect.x != split_position+thickness/2);
				two.rect.x = split_position+thickness/2;
				changed |= two.set_width(this->rect.w-split_position-thickness/2);
			}
			return changed;
		}
		virtual bool on_height_set() override
		{
			bool changed = false;
			if (is_horizontal)
			{
				changed |= one.set_height(split_position-thickness/2);
				changed = (two.rect.y != split_position+thickness/2);
				two.rect.y = split_position+thickness/2;
				changed |= two.set_height(this->rect.h-split_position-thickness/2);
			}
			else
			{
				changed |= one.set_height(this->rect.h);
				changed |= two.set_height(this->rect.h);
			}
			return changed;
		}
		virtual bool on_size_set() override
		{
			bool changed = false;
			if (is_horizontal)
			{
				changed |= one.set_size({this->rect.w, split_position-thickness/2});
				changed |= (two.rect.y != split_position+thickness/2);
				two.rect.y = split_position+thickness/2;
				changed |= two.set_size({this->rect.w, this->rect.h-split_position-thickness/2});
			}
			else
			{
				changed |= one.set_size({split_position-thickness/2, this->rect.h});
				changed = (two.rect.x != split_position+thickness/2);
				two.rect.x = split_position+thickness/2;
				changed |= two.set_size({this->rect.w-split_position-thickness/2, this->rect.h});
			}
			return changed;
		}

		virtual bool focusable() override { return false; }
		virtual int width_packed() override
		{
			if (is_horizontal)
				return this->rect.w;
			else
				return one.width_packed() + two.width_packed() + thickness;
		}
		virtual int height_packed() override
		{
			if (is_horizontal)
				return one.height_packed() + two.height_packed() + thickness;
			else
				return this->rect.h;
		}
		virtual void _redraw() override
		{
			if (one.needs_redraw)
				one._redraw();
			if (two.needs_redraw)
				two._redraw();
			if (is_horizontal)
				this->drawable_area.fill_rect(0, split_position-thickness, this->rect.w, split_position+thickness, this->color_bg.r, this->color_bg.g, this->color_bg.b, this->color_bg.a);
			else
				this->drawable_area.fill_rect(split_position-thickness, 0, split_position+thickness, this->rect.h, this->color_bg.r, this->color_bg.g, this->color_bg.b, this->color_bg.a);

			this->drawable_area.copy_from(one.drawable_area, 0, 0);
			if (is_horizontal)
				this->drawable_area.copy_from(two.drawable_area, 0, split_position+thickness/2);
			else
				this->drawable_area.copy_from(two.drawable_area, split_position+thickness/2, 0);

			this->needs_redraw = false;
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

			bool changed = false;
			if (is_horizontal)
			{
				changed |= one.set_size({this->rect.w, split_position-thickness/2});
				changed |= (two.rect.y != split_position+thickness/2);
				two.rect.y = split_position+thickness/2;
				changed |= two.set_size({this->rect.w, this->rect.h-split_position-thickness/2});
			}
			else
			{
				changed |= one.set_size({split_position-thickness/2, this->rect.h});
				changed = (two.rect.x != split_position+thickness/2);
				two.rect.x = split_position+thickness/2;
				changed |= two.set_size({this->rect.w-split_position-thickness/2, this->rect.h});
			}
			this->set_needs_redraw();
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
					/* TODO
					if (this->has_mouse())
					{
						if (ev.data.mouse.released)
						{
							this->mouse_release(this);
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
					*/

					if (ev.data.mouse.pressed)
					{
						if (   (!is_horizontal && (ev.data.mouse.x > split_position - thickness/2) && (ev.data.mouse.x < split_position + thickness/2))
							|| ( is_horizontal && (ev.data.mouse.y > split_position - thickness/2) && (ev.data.mouse.y < split_position + thickness/2))  )
						{
							// grabbing the splitter
							if (this->parent_container)
							{
								//this->parent_container->mouse_grab(this);
								// TODO
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
					return processed;
				}
				case key:
				{
					return false;
				}
				default:
					break;
			}
			return false;
		}
	};

	struct Button : Widget
	{
		bool pressed = false;
		Text caption;
		std::function<void()> func;

		Button(Window * window, std::string t, std::function<void()> f, Rect r = Rect{0,0,100,30})
			: Widget(window, r)
			, caption(t, window)
			, func(std::move(f))
		{
			this->border_is_sunken = false;
		}

		void set_action(std::function<void()> f)
		{
			func = f;
		}
		void set_text(std::string s)
		{
			caption.set_text(s);
			this->set_needs_redraw();
		}

		virtual bool pack() override
		{
			return this->set_size({2*this->border_width + 2*this->border_padding + caption.w, 2*this->border_width + 2*this->border_padding + caption.h});
		}
		virtual bool hpack() override
		{
			return this->set_width(2*this->border_width + 2*this->border_padding + caption.w);
		}
		virtual bool vpack() override
		{
			return this->set_height(2*this->border_width + 2*this->border_padding + caption.h);
		}
		virtual int width_packed() override
		{
			return 2*(this->border_width + this->border_padding) + caption.w;
		}
		virtual int height_packed() override
		{
			return 2*(this->border_width + this->border_padding) + caption.h;
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

			this->needs_redraw = false;
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
						return true;
					}
					else
					{
						if ( ! pressed)
							return false;
						pressed = false;
						this->border_is_sunken = false;
						event_clicked();
						this->set_needs_redraw();
						return true;
					}
					return true;
				}
				case key:
				{
					// TODO: event_clicked if enter or space
				}
				default:
					break;
			}
			return false;
		}

		virtual void event_clicked()
		{
			if (func)
				func();
		}
	};


	struct TextEdit : Widget
	{
		Text caption;
		int text_x;
		int text_y;	
		unsigned int char_pos = 0;
		unsigned int cursor_x = 0;
	
		TextEdit(Window * window, std::string t, Rect r = Rect{0,0,150,25})
			: Widget(window, r)
			, caption(t, window)
		{
			this->border_is_sunken = true;
			this->border_padding = 5;
			this->color_bg = 255;
		}

		void set_text(std::string s)
		{
			caption.set_text(s);
		}

		virtual bool can_hfill() override { return false; }

		virtual bool pack() override
		{
			return this->set_size({2*this->border_width + 2*this->border_padding + caption.w, 2*this->border_width + 2*this->border_padding + caption.h});
		}
		virtual bool hpack() override
		{
			return this->set_width(2*this->border_width + 2*this->border_padding + caption.w);
		}
		virtual bool vpack() override
		{
			return this->set_height(2*this->border_width + 2*this->border_padding + caption.h);
		}
		virtual int width_packed() override
		{
			return 2*(this->border_width + this->border_padding) + caption.w;
		}
		virtual int height_packed() override
		{
			return 2*(this->border_width + this->border_padding) + caption.h;
		}
		virtual void _redraw() override
		{
			// Background
			this->clear_background();
			// Text
			//caption.render();
			text_x = this->border_padding;
			text_y = (this->rect.h - caption.h) / 2;
			this->drawable_area.copy_from(caption, text_x, text_y);

			// cursor
			if (this->has_focus())
				this->drawable_area.draw_line(cursor_x, text_y, cursor_x, text_y + caption.h, 0, 0, 0);

			// Border
			this->draw_border();

			this->needs_redraw = false;
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
						this->set_needs_redraw();
						return true;
					}
					else
					{
						// mouse move
						if (   ev.data.mouse.x > this->border_width + this->border_padding
							&& ev.data.mouse.y > this->border_width + this->border_padding
							&& ev.data.mouse.x < this->rect.w - (this->border_width + this->border_padding)
							&& ev.data.mouse.y < this->rect.h - (this->border_width + this->border_padding))
						{
							if (this->parent_container)
								this->parent_container->parent_window->set_cursor(MouseCursorImg::IBEAM);
						}
						else
						{
							if (this->parent_container)
								this->parent_container->parent_window->set_cursor(MouseCursorImg::ARROW);
						}
					}
					return false;
				}
				case key:
				{
					if (ev.data.key.pressed)
						event_key_down(ev.data.key.charcode);
				}
				default:
					break;
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
				this->set_needs_redraw();
				return true;
			}
			return false;
		}
	};


	struct VLayout : Layout
	{
		horizontal_policy hpolicy;
		  vertical_policy vpolicy;

		VLayout() {}
		VLayout(horizontal_policy h, vertical_policy v)
			: hpolicy(h)
			, vpolicy(v)
		{}

		virtual int width_packed(Container & container) override
		{
			int w = 2*(container.border_width + container.border_padding);
			int max_widget_width = 0;
			for (Widget * widget : container.widgets)
				max_widget_width = std::max(max_widget_width, widget->width_packed());
			return w + max_widget_width;
		}
		virtual int height_packed(Container & container) override
		{
			int h = 2*(container.border_width + container.border_padding);
			for (Widget * widget : container.widgets)
				h += widget->height_packed();
			return h;
		}

		virtual bool rearrange_widgets(Container & container) override
		{
			bool changed = false;

			changed |= calculate_new_w(container);
			changed |= calculate_new_h(container);
			changed |= calculate_new_x(container);
			changed |= calculate_new_y(container);

			if (changed)
				container.set_needs_redraw();

			return changed;
		}

		bool calculate_new_w(Container & container)
		{
			bool changed = false;
			switch(hpolicy.sizing)
			{
				case horizontal_policy::sizing_t::none:
					// do nothing
					break;
				case horizontal_policy::sizing_t::pack:
				{
					for (Widget * widget : container.widgets)
						changed |= widget->hpack();
					//int max_packed_width = 0;
					//for (Widget * widget : container.widgets)
					//	max_packed_width = std::max(max_packed_width, widget->width_packed());
					//container.set_width(max_packed_width + 2*(container.border_width+container.border_padding), false, false);
					break;
				}
				case horizontal_policy::sizing_t::justify:
				{
					int max_packed_width = 0;
					for (Widget * widget : container.widgets)
						max_packed_width = std::max(max_packed_width, widget->width_packed());
					for (Widget * widget : container.widgets)
						changed |= widget->set_width(max_packed_width);
					break;
				}
				case horizontal_policy::sizing_t::fill:
					for (Widget * widget : container.widgets)
						changed |= widget->set_width(container.rect.w - 2*(container.border_width+container.border_padding));
					break;
			}
			return changed;
		}

		bool calculate_new_x(Container & container)
		{
			bool changed = false;
			if (hpolicy.alignment == horizontal_policy::alignment_t::center)
				for (Widget * widget : container.widgets)
				{
					int new_x = (container.rect.w - widget->rect.w)/2;
					changed |= (widget->rect.x != new_x);
					widget->rect.x = new_x;
				}
			else if (hpolicy.alignment == horizontal_policy::alignment_t::right)
				for (Widget * widget : container.widgets)
				{
					int new_x = container.rect.w - container.border_width - container.border_padding - widget->rect.w;
					changed |= widget->rect.x != new_x;
					widget->rect.x = new_x;
				}
			else // left or nonw
				for (Widget * widget : container.widgets)
				{
					int new_x = container.border_width + container.border_padding;
					changed |= widget->rect.x != new_x;
					widget->rect.x = new_x;
				}
			return changed;
		}

		bool calculate_new_y(Container & container)
		{
			bool changed = false;
			int y = [&](){
					if (vpolicy.alignment == vertical_policy::alignment_t::center)
					{
						int total_height = container.inter_padding * (container.widgets.size()-1);
						for (Widget * widget : container.widgets)
							total_height += widget->rect.h;
						return (container.rect.h - total_height)/2;
					}
					else if (vpolicy.alignment == vertical_policy::alignment_t::bottom)
					{
						int total_height = container.inter_padding * (container.widgets.size()-1);
						for (Widget * widget : container.widgets)
							total_height += widget->rect.h;
						return container.rect.h - container.border_width - container.border_padding - total_height;
					}
					else // top or none
					{
						return container.border_width + container.border_padding;
					}
				}();
			for (Widget * widget : container.widgets)
			{
				changed |= (widget->rect.y != y);
				widget->rect.y = y;
				y += widget->rect.h + container.inter_padding;
			}
			return changed;
		}

		bool calculate_new_h(Container & container)
		{
			bool changed = false;
			if (vpolicy.sizing == vertical_policy::sizing_t::pack)
			{
				for (Widget * widget : container.widgets)
					changed |= widget->vpack();
			}
			else if (vpolicy.sizing == vertical_policy::sizing_t::justify)
			{
				int max_packed_height = 0;
				for (Widget * widget : container.widgets)
					max_packed_height = std::max(max_packed_height, widget->height_packed());
				for (Widget * widget : container.widgets)
					changed |= widget->set_height(max_packed_height);
			}
			else if (vpolicy.sizing == vertical_policy::sizing_t::fill)
			{
				int min_total_height = 0;
				int fill_widgets_count = 0;
				for (Widget * widget : container.widgets)
				{
					min_total_height += widget->height_packed();
					fill_widgets_count += widget->can_hfill();
				}
				if (fill_widgets_count != 0)
				{
					int surplus_height = container.rect.h - 2*(container.border_width+container.border_padding) - min_total_height;
					int surplus_height_per_widget = surplus_height / fill_widgets_count;
					int surplus_widgets = surplus_height % fill_widgets_count;
					for (Widget * widget : container.widgets)
					{
						if (widget->can_hfill())
						{
							changed |= widget->set_height(widget->height_packed() + surplus_height_per_widget + (surplus_widgets>0));
							surplus_height -= surplus_height_per_widget;
							surplus_widgets -= (surplus_widgets>0);
						}
					}
				}
			}
			else // none
			{
				// do nothing
			}
			return changed;
		}
	};
	struct HLayout : Layout
	{
		horizontal_policy hpolicy;
		  vertical_policy vpolicy;

		HLayout() {}
		HLayout(horizontal_policy h, vertical_policy v)
			: hpolicy(h)
			, vpolicy(v)
		{}

		virtual bool rearrange_widgets(Container & container) override
		{
			bool changed = false;

			changed |= calculate_new_w(container);
			changed |= calculate_new_h(container);
			changed |= calculate_new_x(container);
			changed |= calculate_new_y(container);

			if (changed)
				container.set_needs_redraw();

			return changed;
		}

		virtual int width_packed(Container & container) override
		{
			int w = 2*(container.border_width + container.border_padding);
			for (Widget * widget : container.widgets)
				w += widget->width_packed();
			return w;
		}
		virtual int height_packed(Container & container) override
		{
			int h = 2*(container.border_width + container.border_padding);
			int max_widget_height = 0;
			for (Widget * widget : container.widgets)
				max_widget_height = std::max(max_widget_height, widget->height_packed());
			return h + max_widget_height;
		}

		bool calculate_new_h(Container & container)
		{
			bool changed = false;
			switch(vpolicy.sizing)
			{
				case vertical_policy::sizing_t::none:
					// do nothing
					break;
				case vertical_policy::sizing_t::pack:
				{
					for (Widget * widget : container.widgets)
						changed |= widget->vpack();
					//int max_packed_height = 0;
					//for (Widget * widget : container.widgets)
					//	max_packed_height = std::max(max_packed_height, widget->height_packed());
					//container.set_height(max_packed_height + 2*(container.border_width+container.border_padding), false, false);
					break;
				}
				case vertical_policy::sizing_t::justify:
				{
					int max_packed_height = 0;
					for (Widget * widget : container.widgets)
						max_packed_height = std::max(max_packed_height, widget->height_packed());
					for (Widget * widget : container.widgets)
						changed |= widget->set_height(max_packed_height);
					break;
				}
				case vertical_policy::sizing_t::fill:
					for (Widget * widget : container.widgets)
						changed |= widget->set_height(container.rect.h - 2*(container.border_width+container.border_padding));
					break;
			}
			return changed;
		}

		bool calculate_new_y(Container & container)
		{
			bool changed = false;
			if (vpolicy.alignment == vertical_policy::alignment_t::center)
				for (Widget * widget : container.widgets)
					widget->rect.y = (container.rect.h - widget->rect.h)/2;
			else if (vpolicy.alignment == vertical_policy::alignment_t::bottom)
				for (Widget * widget : container.widgets)
					widget->rect.y = container.rect.h - container.border_width - container.border_padding - widget->rect.h;
			else // top or nonw
				for (Widget * widget : container.widgets)
					widget->rect.y = container.border_width + container.border_padding;
			return changed;
		}

		bool calculate_new_x(Container & container)
		{
			bool changed = false;
			int x = [&](){
					if (hpolicy.alignment == horizontal_policy::alignment_t::center)
					{
						int total_width = container.inter_padding * (container.widgets.size()-1);
						for (Widget * widget : container.widgets)
							total_width += widget->rect.w;
						return (container.rect.w - total_width)/2;
					}
					else if (hpolicy.alignment == horizontal_policy::alignment_t::right)
					{
						int total_width = container.inter_padding * (container.widgets.size()-1);
						for (Widget * widget : container.widgets)
							total_width += widget->rect.w;
						return container.rect.w - container.border_width - container.border_padding - total_width;
					}
					else // left or none
					{
						return container.border_width + container.border_padding;
					}
				}();
			for (Widget * widget : container.widgets)
			{
				changed |= (widget->rect.x != x);
				widget->rect.x = x;
				x += widget->rect.w + container.inter_padding;
			}
			return changed;
		}

		bool calculate_new_w(Container & container)
		{
			bool changed = false;
			if (hpolicy.sizing == horizontal_policy::sizing_t::pack)
			{
				for (Widget * widget : container.widgets)
					changed |= widget->hpack();
			}
			else if (hpolicy.sizing == horizontal_policy::sizing_t::justify)
			{
				int max_packed_width = 0;
				for (Widget * widget : container.widgets)
					max_packed_width = std::max(max_packed_width, widget->width_packed());
				for (Widget * widget : container.widgets)
					changed |= widget->set_width(max_packed_width);
			}
			else if (hpolicy.sizing == horizontal_policy::sizing_t::fill)
			{
				int min_total_width = 0;
				int fill_widgets_count = 0;
				for (Widget * widget : container.widgets)
				{
					min_total_width += widget->width_packed();
					fill_widgets_count += widget->can_vfill();
				}
				if (fill_widgets_count != 0)
				{
					int surplus_width = container.rect.w - 2*(container.border_width+container.border_padding) - min_total_width;
					int surplus_width_per_widget = surplus_width / fill_widgets_count;
					int surplus_widgets = surplus_width % fill_widgets_count;
					for (Widget * widget : container.widgets)
					{
						if (widget->can_hfill())
						{
							changed |= widget->set_width(widget->width_packed() + surplus_width_per_widget + (surplus_widgets>0));
							//surplus_width -= surplus_width_per_widget;
							surplus_widgets -= (surplus_widgets>0);
						}
					}
				}
			}
			else // none
			{
				// do nothing
			}
			return changed;
		}
	};



	struct PopupMenu;

	struct MenuItem : Button
	{
		std::unique_ptr<PopupMenu> submenu;

		MenuItem(Window * window, std::string text, std::function<void()> f, Rect r = Rect{0,0,100,20})
			: Button(window, text, f, r)
		{
			this->border_width = 0;
			this->border_padding = 6;
		}
		virtual void _redraw() override
		{			
			int offset = this->pressed;

			this->clear_background();

			// Text
			this->caption.render();
			int x = this->border_width + this->border_padding;
			int y = (this->rect.h - this->caption.h) / 2 + offset;
			this->drawable_area.copy_from(this->caption, x, y);

			// Border
			this->draw_border();

			this->needs_redraw = false;
		}
	};

	struct PopupMenu : Container
	{
		std::vector<std::unique_ptr<MenuItem>> menu_items;

		PopupMenu(Window * window)
			: Container(window)
		{
			this->border_color_light = this->border_color_dark;
			this->color_bg = this->color_bg + (256-this->color_bg)/2;

			this->border_is_sunken = false;
			this->border_width = 1;
			this->border_padding = 0;
			this->inter_padding = 0;
			this->set_layout(std::make_unique<VLayout>(horizontal_policy{horizontal_policy::alignment_t::left  , horizontal_policy::sizing_t::justify}
			                                          ,  vertical_policy{  vertical_policy::alignment_t::center,   vertical_policy::sizing_t::pack}));
		}

		PopupMenu & add_submenu(std::string caption)
		{
			std::unique_ptr<MenuItem> menu_item = std::make_unique<MenuItem>(this->parent_window, caption, [&](){});
			MenuItem * menu_item_ptr = menu_item.get();
			menu_item->submenu = std::make_unique<PopupMenu>(this->parent_window);
			menu_item->submenu->parent_container = &this->parent_window->container;
			menu_item->set_action([this,menu_item_ptr]()
				{
					menu_item_ptr->submenu->rect.x = menu_item_ptr->absolute_x() + menu_item_ptr->rect.w;
					menu_item_ptr->submenu->rect.y = menu_item_ptr->absolute_y();
					this->parent_window->add_popup(menu_item_ptr->submenu.get());
				});
			menu_items.push_back(std::move(menu_item));
			Container::add_widget(menu_items.back().get());
			return *menu_items.back()->submenu;
		}
		void add(std::string caption, std::function<void()> func)
		{
			std::unique_ptr<MenuItem> menu_item = std::make_unique<MenuItem>(this->parent_window, caption, [this,func]()
				{
					this->parent_window->clear_popups();
					func();
				});
			menu_items.push_back(std::move(menu_item));
			Container::add_widget(menu_items.back().get());
			this->pack();
		}
	};

	struct MenuBar : Container
	{
		std::vector<std::unique_ptr<MenuItem>> menu_items;

		MenuBar(Window * window)
			: Container(window, {0,0,window->w,50})
		{
			this->border_padding = 0;
			this->inter_padding = 4;
			this->border_width = 1;
			this->border_is_sunken = false;

			auto this_layout = std::make_unique<HLayout>(horizontal_policy{horizontal_policy::alignment_t::left  , horizontal_policy::sizing_t::pack}
			                                            ,  vertical_policy{  vertical_policy::alignment_t::center,   vertical_policy::sizing_t::pack});
			this->set_layout(std::move(this_layout));
		}

		PopupMenu & add_submenu(std::string caption)
		{
			std::unique_ptr<MenuItem> menu_item = std::make_unique<MenuItem>(this->parent_window, caption, [](){});
			MenuItem * menu_item_ptr = menu_item.get();
			menu_item->submenu = std::make_unique<PopupMenu>(this->parent_window);
			menu_item->submenu->parent_container = &this->parent_window->container;
			menu_item->set_action([this,menu_item_ptr]()
				{
					menu_item_ptr->submenu->rect.x = menu_item_ptr->absolute_x();
					menu_item_ptr->submenu->rect.y = menu_item_ptr->absolute_y() + menu_item_ptr->rect.h;
					this->parent_window->add_popup(menu_item_ptr->submenu.get());
				});
			menu_items.push_back(std::move(menu_item));
			Container::add_widget(menu_items.back().get());
			return *menu_items.back()->submenu;
		}
		void add(std::string caption, std::function<void()> func)
		{
			std::unique_ptr<MenuItem> menu_item = std::make_unique<MenuItem>(this->parent_window, caption, [this,func]()
				{
					this->parent_window->clear_popups();
					func();
				});
			menu_items.push_back(std::move(menu_item));
			Container::add_widget(menu_items.back().get());
		}
		virtual bool can_hfill() override { return false; }
	};


	struct Grid : Widget
	{
		inline static const color_t color_cell_bg          = color_t(255);
		inline static const color_t color_text_header      = color_t(32);
		inline static const color_t color_lines            = color_t(160);
		inline static const color_t color_bg_header        = color_t(210);
		inline static const color_t selected_cells_overlay = color_t(128,200,128,96);
		inline static const int header_resizing_area_thickness = 2;

		Window * parent_window;

		std::vector<std::vector<std::string>> formulas;
		std::vector<std::vector<std::string>> values;

		unsigned int header_cols_height = 18;
		unsigned int header_rows_width = 40;

		std::vector<unsigned int> thickness_cols;
		std::vector<unsigned int> thickness_rows;

		std::set<unsigned int>   selected_cols;
		std::set<unsigned int>   selected_rows;
		std::set<unsigned int> unselected_cols;
		std::set<unsigned int> unselected_rows;
		std::set<std::pair<unsigned int,unsigned int>>   selected_cells;
		std::set<std::pair<unsigned int,unsigned int>> unselected_cells;
		bool selected_all = false;		

		std::deque<Text> header_captions_cols;
		std::deque<Text> header_captions_rows;

		bool key_ctrl = false;

		Grid(Window * window)
			: Widget(window, {0,0,200,200})
			, parent_window(window)
		{
			this->border_width = 0;
			this->border_padding = 0;
			this-> inter_padding = 0;
			this->color_bg = 160;

			insert_columns(50, 0);
			insert_rows(100, 0);
		}

		virtual int  width_packed() { return 0; }
		virtual int height_packed() { return 0; }

		bool has_integrity() const
		{
			// row count
			if (formulas.size() != values.size())
				return false;
			if (formulas.size() != thickness_rows.size())
				return false;
			if (formulas.size() < selected_rows.size())
				return false;

			// column count
			unsigned int col_count = get_column_count();
			for (auto & row : formulas)
				if (row.size() != col_count)
					return false;
			for (auto & row : values)
				if (row.size() != col_count)
					return false;
			if (thickness_rows.size() != col_count)
				return false;
			if (selected_cols.size() > col_count)
				return false;

			// selection
			if (selected_cells.size() > get_column_count() * get_row_count())
				return false;

			return true;
		}

		unsigned int get_column_count() const
		{
			if (formulas.size() == 0)
				return 0;
			return formulas[0].size();
		}
		unsigned int get_row_count() const
		{
			return formulas.size();
		}

		void insert_columns(unsigned int count, unsigned int before_idx)
		{
			assert(has_integrity());

			if (before_idx > get_column_count())
				return;

			for (auto & row : formulas)
				row.insert(std::next(std::begin(row), before_idx), count, "");
			for (auto & row : values)
				row.insert(std::next(std::begin(row), before_idx), count, "");
			thickness_cols.insert(std::next(std::begin(thickness_cols), before_idx), count, 50);

			// column headers
			for (decltype(count) i=0 ; i<count ; ++i)
				header_captions_cols.emplace_back(number_to_column_code(header_captions_cols.size()), parent_window, color_text_header.r, color_text_header.g, color_text_header.b);
		}
		void insert_rows(unsigned int count, unsigned int before_idx)
		{
			assert(has_integrity());

			if (before_idx > get_row_count())
				return;

			formulas.insert(std::next(std::begin(formulas), before_idx), count, std::vector<std::string>(count, ""));
			values  .insert(std::next(std::begin(values  ), before_idx), count, std::vector<std::string>(count, ""));
			thickness_rows.insert(std::next(std::begin(thickness_rows), before_idx), count, 18);

			// row headers
			for (decltype(count) i=0 ; i<count ; ++i)
				header_captions_rows.emplace_back(std::to_string(header_captions_rows.size()), parent_window, color_text_header.r, color_text_header.g, color_text_header.b);
		}

		int get_total_width()
		{
			int total = header_rows_width;
			for (int t : thickness_cols)
				total += t;
			return total;
		}
		int get_total_height()
		{
			int total = header_cols_height;
			for (int t : thickness_rows)
				total += t;
			return total;
		}

		bool does_col_have_selection(unsigned int idx) const
		{
			if (selected_all && ! unselected_cols.contains(idx))
				return true;
			for (auto n : selected_cols)
				if (idx == n)
					return true;
			for (auto p : selected_cells)
				if (idx == p.first)
					return true;
			return false;
		}
		bool does_row_have_selection(unsigned int idx) const
		{
			if (selected_all && ! unselected_rows.contains(idx))
				return true;
			for (auto n : selected_rows)
				if (idx == n)
					return true;
			for (auto p : selected_cells)
				if (idx == p.second)
					return true;
			return false;
		}

		virtual void _redraw() override
		{
			this->clear_background();

			int draw_width  = std::min(this->rect.w, get_total_width ());
			int draw_height = std::min(this->rect.h, get_total_height());

			// column headers
			int x = header_rows_width;
			unsigned int i = 0;
			for (int thickness : thickness_cols)
			{
				int next_x = x + thickness;

				// dark rectangle
				this->drawable_area.fill_rect(x, 0, thickness-1, header_cols_height-1, color_bg_header.r, color_bg_header.g, color_bg_header.b);
				if (does_col_have_selection(i))
					this->drawable_area.fill_rect(x, 0, thickness-1, header_cols_height-1, selected_cells_overlay.r, selected_cells_overlay.g, selected_cells_overlay.b, selected_cells_overlay.a);

				int w_paste = header_captions_cols[i].w;
				int h_paste = header_captions_cols[i].h;
				int x_paste = (x+next_x)/2 - w_paste/2;
				int y_paste = header_cols_height/2 - h_paste/2;

				if (x_paste < x)
				{
					w_paste = thickness;
					x_paste = x;
				}
				if (y_paste < 0)
				{
					h_paste = header_cols_height;
					y_paste = 0;
				}

				this->drawable_area.copy_from(header_captions_cols[i], x_paste, y_paste, w_paste, h_paste);

				if (x > draw_width)
					break;

				x = next_x;
				++i;
			}

			// row headers
			int y = header_cols_height;
			i = 0;
			for (int thickness : thickness_rows)
			{
				int next_y = y + thickness;

				// dark rectangle
				this->drawable_area.fill_rect(0, y, header_rows_width-1, thickness-1, color_bg_header.r, color_bg_header.g, color_bg_header.b);
				if (does_row_have_selection(i))
					this->drawable_area.fill_rect(0, y, header_rows_width-1, thickness-1, selected_cells_overlay.r, selected_cells_overlay.g, selected_cells_overlay.b, selected_cells_overlay.a);

				int w_paste = header_captions_rows[i].w;
				int h_paste = header_captions_rows[i].h;
				int x_paste = header_rows_width/2 - w_paste/2;
				int y_paste = (y+next_y)/2 - h_paste/2;

				if (y_paste < y)
				{
					h_paste = thickness;
					y_paste = y;
				}
				if (x_paste < 0)
				{
					w_paste = header_rows_width;
					x_paste = 0;
				}

				this->drawable_area.copy_from(header_captions_rows[i], x_paste, y_paste, w_paste, h_paste);

				if (y > draw_height)
					break;

				y = next_y;
				++i;
			}

			// cells
			y = header_cols_height;
			unsigned int row_idx = 0;
			for (int thickness_row : thickness_rows)
			{
				x = header_rows_width;
				unsigned int col_idx = 0;
				int next_y = y + thickness_row;
				for (int thickness_col : thickness_cols)
				{
					int next_x = x + thickness_col;

					color_t cell_color = get_cell_color_bg(col_idx, row_idx);
					this->drawable_area.fill_rect(x, y, thickness_col-1, thickness_row-1, cell_color.r, cell_color.g, cell_color.b, cell_color.a);

					if (x > draw_width)
						break;
					x = next_x;
					++col_idx;
				}
				if (y > draw_height)
					break;
				y = next_y;
				++row_idx;
			}

			this->draw_border();
		}

		color_t get_cell_color_bg(unsigned int col_idx, unsigned int row_idx)
		{
			if (is_cell_selected(col_idx, row_idx))
				return selected_cells_overlay;
			else
				return color_cell_bg;
		}

		bool is_cell_selected(unsigned int col_idx, unsigned int row_idx)
		{
			auto p = std::make_pair(col_idx, row_idx);
			if (selected_cells.contains(p))
				return true;
			if (unselected_cells.contains(p))
				return false;
			if (selected_all)
			{
				if (unselected_rows.contains(row_idx) || unselected_cols.contains(col_idx))
					return false;
				return true;
			}
			else
			{
				return (selected_rows.contains(row_idx) || selected_cols.contains(col_idx));
			}
		}

		// returns -1 for header
		int get_col_at(int x) const
		{
			if (x < (int)header_rows_width)
				return -1;
			int result = 0;
			int total_thickness = header_rows_width;
			for (auto & thickness : thickness_cols)
			{
				total_thickness += thickness;
				if (x < total_thickness)
					break;
				++result;
			}
			return result;
		}
		// returns -1 for header
		int get_row_at(int y) const
		{
			if (y < (int)header_cols_height)
				return -1;
			int result = 0;
			int total_thickness = header_cols_height;
			for (auto & thickness : thickness_rows)
			{
				total_thickness += thickness;
				if (y < total_thickness)
					break;
				++result;
			}
			return result;
		}

		void clear_selection()
		{
			  selected_cols.clear();
			  selected_rows.clear();
			unselected_cols.clear();
			unselected_rows.clear();
			  selected_cells.clear();
			unselected_cells.clear();
			selected_all = false;
		}

		// return true if the row was unselected
		bool toggle_unselected_row(unsigned int row_idx)
		{
			bool was_unselected = 0 < unselected_rows.erase(row_idx);
			if ( ! was_unselected)
				unselected_rows.insert(row_idx);
			return was_unselected;
		}
		// return true if the row was selected
		bool toggle_selected_row(unsigned int row_idx)
		{
			bool was_selected = 0 < selected_rows.erase(row_idx);
			if ( ! was_selected)
				selected_rows.insert(row_idx);
			return was_selected;
		}
		// return true if the col was unselected
		bool toggle_unselected_col(unsigned int col_idx)
		{
			bool was_unselected = 0 < unselected_cols.erase(col_idx);
			if ( ! was_unselected)
				unselected_cols.insert(col_idx);
			return was_unselected;
		}
		// return true if the col was selected
		bool toggle_selected_col(unsigned int col_idx)
		{
			bool was_selected = 0 < selected_cols.erase(col_idx);
			if ( ! was_selected)
				selected_cols.insert(col_idx);
			return was_selected;
		}

		void clear_selected_cells_in_row(  unsigned int row_idx)
		{
			std::erase_if(selected_cells, [&](const auto & p){ return p.second == row_idx; });
		}
		void clear_selected_cells_in_col(  unsigned int col_idx)
		{
			std::erase_if(selected_cells, [&](const auto & p){ return p.first == col_idx; });
		}
		void clear_unselected_cells_in_row(unsigned int row_idx)
		{
			std::erase_if(unselected_cells, [&](const auto & p){ return p.second == row_idx && ! selected_cols.contains(p.first); });
		}
		void clear_unselected_cells_in_col(unsigned int col_idx)
		{
			std::erase_if(unselected_cells, [&](const auto & p){ return p.first == col_idx && ! selected_rows.contains(p.second); });
		}
		bool is_col_selected  (unsigned int col_idx) { return selected_cols  .contains(col_idx); }
		bool is_row_selected  (unsigned int row_idx) { return selected_rows  .contains(row_idx); }
		bool is_col_unselected(unsigned int col_idx) { return unselected_cols.contains(col_idx); }
		bool is_row_unselected(unsigned int row_idx) { return unselected_rows.contains(row_idx); }

		void toggle_selected_cell(unsigned int col_idx, unsigned int row_idx)
		{
			auto p = std::make_pair(col_idx, row_idx);
			bool col_is_selected = is_col_selected(col_idx);
			bool row_is_selected = is_row_selected(row_idx);
			bool col_is_unselected = is_col_unselected(col_idx);
			bool row_is_unselected = is_row_unselected(row_idx);

			if (selected_all)
			{
				if ( ! col_is_unselected && ! row_is_unselected)
				{
					if (0 == unselected_cells.erase(p))
					{
						unselected_cells.insert(p);
					}
				}
				else
				{
					if (0 == selected_cells.erase(p))
					{
						selected_cells.insert(p);
					}
				}
			}
			else
			{
				if (col_is_selected || row_is_selected)
				{
					if (0 == unselected_cells.erase(p))
					{
						unselected_cells.insert(p);
					}
				}
				else
				{
					if (0 == selected_cells.erase(p))
					{
						selected_cells.insert(p);
					}
				}
			}
		}

		// returns -2 if none, -1 if header, idx otherwise
		int is_mouse_on_row_header_edge(int mouse_x, int mouse_y)
		{
			if (mouse_x <= (int)header_rows_width + header_resizing_area_thickness)
			{
				// might be on a row's edge in row header
				int y = header_cols_height;
				int i = -1;
				for (int thickness : thickness_rows)
				{
					int next_y = y + thickness;
					if (   mouse_y >= y - header_resizing_area_thickness
						&& mouse_y <= y + header_resizing_area_thickness)
						return i;
					if (y >= this->rect.h)
						break;
					y = next_y;
					++i;
				}
			}
			return -2;
		}
		// returns -2 if none, -1 if header, idx otherwise
		int is_mouse_on_col_header_edge(int mouse_x, int mouse_y)
		{
			if (mouse_y < (int)header_cols_height + header_resizing_area_thickness)
			{
				// might be on a row's edge in row header
				int x = header_rows_width;
				int i = -1;
				for (int thickness : thickness_cols)
				{
					int next_x = x + thickness;
					if (   mouse_x >= x - header_resizing_area_thickness
						&& mouse_x <= x + header_resizing_area_thickness)
						return i;
					if (x >= this->rect.w)
						break;
					x = next_x;
					++i;
				}
			}
			return -2;
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
					int row_edge_idx = is_mouse_on_row_header_edge(ev.data.mouse.x, ev.data.mouse.y);
					int col_edge_idx = is_mouse_on_col_header_edge(ev.data.mouse.x, ev.data.mouse.y); 

					if (ev.data.mouse.pressed != true)
					{
						// mouse move
						if (row_edge_idx != -2)
							parent_window->set_cursor(MouseCursorImg::SIZENS);
						else if (col_edge_idx != -2)
							parent_window->set_cursor(MouseCursorImg::SIZEWE);
						else
							parent_window->set_cursor(MouseCursorImg::ARROW);
						break;
					}

					// mouse click

					this->take_focus();

					if (row_edge_idx == -1)
					{
						unsigned int row_thickness = header_cols_height;
						parent_window->set_cursor(MouseCursorImg::SIZENS);
						this->mouse_grab(this,
							[row_edge_idx,this,row_thickness](Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int drag_x, [[maybe_unused]]int drag_y)
							{
								this->header_cols_height = row_thickness + drag_y - grab_y;
								this->set_needs_redraw();
							},
							[row_edge_idx,this,row_thickness](Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int ungrab_x, [[maybe_unused]]int ungrab_y)
							{
								this->parent_window->set_cursor(MouseCursorImg::ARROW);
								this->header_cols_height = row_thickness + ungrab_y - grab_y;
								this->set_needs_redraw();
							});
						break;
					}
					else if (row_edge_idx >= 0)
					{
						unsigned row_thickness = this->thickness_rows[row_edge_idx];
						parent_window->set_cursor(MouseCursorImg::SIZENS);
						this->mouse_grab(this,
							[row_edge_idx,this,row_thickness](Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int drag_x, [[maybe_unused]]int drag_y)
							{
								this->thickness_rows[row_edge_idx] = row_thickness + drag_y - grab_y;
								this->set_needs_redraw();
							},
							[row_edge_idx,this,row_thickness](Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int ungrab_x, [[maybe_unused]]int ungrab_y)
							{
								this->parent_window->set_cursor(MouseCursorImg::ARROW);
								this->thickness_rows[row_edge_idx] = row_thickness + ungrab_y - grab_y;
								this->set_needs_redraw();
							});
						break;
					}
					else if (col_edge_idx == -1)
					{
						unsigned int col_thickness = header_rows_width;
						parent_window->set_cursor(MouseCursorImg::SIZEWE);
						this->mouse_grab(this,
							[col_edge_idx,this,col_thickness](Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int drag_x, [[maybe_unused]]int drag_y)
							{
								this->header_rows_width = col_thickness + drag_x - grab_x;
								this->set_needs_redraw();
							},
							[col_edge_idx,this,col_thickness](Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int ungrab_x, [[maybe_unused]]int ungrab_y)
							{
								this->parent_window->set_cursor(MouseCursorImg::ARROW);
								this->header_rows_width = col_thickness + ungrab_x - grab_x;
								this->set_needs_redraw();
							});
						break;
					}
					else if (col_edge_idx >= 0)
					{
						unsigned int col_thickness = this->thickness_cols[col_edge_idx];
						parent_window->set_cursor(MouseCursorImg::SIZEWE);
						this->mouse_grab(this,
							[col_edge_idx,this,col_thickness](Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int drag_x, [[maybe_unused]]int drag_y)
							{
								this->thickness_cols[col_edge_idx] = col_thickness + drag_x - grab_x;
								this->set_needs_redraw();
							},
							[col_edge_idx,this,col_thickness](Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int ungrab_x, [[maybe_unused]]int ungrab_y)
							{
								this->parent_window->set_cursor(MouseCursorImg::ARROW);
								this->thickness_cols[col_edge_idx] = col_thickness + ungrab_x - grab_x;
								this->set_needs_redraw();
							});
						break;
					}

					bool changed = false;
					int col_idx = get_col_at(ev.data.mouse.x);
					int row_idx = get_row_at(ev.data.mouse.y);
					if ( ! key_ctrl)
					{
						clear_selection();
						if (col_idx == -1 && row_idx == -1)
						{
							changed |= ! selected_all;
							selected_all = true;
						}
						else if (col_idx == -1)
						{
							if ( ! is_row_selected(row_idx))
							{
								changed |= true;
								selected_rows.insert(row_idx);
								clear_unselected_cells_in_row(row_idx);
							}
						}
						else if (row_idx == -1)
						{
							if ( ! is_col_selected(col_idx))
							{
								changed |= true;
								selected_cols.insert(col_idx);
								clear_unselected_cells_in_col(col_idx);
							}
						}
						else
						{
							selected_cells.emplace(col_idx,row_idx);
							changed |= true;
						}
					}
					else
					{
						// ctrl is held down

						if (col_idx == -1 && row_idx == -1)
						{
							clear_selection();
							changed |= ! selected_all;
							selected_all = true;
						}
						else if (col_idx == -1)
						{
							changed |= true;
							if (selected_all)
							{
								bool was_unselected = toggle_unselected_row((unsigned int)row_idx);
								if (was_unselected)
									clear_selected_cells_in_row(row_idx);
								else
									clear_unselected_cells_in_row(row_idx);
							}
							else
							{
								bool was_selected = toggle_selected_row((unsigned int)row_idx);
								if (was_selected)
									clear_unselected_cells_in_row(row_idx);
								else
									clear_selected_cells_in_row(row_idx);
							}
						}
						else if (row_idx == -1)
						{
							changed |= true;
							if (selected_all)
							{
								bool was_unselected = toggle_unselected_col((unsigned int)col_idx);
								if (was_unselected)
									clear_selected_cells_in_col(col_idx);
								else
									clear_unselected_cells_in_col(col_idx);
							}
							else
							{
								bool was_selected = toggle_selected_col((unsigned int)col_idx);
								if (was_selected)
									clear_unselected_cells_in_col(col_idx);
								else
									clear_selected_cells_in_col(col_idx);
							}
						}
						else
						{
							changed |= true;
							toggle_selected_cell(col_idx, row_idx);
						}
					}
					if (changed)
						this->set_needs_redraw();
					return changed;
				}
				case key:
					if (ev.data.key.keycode == Scancode::Ctrl)
						key_ctrl = ev.data.key.pressed;
					break;
				default:
					break;
			}
			return false;
		}
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
		int grab_x, grab_y;
		std::function<void(Widget *, int /*grab_x*/, int /*grab_y*/, int /*drag_x*/, int /*drag_y*/)> drag_f;
		std::function<void(Widget *, int /*grab_x*/, int /*grab_y*/, int /*drag_x*/, int /*drag_y*/)> ungrab_f;

		void operator()(Widget * w, int x, int y, decltype(drag_f) drag_func, decltype(ungrab_f) ungrab_func)
		{
			if ( ! w)
				return;
			widget = w;
			grab_x = x;
			grab_y = y;
			drag_f = drag_func;
			ungrab_f = ungrab_func;
			widget->set_mouse_grabbed(true);
			//drag_f(widget, grab_x, grab_y, x, y);
			//widget->set_mouse_grabbed(false);
			//widget = w;
			//if (widget)
			//	widget->set_mouse_grabbed(true);
		}
		void drag(int x, int y)
		{
			if (drag_f)
				drag_f(widget, grab_x, grab_y, x, y);
		}
		void ungrab(int x, int y)
		{
			if (ungrab_f)
				ungrab_f(widget, grab_x, grab_y, x, y);
			widget->set_mouse_grabbed(false);
			widget = nullptr;
		}
		operator Widget*() { return widget; }
	};

	struct Window : public WSW::Window_t
	{
		Container container;
		focus_holder focus;
		mouse_grabber mousegrab;
		std::vector<PopupMenu*> popups;
		event current_event;

		Window(const char * title, int width, int height)
			: WSW::Window_t(title, width, height)
			, container(this, Rect{0, 0, width, height})
		{
			container.border_width  = 0;
			container.border_padding = 0;
			container.inter_padding = 0;
		}

		void set_focus(Widget * widg)
		{
			focus(widg);
		}
		void mouse_grab(Widget * widg, 
		                std::function<void(Widget *, int /*grab_x*/, int /*grab_y*/, int /*drag_x*/, int /*drag_y*/)> drag_f,
		                std::function<void(Widget *, int /*grab_x*/, int /*grab_y*/, int /*drag_x*/, int /*drag_y*/)> ungrab_f)
		{
			mousegrab(widg, current_event.data.mouse.x, current_event.data.mouse.y, drag_f, ungrab_f);
		}
		//void mouse_release(Widget * widg)
		//{
		//	if (widg == mousegrab)
		//		mousegrab(nullptr);
		//}

		void add_popup(PopupMenu * menu)
		{
			menu->parent_container = &container;
			popups.push_back(menu);
			container.set_needs_redraw();
		}
		void clear_popups()
		{
			popups.clear();
			container.set_needs_redraw();
		}
		virtual bool on_size_set([[maybe_unused]]int w, [[maybe_unused]]int h) { return false; }
		virtual bool handle_event(event ev) override
		{
			current_event = ev;
			switch(ev.type)
			{
				case nop:
					return true;
				case window_shown:
					break;
				case window_resized:
					this->container.set_size({ev.data.window_resized.w, ev.data.window_resized.h});
					on_size_set(ev.data.window_resized.w, ev.data.window_resized.h);
					break;
				case mouse:
				{
					//std::cout << "win evt mouse, pressed " << ev.data.mouse.pressed << std::endl;
					//std::cout << "win evt mouse, released " << ev.data.mouse.released << std::endl;
					//std::cout << "win popu count " << popups.size() << std::endl;

					bool event_caught_by_popup = false;

					int x = ev.data.mouse.x;
					int y = ev.data.mouse.y;
					auto itr  = popups.rbegin();
					auto endr = popups.rend();
					for ( ; itr!=endr ; ++itr)
					{
						PopupMenu * popup = *itr;
						if (   x >= popup->rect.x && x < popup->rect.x + popup->rect.w
							&& y >= popup->rect.y && y < popup->rect.y + popup->rect.h )
						{
							auto ev2 = ev;
							ev2.data.mouse.x -= popup->rect.x;
							ev2.data.mouse.y -= popup->rect.y;
							popup->handle_event(ev2);
							event_caught_by_popup = true;
							break;
						}
					}

					if ( ! event_caught_by_popup)
					{
						if (ev.data.mouse.pressed)
						{
							if (popups.size() > 0)
							{
								if (itr == endr)
									popups.clear();
								else
									popups.erase(std::next(itr).base(), popups.end());
								container.set_needs_redraw();
							}
						}

						if (mousegrab)
						{
							if ( ! ev.data.mouse.released)
								mousegrab.drag(ev.data.mouse.x, ev.data.mouse.y);
							else
								mousegrab.ungrab(ev.data.mouse.x, ev.data.mouse.y);
						}
						else
						{
							Widget * widget = this->container.find_widget_at(x, y);
							if (widget)
							{
								ev.data.mouse.x -= widget->rect.x;
								ev.data.mouse.y -= widget->rect.y;
								widget->handle_event(ev);
							}
						}
					}
					break;
				}
				case key:
				{
					Widget * widget = focus;
					if (widget)
						widget->handle_event(ev);
					break;
				}
				default:
					break;
			}
			_redraw();
			return false;
		}

		virtual void _redraw()
		{
			if (container.needs_redraw)
			{
				container._redraw();
				for (PopupMenu * menu : popups)
				{
					if (menu->needs_redraw)
						menu->_redraw();
					container.drawable_area.copy_from(menu->drawable_area, menu->rect.x, menu->rect.y);
				}
				container.drawable_area.refresh_window();
				WSW::Window_t::present();
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
			return w;
		}

		void loop()
		{
			window_system_wrapper.loop();
		}
	};

}; // struct-namespace
