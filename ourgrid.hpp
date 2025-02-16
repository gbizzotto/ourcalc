
#pragma once


#include "pybind11/pybind11.h"
#include "pybind11/embed.h"

#include <iostream>
#include <vector>
#include "our_windows.hpp"
#include "sdl_wrapper.hpp"

namespace py = pybind11;

/*
int add(int i, int j) {
    return i + j;
}
PYBIND11_MODULE(example, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring

    m.def("add", &add, "A function that adds two numbers");
}
*/

std::string column_name_from_int(int c);

struct CellData
{
	icu::UnicodeString formula;
	std::string type;
	Text display;
	bool error = false;
	std::set<std::pair<unsigned int,unsigned int>> dependencies = std::set<std::pair<unsigned int,unsigned int>>();
	std::string error_msg = std::string("");
	std::vector<std::pair<unsigned int, unsigned int>> dependent_cells = {};

	bool do_dependencies_depend_on_us(decltype(dependencies) deps, unsigned int col, unsigned int row);
	void clear_dependencies(unsigned int col, unsigned int row);
	bool reevaluate(int col, int row);
	bool set_formula(icu::UnicodeString contents, int col, int row)
	{
		if (formula == contents)
			return false;

		clear_dependencies(col, row);

		formula = contents;
		return reevaluate(col, row);
	}
	void add_dependent(unsigned int col, unsigned int row)
	{
		auto p = std::make_pair(col, row);
		auto it = std::lower_bound(std::begin(dependent_cells), std::end(dependent_cells), p);
		if (it == std::end(dependent_cells) || *it != p)
			dependent_cells.insert(it, p);
	}
	void remove_dependent(unsigned int col, unsigned int row)
	{
		auto p = std::make_pair(col, row);
		auto it = std::lower_bound(std::begin(dependent_cells), std::end(dependent_cells), p);
		if (it != std::end(dependent_cells) && *it == p)
			dependent_cells.erase(it);
	}
};

template<typename T>
struct Grid : T::Widget
{
	inline static const color_t color_cell_bg          = color_t(255);
	inline static const color_t color_text_header      = color_t(32);
	inline static const color_t color_lines            = color_t(160);
	inline static const color_t color_bg_header        = color_t(210);
	inline static const color_t selected_cells_overlay = color_t(128,200,128,96);
	inline static const color_t color_active_cell      = color_t(0);
	inline static const int header_resizing_area_thickness = 2;

	Window * parent_window;
	T::TextEdit & editor;

	std::pair<unsigned int, unsigned int> active_cell;

	std::vector<std::vector<CellData>> cell_data;

	unsigned int header_cols_height = 18;
	unsigned int header_rows_width = 40;

	std::vector<unsigned int> thickness_cols;
	std::vector<unsigned int> thickness_rows;

	std::set<unsigned int>   selected_cols;
	std::set<unsigned int>   selected_rows;
	std::set<unsigned int> unselected_cols;
	std::set<unsigned int> unselected_rows;
	std::set<std::pair<unsigned int,unsigned int>>   selected_cells;
	std::set<std::pair<unsigned int,unsigned int>> unselected_cells;
	bool selected_all = false;		

	std::deque<Text> header_captions_cols;
	std::deque<Text> header_captions_rows;

	bool key_ctrl = false;

	py::scoped_interpreter guard;

	Text error_display;

	py::dict globals;
	py::dict locals;

	Grid(T::Window * window, T::TextEdit & edit)
		: T::Widget(window, {0,0,200,200})
		, parent_window(window)
		, editor(edit)
		, active_cell{std::numeric_limits<unsigned int>::max(),std::numeric_limits<unsigned int>::max()}
		, error_display(std::string("Error"), window, 255,0,0,255)
	{
		this->border_width = 0;
		this->border_padding = 0;
		this-> inter_padding = 0;
		this->color_bg = 160;

		run_python("from ourcalc import *");

		insert_columns(20, 0);
		insert_rows(100, 0);

		set_active_cell(0,0);

	}

	void run_python(std::string code)
	{
		try
		{
			//std::cout << "Running:" << std::endl
			//          << code << std::endl;
			py::exec(code, globals, locals);
		}
		catch(std::exception & e)
		{
			std::cout << e.what() << std::endl;
		}
		catch(...)
		{
			std::cout << "Unknown exception" << std::endl;
		}
	}

	virtual int  width_packed() { return 0; }
	virtual int height_packed() { return 0; }

