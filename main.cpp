
#include <iostream>

#include "sdl_wrapper.hpp"
#include "our_windows.hpp"


using OurW = OW<SDL>;

struct my_window : OurW::Window
{
	using Super = typename OurW::Window;
	OurW::Container top_container;
	OurW::MenuBar menubar1;
	OurW::MenuItem menufile;
	OurW::MenuItem menuedit;
	OurW::Container middle_container;
	OurW::Splitter split1;
	OurW::Button butt1;
	OurW::Button butt2;
	OurW::TextEdit text1;
	OurW::Label label1;

	OurW::PopupMenu menufile_popup;
	OurW::MenuItem menufile_open;
	OurW::MenuItem menufile_quit;

	my_window(const char * title, int width, int height)
		: OurW::Window(title, width, height)
		, top_container(this, {0,0,width,height})
		, menubar1(this)
		, menufile(this, "File", {0,0,10,10}, [&](){ /*std::cout << "open clicked" << std::endl;*/ })
		, menuedit(this, "Edit", {0,0,10,10}, [&](){ butt1.set_text("poiu"); })
		, middle_container(this, {0,0,400,400})
		, split1(this, {200, 200, 400, 400}, false)
		, butt1(this, "button 1", {10, 50,200,40}, [](){})
		, butt2(this, "button 2", {10,150,200,40}, [&](){ butt1.set_text("123"); })
		, text1(this, "asdf", {10,200,200,40})
		, label1(this, "gxb", {10,250,200,40})
		, menufile_popup(this)
		, menufile_open(this, "Open...", {0,0,10,10}, [&](){  })
		, menufile_quit(this, "Quit", {0,0,10,10}, [&](){ exit(0); })
	{
		this->top_container.border_padding = 0;
		this->top_container.inter_padding = 0;
		this->top_container.border_width = 0;
		this->top_container.set_layout(std::make_unique<OurW::VLayout>(horizontal_policy{horizontal_policy::alignment_t::left, horizontal_policy::sizing_t::fill}
		                                                          ,  vertical_policy{  vertical_policy::alignment_t::top ,   vertical_policy::sizing_t::fill}));

		middle_container.set_layout(std::make_unique<OurW::VLayout>(horizontal_policy{horizontal_policy::alignment_t::left, horizontal_policy::sizing_t::fill}
		                                                           ,  vertical_policy{  vertical_policy::alignment_t::top ,   vertical_policy::sizing_t::fill}));
		butt1.pack();
		text1.pack();

		this->top_container.add_widget(menubar1);
		menubar1.set_height(menufile.height_packed() + 2*(menubar1.border_width+menubar1.border_padding));
		menubar1.add_widget(menufile);
		menubar1.add_widget(menuedit);
		this->top_container.add_widget(middle_container);
		middle_container.border_padding = 0;
		middle_container.add_widget(split1);
		split1.one.add_widget(butt1 );
		split1.one.add_widget(butt2 );
		split1.one.add_widget(text1 );
		split1.one.add_widget(label1);
		split1.one.set_layout(std::make_unique<OurW::VLayout>(horizontal_policy{horizontal_policy::alignment_t::center, horizontal_policy::sizing_t::justify}
		                                                     ,  vertical_policy{  vertical_policy::alignment_t::center,   vertical_policy::sizing_t::justify}));

		menufile_popup.add_widget(menufile_open);
		menufile_popup.add_widget(menufile_quit);

		menufile.set_action([&]()
			{
				this->add_popup(&menufile_popup);
			});

		this->container.add_widget(top_container);
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
