
#include <iostream>

#include "sdl_wrapper.hpp"
#include "our_windows.hpp"
#include "ourgrid.hpp"


using OurW = OW<SDL>;

struct my_window : OurW::Window
{
	using Super = typename OurW::Window;
	OurW::Container top_container;
	OurW::MenuBar menubar;
	OurW::TextEdit text1;
	Grid<OurW> grid1;

	my_window(const char * title, int width, int height)
		: OurW::Window(title, width, height)
		, top_container(this)
		, menubar(this)
		, text1(this, "")
		, grid1(this, text1)
	{
		global_grid = &grid1;

		this->top_container.border_width = 0;
		this->top_container.border_padding = 0;
		this->top_container.inter_padding = 0;
		this->top_container.set_layout(std::make_unique<OurW::VLayout>(horizontal_policy{horizontal_policy::alignment_t::left, horizontal_policy::sizing_t::fill}
		                                                              ,  vertical_policy{  vertical_policy::alignment_t::top ,   vertical_policy::sizing_t::fill}));

		this->top_container.add_widget(menubar);
		this->top_container.add_widget(text1);
		this->top_container.add_widget(grid1);

		auto & menu_file = menubar.add_submenu("Test1");
		/*auto & menu_edit = */menubar.add("Test2", [](){});
		auto & menu_file_open = menu_file.add_submenu("Submenu Test 1");
		menu_file.add("Quit", [](){ exit(0); });
		menu_file_open.add("Aasdfasdflkjsadf" , [&](){ });
		menu_file_open.add("bkjlkjblkjblkblkj", [&](){ });
		menubar.vpack();

		top_container.set_size({w, h});
		
		this->container.add_widget(top_container);

		grid1.take_focus();
	}

	virtual bool on_size_set(int w, int h)
	{
		return top_container.set_size({w, h});
	}
};


int main()
{
	assert(number_to_column_code(26*27+26) == "ABA");
	OurW::Manager wm;

	for (unsigned int i=0 ; i<55 ; ++i)
		if (i != parse_col_name(number_to_column_code(i)))
			std::cout << "Bad: " << i << ", col: " << number_to_column_code(i) << " != " << parse_col_name(number_to_column_code(i)) << std::endl;

	/*auto & w = */wm.make_window<my_window>("OurCalc", 1024, 768);

	wm.loop();

	return 0;
}
