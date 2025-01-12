
#pragma once

namespace OW
{

struct Rect
{
	int x,y,w,h;
};

struct Widget
{
	Rect rect;
};

struct Button : Widget
{
	Button(Rect r)
		: Widget{r}
	{}
};

template<typename WSW>
struct Window : public WSW::Window_t
{
	std::vector<std::unique_ptr<Widget>> widgets;

	template<typename WDG>
	WDG & make_widget(Rect r)
	{
		widgets.emplace_back(std::make_unique<WDG>(r));
		return (WDG&)*widgets.back();
	}
};

template<typename WSW>
struct Manager
{
	using Window_t = Window<WSW>;

	WSW window_system_wrapper;

	template<typename W>
	Window_t & make_window()
	{
		return window_system_wrapper.template make_window<W>();
	}

	void loop()
	{
		window_system_wrapper.loop();
	}
};

} // namespace