	bool has_integrity() const
	{
		unsigned int row_count = get_row_count();
		unsigned int col_count = get_col_count();

		// row count
		assert(row_count == cell_data.size());
		assert(row_count == thickness_rows.size());
		assert(row_count >= selected_rows.size());

		// column count
		for (auto & row : cell_data)
			assert(row.size() == col_count);
		assert(col_count == thickness_cols.size());
		assert(col_count >=  selected_cols.size());

		// selection
		assert (selected_cells.size() <= col_count * row_count);

		return true;
	}

	void insert_columns(unsigned int count, unsigned int before_idx)
	{
		assert(has_integrity());

		if (before_idx > get_col_count())
			return;
		
		std::string code;
		int row_number = 0;
		for (auto & row : cell_data)
		{
			row.insert(std::next(std::begin(row), before_idx), count, {"", "", Text(parent_window)});

			for (unsigned int col_number=before_idx,i=0 ; i<count ; ++i,++col_number)
				code.append(column_name_from_int(col_number))
				    .append(std::to_string(row_number))
				    .append("=make_ourcell('',")
				    .append(std::to_string(col_number))
				    .append(",")
				    .append(std::to_string(row_number))
				    .append(")\n");
			++row_number;
		}
		thickness_cols.insert(std::next(std::begin(thickness_cols), before_idx), count, 50);

		run_python(code);

		// column headers
		for (decltype(count) i=0 ; i<count ; ++i)
			header_captions_cols.emplace_back(number_to_column_code(header_captions_cols.size()), parent_window, color_text_header.r, color_text_header.g, color_text_header.b);
	}
	void insert_rows(unsigned int count, unsigned int before_idx)
	{
		assert(has_integrity());

		if (before_idx > get_row_count())
			return;

		std::string code;
		for (unsigned col_number=0 ; col_number < thickness_cols.size() ; ++col_number)
		{
			for (unsigned int row_number=before_idx,i=0 ; i<count ; ++i,++row_number)
				code.append(column_name_from_int(col_number))
				    .append(std::to_string(row_number))
				    .append("=make_ourcell('',")
				    .append(std::to_string(col_number))
				    .append(",")
				    .append(std::to_string(row_number))
				    .append(")\n");
		}

		cell_data.insert(std::next(std::begin(cell_data), before_idx), count, std::vector<CellData>(count, {"", "", Text(parent_window)}));
		thickness_rows.insert(std::next(std::begin(thickness_rows), before_idx), count, 18);

		run_python(code);

		// row headers
		for (decltype(count) i=0 ; i<count ; ++i)
			header_captions_rows.emplace_back(std::to_string(header_captions_rows.size()), parent_window, color_text_header.r, color_text_header.g, color_text_header.b);
	}

	int get_total_width()
	{
		int total = header_rows_width;
		for (int t : thickness_cols)
			total += t;
		return total;
	}
	int get_total_height()
	{
		int total = header_cols_height;
		for (int t : thickness_rows)
			total += t;
		return total;
	}

	bool does_col_have_selection(unsigned int idx) const
	{
		if (selected_all && ! unselected_cols.contains(idx))
			return true;
		for (auto n : selected_cols)
			if (idx == n)
				return true;
		for (auto p : selected_cells)
			if (idx == p.first)
				return true;
		return false;
	}
	bool does_row_have_selection(unsigned int idx) const
	{
		if (selected_all && ! unselected_rows.contains(idx))
			return true;
		for (auto n : selected_rows)
			if (idx == n)
				return true;
		for (auto p : selected_cells)
			if (idx == p.second)
				return true;
		return false;
	}

