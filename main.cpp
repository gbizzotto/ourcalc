
#include "sdl_wrapper.hpp"
#include "our_windows.hpp"


using OurW = OW<SDL>;

struct my_button_t : OurW::Button
{
	OurW::Button * butt1;

	my_button_t(OurW::Window * parent_window, std::string t, OurW::Rect r)
		: OurW::Button(parent_window, t, r)
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
	OurW::TextEdit text1;
	OurW::Label label1;

	my_window(const char * title, int width, int height)
		: OurW::Window(title, width, height)
		, butt1(this, "button 1", {10, 50,200,40})
		, butt2(this, "button 2", {10,150,200,40})
		, text1(this, "asdf", {10,200,200,40})
		, label1(this, "gxb", {10,250,200,40})
	{
		butt2.set_target(butt1);

		this->layout->add_widget(&butt1);
		this->layout->add_widget(&butt2);
		this->layout->add_widget(&text1);
		this->layout->add_widget(&label1);

		Container::event_redraw();
	}
};


int main()
{
	OurW::Manager wm;

	auto & w = wm.make_window<my_window>("Test", 1024, 768);
	w.set_layout_type<OurW::VLayout>();

	wm.loop();

	return 0;
}
