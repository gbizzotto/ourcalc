
#pragma once

namespace OW
{

template<typename WSW>
struct Window : public WSW::WindowHandler
{	
	typename WSW::Window * underlying_window;

	void set_underlying(WSW::Window & underlying)
	{
		underlying_window = &underlying;
	}
};

template<typename WSW>
struct Manager
{
	WSW window_system_wrapper;

	template<typename W>
	W make_window()
	{
		W w;
		auto & underlying = window_system_wrapper.make_window(w);
		w.set_underlying(underlying);
		underlying.set_handler(w);
		return w;
	}

	void loop()
	{
		window_system_wrapper.loop();
	}
};

} // namespace
