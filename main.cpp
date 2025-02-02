
#include "sdl_wrapper.hpp"
#include "our_windows.hpp"


using OurW = OW<SDL>;

struct my_button_t : OurW::Button
{
	OurW::Button * butt1;

	my_button_t(OurW::Container * container, std::string t, OurW::Rect r)
		: OurW::Button(container, t, r)
	{}

	void set_target(OurW::Button & b)
	{
		butt1 = &b;
	}

	virtual void event_clicked() override
	{
		butt1->set_text("123");
	}
};

struct my_window : OurW::Window
{
	using Super = typename OurW::Window;
	OurW::Splitter split1;
	OurW::Button butt1;
	my_button_t  butt2;
	OurW::TextEdit text1;
	OurW::Label label1;

	my_window(const char * title, int width, int height)
		: OurW::Window(title, width, height)
		, split1(&this->container, {200, 200, 400, 400}, true)
		, butt1 (&split1.one, "button 1", {10, 50,200,40})
		, butt2 (&split1.one, "button 2", {10,150,200,40})
		, text1 (&split1.two, "asdf", {10,200,200,40})
		, label1(&split1.two, "gxb", {10,250,200,40})
	{
		butt2.set_target(butt1);

		//split1.one.add_widget(&butt1);
		//split1.one.add_widget(&butt2);
		//split1.two.add_widget(&text1);
		//split1.two.add_widget(&label1);

		split1.two.set_layout(std::make_unique<OurW::VLayout>(vertical_alignment_t::bottom));

		//this->container.add_widget(&split1, true);

		butt1.pack();
		text1.pack();

		//container.event_redraw();
	}
};


int main()
{
	OurW::Manager wm;

	/*auto & w = */wm.make_window<my_window>("Test", 1024, 768);
	//w.set_layout_type<OurW::VLayout>();

	wm.loop();

	return 0;
}
