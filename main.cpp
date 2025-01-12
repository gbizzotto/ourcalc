
#include "sdl_wrapper.hpp"
#include "our_windows.hpp"


using WindowManager = OW::Manager<SDL>;

struct bw_window : WindowManager::Window_t
{
	using Super = typename WindowManager::Window_t;
	OW::Button & butt;

	bw_window()
		: butt(make_widget<std::remove_reference<decltype(butt)>::type>({10,50,100,20}))
	{
	}

	virtual void event_button_down() override
	{
		fill(0,0,0);
		
	}
	virtual void event_button_up() override
	{
		fill(255,255,255);
	}
};


int main()
{
	WindowManager wm;

	auto & w = wm.make_window<bw_window>();

	wm.loop();

	return 0;
}
