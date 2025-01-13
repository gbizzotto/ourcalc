
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

		Widget(Window & w, Rect r)
			: w(w)
			, rect(r)
		{}

		virtual void draw() {}
	};

	struct Button : Widget
	{
		Button(Window & w, Rect r)
			: Widget{w, r}
		{}
		
		virtual void draw() override
		{
			this->w.draw_rect(this->rect.x, this->rect.y, this->rect.w, this->rect.h, 128, 128, 128);
		}
	};

	struct Window : public WSW::Window_t
	{
		std::vector<std::unique_ptr<Widget>> widgets;

		template<typename WDG>
		WDG & make_widget(Rect r)
		{
			widgets.emplace_back(std::make_unique<WDG>(*this, r));
			widgets.back()->draw();
			return (WDG&)*widgets.back();
		}

		virtual void event_redraw()
		{
			this->fill(196,196,196);
			for (auto & widget : widgets)
				widget->draw();
		}

		virtual void event_mouse_button_down() {}
		virtual void event_mouse_button_up() {}
	};

	struct Manager
	{
		WSW window_system_wrapper;

		template<typename W>
		Window & make_window()
		{
			return window_system_wrapper.template make_window<W>();
		}

		void loop()
		{
			window_system_wrapper.loop();
		}
	};

}; // struct-namespace
