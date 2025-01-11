
#include "sdl_wrapper.hpp"
#include "our_windows.hpp"


using WindowManager = OW::Manager<SDL>;

struct bw_window : public WindowManager::Window_t
{
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