	virtual void _redraw() override
	{
		this->clear_background();

		int draw_width  = std::min(this->rect.w, get_total_width ());
		int draw_height = std::min(this->rect.h, get_total_height());

		// column headers
		int x = header_rows_width;
		unsigned int i = 0;
		for (int thickness : thickness_cols)
		{
			int next_x = x + thickness;

			// dark rectangle
			this->drawable_area.fill_rect(x, 0, thickness-1, header_cols_height-1, color_bg_header.r, color_bg_header.g, color_bg_header.b);
			if (does_col_have_selection(i))
				this->drawable_area.fill_rect(x, 0, thickness-1, header_cols_height-1, selected_cells_overlay.r, selected_cells_overlay.g, selected_cells_overlay.b, selected_cells_overlay.a);

			this->drawable_area.copy_from_text_to_rect(header_captions_cols[i], x, 0, thickness, header_cols_height);

			if (x > draw_width)
				break;

			x = next_x;
			++i;
		}

		// row headers
		int y = header_cols_height;
		i = 0;
		for (int thickness : thickness_rows)
		{
			int next_y = y + thickness;

			// dark rectangle
			this->drawable_area.fill_rect(0, y, header_rows_width-1, thickness-1, color_bg_header.r, color_bg_header.g, color_bg_header.b);
			if (does_row_have_selection(i))
				this->drawable_area.fill_rect(0, y, header_rows_width-1, thickness-1, selected_cells_overlay.r, selected_cells_overlay.g, selected_cells_overlay.b, selected_cells_overlay.a);

			this->drawable_area.copy_from_text_to_rect(header_captions_rows[i], 0, y, header_rows_width, thickness);

			if (y > draw_height)
				break;

			y = next_y;
			++i;
		}

		// cells
		y = header_cols_height;
		unsigned int row_idx = 0;
		for (int thickness_row : thickness_rows)
		{
			x = header_rows_width;
			unsigned int col_idx = 0;
			int next_y = y + thickness_row;
			for (int thickness_col : thickness_cols)
			{
				int next_x = x + thickness_col;

				color_t cell_color = get_cell_color_bg(col_idx, row_idx);
				this->drawable_area.fill_rect(x, y, thickness_col-1, thickness_row-1, cell_color.r, cell_color.g, cell_color.b, cell_color.a);

				if (cell_data[row_idx][col_idx].formula.length() > 0)
				{
					if (cell_data[row_idx][col_idx].error)
						this->drawable_area.copy_from_text_to_rect(error_display, x, y, thickness_col-1, thickness_row-1);
					else
						this->drawable_area.copy_from_text_to_rect(cell_data[row_idx][col_idx].display, x, y, thickness_col-1, thickness_row-1);
				}

				if (active_cell.first == col_idx && active_cell.second == row_idx)
				{
					this->drawable_area.draw_rect(x, y, thickness_col-1, thickness_row-1, color_active_cell.r, color_active_cell.g, color_active_cell.b, color_active_cell.a);
					//this->drawable_area.draw_rect(x-1, y-1, thickness_col+1, thickness_row+1, color_active_cell.r, color_active_cell.g, color_active_cell.b, color_active_cell.a);
				}

				if (x > draw_width)
					break;
				x = next_x;
				++col_idx;
			}
			if (y > draw_height)
				break;
			y = next_y;
			++row_idx;
		}

		this->draw_border();
	}

	color_t get_cell_color_bg(unsigned int col_idx, unsigned int row_idx)
	{
		if (is_cell_selected(col_idx, row_idx))
			return selected_cells_overlay;
		else
			return color_cell_bg;
	}

	bool is_cell_selected(unsigned int col_idx, unsigned int row_idx)
	{
		auto p = std::make_pair(col_idx, row_idx);
		if (selected_cells.contains(p))
			return true;
		if (unselected_cells.contains(p))
			return false;
		if (selected_all)
		{
			if (unselected_rows.contains(row_idx) || unselected_cols.contains(col_idx))
				return false;
			return true;
		}
		else
		{
			return (selected_rows.contains(row_idx) || selected_cols.contains(col_idx));
		}
	}

	// returns -1 for header
	int get_col_at(int x) const
	{
		if (x < (int)header_rows_width)
			return -1;
		int result = 0;
		int total_thickness = header_rows_width;
		for (auto & thickness : thickness_cols)
		{
			total_thickness += thickness;
			if (x < total_thickness)
				break;
			++result;
		}
		return result;
	}
	// returns -1 for header
	int get_row_at(int y) const
	{
		if (y < (int)header_cols_height)
			return -1;
		int result = 0;
		int total_thickness = header_cols_height;
		for (auto & thickness : thickness_rows)
		{
			total_thickness += thickness;
			if (y < total_thickness)
				break;
			++result;
		}
		return result;
	}

	void clear_selection()
	{
		  selected_cols.clear();
		  selected_rows.clear();
		unselected_cols.clear();
		unselected_rows.clear();
		  selected_cells.clear();
		unselected_cells.clear();
		selected_all = false;
	}

	// return true if the row was unselected
	bool toggle_unselected_row(unsigned int row_idx)
	{
		bool was_unselected = 0 < unselected_rows.erase(row_idx);
		if ( ! was_unselected)
			unselected_rows.insert(row_idx);
		return was_unselected;
	}
	// return true if the row was selected
	bool toggle_selected_row(unsigned int row_idx)
	{
		bool was_selected = 0 < selected_rows.erase(row_idx);
		if ( ! was_selected)
			selected_rows.insert(row_idx);
		return was_selected;
	}
	// return true if the col was unselected
	bool toggle_unselected_col(unsigned int col_idx)
	{
		bool was_unselected = 0 < unselected_cols.erase(col_idx);
		if ( ! was_unselected)
			unselected_cols.insert(col_idx);
		return was_unselected;
	}
	// return true if the col was selected
	bool toggle_selected_col(unsigned int col_idx)
	{
		bool was_selected = 0 < selected_cols.erase(col_idx);
		if ( ! was_selected)
			selected_cols.insert(col_idx);
		return was_selected;
	}

