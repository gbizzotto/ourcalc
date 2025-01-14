
#include "sdl_wrapper.hpp"
#include "our_windows.hpp"


using OurW = OW<SDL>;

struct my_button_t : OurW::Button
{
	OurW::Button * butt1;

	my_button_t(std::string t, OurW::Rect r)
		: OurW::Button(t, r)
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
	OurW::Button butt1;
	my_button_t  butt2;

	my_window()
		: butt1("button 1", {10, 50,200,40})
		, butt2("button 2", {10,150,200,40})
	{
		butt2.set_target(butt1);

		this->add_widget(&butt1);
		this->add_widget(&butt2);
	}
};


int main()
{
	OurW::Manager wm;

	auto & w = wm.make_window<my_window>();

	wm.loop();

	return 0;
}
