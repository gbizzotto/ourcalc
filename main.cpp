
#include "sdl_wrapper.hpp"
#include "our_windows.hpp"


using OurW = OW<SDL>;

struct my_window : OurW::Window
{
	using Super = typename OurW::Window;
	OurW::Button & butt;

	my_window()
		: butt(make_widget<std::remove_reference<decltype(butt)>::type>({10,50,100,20}))
	{
	}
/*
	virtual void event_redraw() override
	{
		fill(255,255,255);	
		Super::ok();
	}

	virtual void event_mouse_button_down() override
	{
		fill(0,0,0);		
	}
	virtual void event_mouse_button_up() override
	{
		fill(255,255,255);
	}
*/
};


int main()
{
	OurW::Manager wm;

	auto & w = wm.make_window<my_window>();

	wm.loop();

	return 0;
}