	void clear_selected_cells_in_row(  unsigned int row_idx)
	{
		std::erase_if(selected_cells, [&](const auto & p){ return p.second == row_idx; });
	}
	void clear_selected_cells_in_col(  unsigned int col_idx)
	{
		std::erase_if(selected_cells, [&](const auto & p){ return p.first == col_idx; });
	}
	void clear_unselected_cells_in_row(unsigned int row_idx)
	{
		std::erase_if(unselected_cells, [&](const auto & p){ return p.second == row_idx && ! selected_cols.contains(p.first); });
	}
	void clear_unselected_cells_in_col(unsigned int col_idx)
	{
		std::erase_if(unselected_cells, [&](const auto & p){ return p.first == col_idx && ! selected_rows.contains(p.second); });
	}
	bool is_col_selected  (unsigned int col_idx) { return selected_cols  .contains(col_idx); }
	bool is_row_selected  (unsigned int row_idx) { return selected_rows  .contains(row_idx); }
	bool is_col_unselected(unsigned int col_idx) { return unselected_cols.contains(col_idx); }
	bool is_row_unselected(unsigned int row_idx) { return unselected_rows.contains(row_idx); }

	void toggle_selected_cell(unsigned int col_idx, unsigned int row_idx)
	{
		auto p = std::make_pair(col_idx, row_idx);
		bool col_is_selected = is_col_selected(col_idx);
		bool row_is_selected = is_row_selected(row_idx);
		bool col_is_unselected = is_col_unselected(col_idx);
		bool row_is_unselected = is_row_unselected(row_idx);

		if (selected_all)
		{
			if ( ! col_is_unselected && ! row_is_unselected)
			{
				if (0 == unselected_cells.erase(p))
				{
					unselected_cells.insert(p);
				}
			}
			else
			{
				if (0 == selected_cells.erase(p))
				{
					selected_cells.insert(p);
				}
			}
		}
		else
		{
			if (col_is_selected || row_is_selected)
			{
				if (0 == unselected_cells.erase(p))
				{
					unselected_cells.insert(p);
				}
			}
			else
			{
				if (0 == selected_cells.erase(p))
				{
					selected_cells.insert(p);
				}
			}
		}
	}

	// returns -2 if none, -1 if header, idx otherwise
	int is_mouse_on_row_header_edge(int mouse_x, int mouse_y)
	{
		if (mouse_x <= (int)header_rows_width + header_resizing_area_thickness)
		{
			// might be on a row's edge in row header
			int y = header_cols_height;
			int i = -1;
			for (int thickness : thickness_rows)
			{
				int next_y = y + thickness;
				if (   mouse_y >= y - header_resizing_area_thickness
					&& mouse_y <= y + header_resizing_area_thickness)
					return i;
				if (y >= this->rect.h)
					break;
				y = next_y;
				++i;
			}
		}
		return -2;
	}
	// returns -2 if none, -1 if header, idx otherwise
	int is_mouse_on_col_header_edge(int mouse_x, int mouse_y)
	{
		if (mouse_y < (int)header_cols_height + header_resizing_area_thickness)
		{
			// might be on a row's edge in row header
			int x = header_rows_width;
			int i = -1;
			for (int thickness : thickness_cols)
			{
				int next_x = x + thickness;
				if (   mouse_x >= x - header_resizing_area_thickness
					&& mouse_x <= x + header_resizing_area_thickness)
					return i;
				if (x >= this->rect.w)
					break;
				x = next_x;
				++i;
			}
		}
		return -2;
	}

