
#include "sdl_wrapper.hpp"
#include "our_windows.hpp"


using OurW = OW<SDL>;

struct my_window : OurW::Window
{
	using Super = typename OurW::Window;
	OurW::MenuBar menubar1;
	OurW::MenuItem menufile;
	OurW::Container middle_container;
	OurW::Splitter split1;
	OurW::Button butt1;
	OurW::Button butt2;
	OurW::TextEdit text1;
	OurW::Label label1;

	my_window(const char * title, int width, int height)
		: OurW::Window(title, width, height)
		, menubar1(this)
		, menufile(this, "File", {0,0,10,10}, [&](){ butt1.set_text("abcd"); })
		, middle_container(this, {0,0,400,400})
		, split1(this, {200, 200, 400, 400}, true)
		, butt1(this, "button 1", {10, 50,200,40}, [](){})
		, butt2(this, "button 2", {10,150,200,40}, [&](){ butt1.set_text("123"); })
		, text1(this, "asdf", {10,200,200,40})
		, label1(this, "gxb", {10,250,200,40})
	{
		this->container.set_layout(std::make_unique<OurW::VLayout>(vertical_alignment_t::fill));

		this->container.add_widget(menubar1);
		menubar1.add_widget(menufile, true);
		this->container.add_widget(middle_container);
		middle_container.add_widget(split1);
		split1.one.add_widget(butt1 );
		split1.one.add_widget(butt2 );
		split1.two.add_widget(text1 );
		split1.two.add_widget(label1);


		//split1.one.add_widget(&butt1);
		//split1.one.add_widget(&butt2);
		//split1.two.add_widget(&text1);
		//split1.two.add_widget(&label1);

		split1.two.set_layout(std::make_unique<OurW::VLayout>(vertical_alignment_t::bottom));

		//this->container.add_widget(&split1, true);

		butt1.pack();
		text1.pack();

		//container._redraw();
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
