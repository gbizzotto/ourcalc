
#pragma once

namespace OW
{

template<typename WSW>
struct Window : public WSW::Window_t
{
};

template<typename WSW>
struct Manager
{
	using Window_t = typename WSW::Window_t;

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