	CellData * get_cell_at(unsigned int col_idx, unsigned int row_idx)
	{
		if (row_idx >= cell_data.size() || col_idx >= cell_data[row_idx].size())
			return nullptr;
		return &cell_data[row_idx][col_idx];
	}
	icu::UnicodeString get_formula_at(unsigned int col_idx, unsigned int row_idx)
	{
		if (row_idx >= cell_data.size() || col_idx >= cell_data[row_idx].size())
			return "";
		return cell_data[row_idx][col_idx].formula;
	}
	void set_formula_at(unsigned int col_idx, unsigned int row_idx, icu::UnicodeString text)
	{
		if (row_idx >= cell_data.size() || col_idx >= cell_data[row_idx].size())
			return;
		if (cell_data[row_idx][col_idx].set_formula(text, col_idx, row_idx))
			this->set_needs_redraw();
	}
	icu::UnicodeString get_value_at(unsigned int col_idx, unsigned int row_idx)
	{
		if (row_idx >= cell_data.size() || col_idx >= cell_data[row_idx].size())
			return "";
		return cell_data[row_idx][col_idx].display.get_text();
	}
	std::string get_type_at(unsigned int col_idx, unsigned int row_idx)
	{
		if (row_idx >= cell_data.size() || col_idx >= cell_data[row_idx].size())
			return "";
		return cell_data[row_idx][col_idx].type;
	}

	void set_active_cell(unsigned int col_idx, unsigned int row_idx)
	{
		auto p = std::make_pair(col_idx, row_idx);
		if (active_cell != p)
		{
			set_formula_at(active_cell.first, active_cell.second, editor.get_text());

			editor.set_text(get_formula_at(col_idx, row_idx));
			active_cell = p;
		}
	}

	unsigned int get_col_count() const
	{
		return thickness_cols.size();
	}
	unsigned int get_row_count() const
	{
		return thickness_rows.size();
	}

