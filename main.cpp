
#include "sdl_wrapper.hpp"
#include "our_windows.hpp"


using WindowManager = OW::Manager<SDL>;

struct bw_window : public OW::Window<SDL>
{
	virtual void event_button_down() override
	{
		underlying_window->fill(0,0,0);
		
	}
	virtual void event_button_up() override
	{
		underlying_window->fill(255,255,255);
	}
};


int main()
{
	OW::Manager<SDL> ow;

	auto w = ow.make_window<bw_window>();

	ow.loop();

	return 0;
}
