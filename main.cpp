
#include "sdl_wrapper.hpp"
#include "our_windows.hpp"


using OurW = OW<SDL>;

struct my_button_t : OurW::Button
{
	OurW::Button * butt1;

	my_button_t(OurW::Window & _w, OurW::Rect _r)
		: OurW::Button(_w, _r)
	{}

	void set_target(OurW::Button & b)
	{
		butt1 = &b;
	}

	virtual void event_clicked() override
	{
		butt1->set_text("zzzzzzzzz");
	}
};

struct my_window : OurW::Window
{
	using Super = typename OurW::Window;
	OurW::Button & butt1;
	my_button_t & butt2;

	my_window()
		: butt1(make_widget<std::remove_reference<decltype(butt1)>::type>({10,50,200,40}))
		, butt2(make_widget<std::remove_reference<decltype(butt2)>::type>({10,150,200,40}))
	{
		butt1.set_text("abcd");
		butt2.set_target(butt1);
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