	virtual bool handle_event(event & ev) override
	{
		switch(ev.type)
		{
			case nop:
				return true;
			case window_shown:
				return false;
			case mouse:
			{
				int row_edge_idx = is_mouse_on_row_header_edge(ev.data.mouse.x, ev.data.mouse.y);
				int col_edge_idx = is_mouse_on_col_header_edge(ev.data.mouse.x, ev.data.mouse.y); 

				if (ev.data.mouse.pressed != true)
				{
					// mouse move
					if (row_edge_idx != -2)
						parent_window->set_cursor(MouseCursorImg::SIZENS);
					else if (col_edge_idx != -2)
						parent_window->set_cursor(MouseCursorImg::SIZEWE);
					else
						parent_window->set_cursor(MouseCursorImg::ARROW);
					break;
				}

				// mouse click

				int col_idx = get_col_at(ev.data.mouse.x);
				int row_idx = get_row_at(ev.data.mouse.y);

				set_active_cell(std::max(0,col_idx),std::max(0,row_idx));

				this->take_focus();

				if (row_edge_idx == -1)
				{
					unsigned int row_thickness = header_cols_height;
					parent_window->set_cursor(MouseCursorImg::SIZENS);
					this->mouse_grab(this,
						[row_edge_idx,this,row_thickness](T::Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int drag_x, [[maybe_unused]]int drag_y)
						{
							unsigned int new_thickness = (unsigned int)std::max(1, (int)row_thickness + drag_y - grab_y);
							if (this->header_cols_height != new_thickness)
							{
								this->header_cols_height = new_thickness;
								this->set_needs_redraw();
							}
						},
						[row_edge_idx,this,row_thickness](T::Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int ungrab_x, [[maybe_unused]]int ungrab_y)
						{
							this->parent_window->set_cursor(MouseCursorImg::ARROW);
							unsigned int new_thickness = (unsigned int)std::max(1, (int)row_thickness + ungrab_y - grab_y);
							if (this->header_cols_height != new_thickness)
							{
								this->header_cols_height = new_thickness;
								this->set_needs_redraw();
							}
						});
					break;
				}
				else if (row_edge_idx >= 0)
				{
					unsigned int row_thickness = this->thickness_rows[row_edge_idx];
					parent_window->set_cursor(MouseCursorImg::SIZENS);
					this->mouse_grab(this,
						[row_edge_idx,this,row_thickness](T::Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int drag_x, [[maybe_unused]]int drag_y)
						{
							unsigned int new_thickness = (unsigned int)std::max(1, (int)row_thickness + drag_y - grab_y);
							if (this->thickness_rows[row_edge_idx] != new_thickness)
							{
								this->thickness_rows[row_edge_idx] = new_thickness;
								this->set_needs_redraw();
							}
						},
						[row_edge_idx,this,row_thickness](T::Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int ungrab_x, [[maybe_unused]]int ungrab_y)
						{
							this->parent_window->set_cursor(MouseCursorImg::ARROW);
							unsigned int new_thickness = (unsigned int)std::max(1, (int)row_thickness + ungrab_y - grab_y);
							if (this->thickness_rows[row_edge_idx] != new_thickness)
							{
								this->thickness_rows[row_edge_idx] = new_thickness;
								this->set_needs_redraw();
							}
						});
					break;
				}
				else if (col_edge_idx == -1)
				{
					unsigned int col_thickness = header_rows_width;
					parent_window->set_cursor(MouseCursorImg::SIZEWE);
					this->mouse_grab(this,
						[col_edge_idx,this,col_thickness](T::Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int drag_x, [[maybe_unused]]int drag_y)
						{
							unsigned int new_thickness = (unsigned int) std::max(1, (int)col_thickness + drag_x - grab_x);
							if (this->header_rows_width != new_thickness)
							{
								this->header_rows_width = new_thickness;
								this->set_needs_redraw();
							}
						},
						[col_edge_idx,this,col_thickness](T::Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int ungrab_x, [[maybe_unused]]int ungrab_y)
						{
							unsigned int new_thickness = (unsigned int) std::max(1, (int)col_thickness + ungrab_x - grab_x);
							if (this->header_rows_width != new_thickness)
							{
								this->header_rows_width = new_thickness;
								this->set_needs_redraw();
							}
							this->parent_window->set_cursor(MouseCursorImg::ARROW);
						});
					break;
				}
				else if (col_edge_idx >= 0)
				{
					unsigned int col_thickness = this->thickness_cols[col_edge_idx];
					parent_window->set_cursor(MouseCursorImg::SIZEWE);
					this->mouse_grab(this,
						[col_edge_idx,this,col_thickness](T::Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int drag_x, [[maybe_unused]]int drag_y)
						{
							unsigned int new_thickness = (unsigned int) std::max(1, (int)col_thickness + drag_x - grab_x);
							if (this->thickness_cols[col_edge_idx] != new_thickness)
							{
								this->thickness_cols[col_edge_idx] = new_thickness;
								this->set_needs_redraw();
							}
						},
						[col_edge_idx,this,col_thickness](T::Widget*, [[maybe_unused]]int grab_x, [[maybe_unused]]int grab_y, [[maybe_unused]]int ungrab_x, [[maybe_unused]]int ungrab_y)
						{
							unsigned int new_thickness = (unsigned int) std::max(1, (int)col_thickness + ungrab_x - grab_x);
							if (this->thickness_cols[col_edge_idx] != new_thickness)
							{
								this->thickness_cols[col_edge_idx] = new_thickness;
								this->set_needs_redraw();
							}
							this->parent_window->set_cursor(MouseCursorImg::ARROW);
						});
					break;
				}

				bool changed = false;
				if ( ! key_ctrl)
				{
					clear_selection();
					if (col_idx == -1 && row_idx == -1)
					{
						changed |= ! selected_all;
						selected_all = true;
					}
					else if (col_idx == -1)
					{
						if ( ! is_row_selected(row_idx))
						{
							changed |= true;
							selected_rows.insert(row_idx);
							clear_unselected_cells_in_row(row_idx);
						}
					}
					else if (row_idx == -1)
					{
						if ( ! is_col_selected(col_idx))
						{
							changed |= true;
							selected_cols.insert(col_idx);
							clear_unselected_cells_in_col(col_idx);
						}
					}
				}
				else
				{
					// ctrl is held down

					if (col_idx == -1 && row_idx == -1)
					{
						clear_selection();
						changed |= ! selected_all;
						selected_all = true;
					}
					else if (col_idx == -1)
					{
						changed |= true;
						if (selected_all)
						{
							bool was_unselected = toggle_unselected_row((unsigned int)row_idx);
							if (was_unselected)
								clear_selected_cells_in_row(row_idx);
							else
								clear_unselected_cells_in_row(row_idx);
						}
						else
						{
							bool was_selected = toggle_selected_row((unsigned int)row_idx);
							if (was_selected)
								clear_unselected_cells_in_row(row_idx);
							else
								clear_selected_cells_in_row(row_idx);
						}
					}
					else if (row_idx == -1)
					{
						changed |= true;
						if (selected_all)
						{
							bool was_unselected = toggle_unselected_col((unsigned int)col_idx);
							if (was_unselected)
								clear_selected_cells_in_col(col_idx);
							else
								clear_unselected_cells_in_col(col_idx);
						}
						else
						{
							bool was_selected = toggle_selected_col((unsigned int)col_idx);
							if (was_selected)
								clear_unselected_cells_in_col(col_idx);
							else
								clear_selected_cells_in_col(col_idx);
						}
					}
					else
					{
						changed |= true;
						toggle_selected_cell(col_idx, row_idx);
					}
				}
				if (changed)
					this->set_needs_redraw();
				return changed;
			}
			case key:
			{
				if (ev.data.key.keycode == Scancode::Ctrl)
				{
					key_ctrl = ev.data.key.pressed;
					break;
				}
				if ( ! ev.data.key.pressed)
					break;
				bool changed = false;
				switch (ev.data.key.keycode)
				{
					case Scancode::Up:
						if (active_cell.second != 0)
							set_active_cell(active_cell.first, active_cell.second - 1);
						changed |= true;
						break;
					case Scancode::Down:
					case Scancode::Enter:
						if (active_cell.second < get_row_count()-1)
							set_active_cell(active_cell.first, active_cell.second + 1);
						changed |= true;
						break;
					case Scancode::Left:
						if (active_cell.first != 0)
							set_active_cell(active_cell.first - 1, active_cell.second);
						changed |= true;
						break;
					case Scancode::Right:
						if (active_cell.first < get_col_count()-1)
							set_active_cell(active_cell.first + 1, active_cell.second);
						changed |= true;
						break;
					default:
						editor.handle_event(ev);
				}
				if (changed)
					this->set_needs_redraw();
				return changed;
			}
			case text:
				editor.handle_event(ev);
				break;
			default:
				break;
		}
		return false;
	}
};

inline static Grid<OW<SDL>> * global_grid;

/*
PYBIND11_EMBEDDED_MODULE(ourcalc_cell, m) {
	// `m` is a `py::module_` which is used to bind functions and classes
	m.def("get_cell_value", [](int c, int r) {
		std::string tmp;
		global_grid->get_value_at(c, r).toUTF8String(tmp);
		return tmp;
	});
	m.def("get_cell_type", [](int c, int r) {
		return global_grid->get_type_at(c, r);
	});
}
*/

unsigned int parse_col_name(std::string col_name)
{
	if (col_name.size() == 0)
		throw std::exception();
	unsigned int result = col_name[0] - 'A';
	for(unsigned int i=1 ; i<col_name.size() ; ++i)
	{
		result = (result + 1)*26;
		result += col_name[i] - 'A';
	}
	return result;
}

std::tuple<unsigned int, unsigned int> parse_cell_name(const std::string & cell_name)
{
	unsigned int col, row;

	auto i = 0;
	while (cell_name[i] >= 'A' && cell_name[i] <= 'Z')
		++i;
	col = parse_col_name(cell_name.substr(0, i));
	row = std::stoi(cell_name.substr(i));
	return {col, row};
}

std::string column_name_from_int(int c)
{
	int remainder = c % 26;
	c /= 26;
	std::string result(1, 'A'+remainder);
	while (c != 0)
	{
		--c;
		remainder = c % 26;
		c /= 26;
		result.push_back('A'+remainder);
	}
	std::reverse(result.begin(), result.end());
	return result;
}

icu::UnicodeString get_formula_python_code(icu::UnicodeString & formula, int col, int row)
{
	icu::UnicodeString code = R"(
result = _formula_
_colname__row_ = make_ourcell(result, _col_, _row_)

ourcalc_display_text = str(result)
ourcalc_display_type = result.__class__.__name__

import ast
code = '_formula_'
root = ast.parse(code)
root = ast.walk(root)
n = ast.Name
var_set = set()
for node in root:
	if isinstance(node,n) and node.id in locals() and is_ourcell(locals()[str(node.id)]):
		var_set.add(node.id)
ourcalc_variables = ','.join(var_set)
)";

	code.findAndReplace("_col_", icu::UnicodeString::fromUTF8(std::to_string(col)));
	code.findAndReplace("_row_", icu::UnicodeString::fromUTF8(std::to_string(row)));
	code.findAndReplace("_colname_"  , icu::UnicodeString::fromUTF8(column_name_from_int(col)));	
	code.findAndReplace("_formula_"  , formula.tempSubString(1));
	return code;
}
icu::UnicodeString get_string_python_code(icu::UnicodeString & formula, int col, int row)
{
	icu::UnicodeString code = R"(
result = '''_formula_'''
_colname__row_ = make_ourcell(result, _col_, _row_)

ourcalc_display_text = str(result)
ourcalc_display_type = result.__class__.__name__
ourcalc_variables = ""
)";

	code.findAndReplace("_col_", icu::UnicodeString::fromUTF8(std::to_string(col)));
	code.findAndReplace("_row_", icu::UnicodeString::fromUTF8(std::to_string(row)));
	code.findAndReplace("_colname_"  , icu::UnicodeString::fromUTF8(column_name_from_int(col)));	
	code.findAndReplace("_formula_"  , formula);
	return code;
}

std::vector<std::string> split(const std::string& s, const std::string& delimiter)
{
	size_t last = 0;
	size_t next = 0;
	std::vector<std::string> result;
	while ((next = s.find(delimiter, last)) != std::string::npos)
	{
		result.push_back(s.substr(last, next-last));
		last = next + 1;
	}
	if (last != s.size())
		result.push_back(s.substr(last));
	return result;
}

bool CellData::reevaluate(int col, int row)
{
	bool display_changed;
	auto & locals  = global_grid->locals;
	auto & globals = global_grid->globals;
	bool there_was_en_error = error;

	if (formula.length() == 0 ||  formula[0] != '=')
	{
		auto code = get_string_python_code(formula, col, row);
		std::string utf8_code;
		code.toUTF8String(utf8_code);
		//std::cout << utf8_code << std::endl;
		try
		{
			py::exec(utf8_code, globals, locals);
			auto display_text = locals["ourcalc_display_text"].cast<std::string>();
			type              = locals["ourcalc_display_type"].cast<std::string>();
			display_changed = display.set_text(display_text);

			//std::cout << "Display text: " << display_text << std::endl;
			error = false;
		}
		catch(std::exception & e)
		{
			std::cout << e.what() << std::endl;
			error = true;
			error_msg = e.what();
			display_changed = !there_was_en_error;
		}
		catch(...)
		{
			error = true;
			error_msg = "Unknown exception while evaluating expression.";
			display_changed = !there_was_en_error;
		}
		return display_changed;
	}

	//std::cout << formula << std::endl;
	//std::cout << formula.substr(1) << std::endl;
	//std::cout << code << std::endl;
	auto code = get_formula_python_code(formula, col, row);
	std::string utf8_code;
	code.toUTF8String(utf8_code);
	//std::cout << utf8_code << std::endl;
	try
	{
		py::exec(utf8_code, globals, locals);
		auto calculated_text = locals["ourcalc_display_text"].cast<std::string>();
		auto calculated_type = locals["ourcalc_display_type"].cast<std::string>();
		std::vector<std::string> variables = split(locals["ourcalc_variables"].cast<std::string>(), ",");


		error = false;
		// Check for dependency cells that contain error
		for (const std::string & v : variables)
		{
			auto [ref_col, ref_row] = parse_cell_name(v);
			auto * cell = global_grid->get_cell_at(ref_col, ref_row);
			if (cell->error)
			{
				error = true;
				error_msg = v + std::string(" has an error.");
				//return true;
			}
		}

		//std::cout << "Display text: " << display_text << std::endl;

		//std::cout << variables.size() << " variables in code: ";
		//for (auto & v : variables)
		//	std::cout << v;
		//std::cout << std::endl;

		// add dependecies cells
		for (const std::string & v : variables)
		{
			auto [ref_col, ref_row] = parse_cell_name(v);
			auto * cell = global_grid->get_cell_at(ref_col, ref_row);
			if ( ! cell)
				continue;
			dependencies.insert(std::make_pair(ref_col, ref_row));
			cell->add_dependent(col, row);
		}

		// check for circular dependencies
		if (this->do_dependencies_depend_on_us(dependencies, col, row))
		{
			error = true;
			error_msg = "Circula dependency";
		}

		if ( ! error)
		{
			type = calculated_type;
			display_changed = there_was_en_error;
			display_changed |= display.set_text(calculated_text);

			// update dependent cells
			for (const auto & p : dependent_cells)
			{
				auto * cell = global_grid->get_cell_at(p.first, p.second);
				cell->reevaluate(p.first, p.second);
			}
		}
	}
	catch(std::exception & e)
	{
		std::cout << e.what() << std::endl;
		error = true;
		error_msg = e.what();
		display_changed = !there_was_en_error;
	}
	catch(...)
	{
		error = true;
		error_msg = "Unknown exception while evaluating expression.";
		display_changed = !there_was_en_error;
	}
	return display_changed;
}

void CellData::clear_dependencies(unsigned int col, unsigned int row)
{
	for (auto & p : dependencies)
	{
		auto * cell = global_grid->get_cell_at(p.first, p.second);
		if ( ! cell)
			continue;
		cell->remove_dependent(col, row);
	}
	dependencies.clear();
}

bool CellData::do_dependencies_depend_on_us(decltype(dependencies) deps, unsigned int col, unsigned int row)
{
	auto this_pair = std::make_pair(col, row);
	for (const auto & p : deps)
		if (p == this_pair)
			return true;
	deps.insert(this_pair);
	for (const auto & p : dependent_cells)
	{
		auto * cell = global_grid->get_cell_at(p.first, p.second);
		if ( ! cell)
			continue;
		if (cell->do_dependencies_depend_on_us(deps, p.first, p.second))
			return true;
	}
	return false;
}