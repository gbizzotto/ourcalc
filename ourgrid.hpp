
#pragma once


#include "pybind11/pybind11.h"
#include "pybind11/embed.h"

#include <iostream>
#include <vector>
#include <set>
#include <map>
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

std::string        get_cell_name_string(unsigned col_idx, unsigned row_idx);
icu::UnicodeString get_cell_name_utf8  (unsigned col_idx, unsigned row_idx);


struct CellCoords
{
	unsigned int x;
	unsigned int y;
};
bool operator==(const CellCoords & left, const CellCoords & right)
{
	return left.x == right.x && left.y == right.y;
}
bool operator!=(const CellCoords & left, const CellCoords & right)
{
	return left.x != right.x || left.y != right.y;
}
bool operator<(const CellCoords & left, const CellCoords & right)
{
	return left.x < right.x || (left.x == right.x && left.y < right.y);
}

struct CellRect
{
	CellCoords upleft;
	CellCoords downright;

	CellRect(CellCoords a, CellCoords b)
	{
		if (a.x < b.x)
		{
			upleft.x = a.x;
			downright.x = b.x;
		}
		else
		{
			upleft.x = b.x;
			downright.x = a.x;
		}
		if (a.y < b.y)
		{
			upleft.y = a.y;
			downright.y = b.y;
		}
		else
		{
			upleft.y = b.y;
			downright.y = a.y;
		}
	}

	bool contains(const CellCoords & r) const
	{
		return true
		    && r.x >= upleft.x
		    && r.x <= downright.x
		    && r.y >= upleft.y
		    && r.y <= downright.y
		    ;
	}
};
bool operator==(const CellRect & a, const CellRect & b)
{
	return a.upleft == b.upleft && a.downright == b.downright;
}
bool operator<(const CellRect & a, const CellRect & b)
{
	return a.upleft < b.upleft || (a.upleft == b.upleft && a.downright < b.downright);
}

struct SelRect : CellRect
{
	// is it selected or UNselected?
	bool is_positive;

	SelRect(const CellRect & r, bool b)
		: CellRect(r)
		, is_positive(b)
	{}
};

class SelRects : public std::vector<SelRect>
{
public:
	using vtype = std::vector<SelRect>;

	auto find(CellRect r)
	{
		return std::find(this->begin(), this->end(), r);
	}
	auto push_front(SelRect && r)
	{
		return this->insert(this->begin(), r);
	}
};

struct CellData
{
	icu::UnicodeString formula;
	std::string type;
	Text display;
	bool error = false;
	horizontal_policy h_policy = horizontal_policy{horizontal_policy::alignment_t::none, horizontal_policy::sizing_t::none};
	std::string error_msg = std::string("");
	std::set<CellCoords> dependencies = std::set<CellCoords>();
	std::vector<CellCoords> dependent_cells = {};

	bool do_dependencies_depend_on_us(decltype(dependencies) deps, unsigned int col, unsigned int row);
	void clear_dependencies(unsigned int col, unsigned int row);
	bool reevaluate(int col, int row);
	bool set_formula(icu::UnicodeString contents, int col, int row);
	void add_dependent(unsigned int col, unsigned int row)
	{
		auto p = CellCoords{col, row};
		auto it = std::lower_bound(std::begin(dependent_cells), std::end(dependent_cells), p);
		if (it == std::end(dependent_cells) || *it != p)
			dependent_cells.insert(it, p);
	}
	void remove_dependent(unsigned int col, unsigned int row)
	{
		auto p = CellCoords{col, row};
		auto it = std::lower_bound(std::begin(dependent_cells), std::end(dependent_cells), p);
		if (it != std::end(dependent_cells) && *it == p)
			dependent_cells.erase(it);
	}
	horizontal_policy::alignment_t get_horizontal_alignment() const;
};

template<typename T>
struct Grid : T::Widget
{
	inline static const color_t color_cell_bg                 = color_t(255);
	inline static const color_t color_text_header             = color_t(32);
	inline static const color_t color_lines                   = color_t(160);
	inline static const color_t color_bg_header               = color_t(210);
	inline static const color_t   selected_cells_overlay      = color_t(128,200,128,96);
	inline static const color_t unselected_cells_overlay      = color_t(200,200, 64,96);
	inline static const color_t color_active_cell             = color_t(0);
	inline static const color_t color_edit_mode_selected_cell = color_t(64,192,64,255);
	inline static const int header_resizing_area_thickness = 2;

	struct selection_t
	{
		std::set<unsigned int >    selected_cols ;
		std::set<unsigned int >    selected_rows ;
		std::set<unsigned int >  unselected_cols ;
		std::set<unsigned int >  unselected_rows ;
		std::set<CellCoords   >    selected_cells;
		std::set<CellCoords   >  unselected_cells;
		SelRects                            rects;
		bool selected_all = false;
		bool cut; // copied if false
		CellCoords reference_cell; // active_cell at the moment of copying

		void clear()
		{
			  selected_cols .clear();
			  selected_rows .clear();
			unselected_cols .clear();
			unselected_rows .clear();
			  selected_cells.clear();
			unselected_cells.clear();
			           rects.clear();
			selected_all = false;
			cut = false;
		}
		bool empty() const
		{
			return !selected_all
			    && selected_cols.empty()
			    && selected_rows.empty()
			    &&         rects.empty()
			    ;
		}

		bool is_trivial() const
		{
			if (selected_cells.size() == 1
				&& selected_cols.empty()
				&& selected_rows.empty()
				&&         rects.empty())
				return true;

			if (rects.size() == 1
				&&   selected_cells.empty()
				&&   selected_cols .empty()
				&&   selected_rows .empty())
				return true;
		}

		void add_rect(CellCoords old_coords, CellCoords new_coords)
		{
			CellRect r(old_coords, new_coords);
			auto it = rects.find(r);
			if (it != rects.end())
			{	
				if ( ! it->is_positive)
					rects.erase(it);
			}
			else
				rects.push_front({r, true});
		}
		void sub_rect(CellCoords old_coords, CellCoords new_coords)
		{
			CellRect r(old_coords, new_coords);
			auto it = rects.find(r);
			if (it != rects.end())
			{
				if (it->is_positive)
					rects.erase(it);
			}
			else
				rects.push_front({r, false});
		}

		bool does_col_have_selection(unsigned int idx) const
		{
			if (selected_all && ! unselected_cols.contains(idx))
				return true;
			for (auto n : selected_cols)
				if (idx == n)
					return true;
			for (auto p : selected_cells)
				if (idx == p.x)
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
				if (idx == p.y)
					return true;
			return false;
		}

		bool is_cell_selected(unsigned int col_idx, unsigned int row_idx) const
		{
			auto p = CellCoords{col_idx, row_idx};
			if (selected_cells.contains(p))
				return true;
			if (unselected_cells.contains(p))
				return false;
			for (const auto & rect : rects)
				if (rect.contains(p))
					return rect.is_positive;
			if (selected_all)
			{
				if (unselected_rows.contains(row_idx) || unselected_cols.contains(col_idx))
					return false;
				return true;
			}
			return (selected_rows.contains(row_idx) || selected_cols.contains(col_idx));
		}
		bool is_cell_unselected(unsigned int col_idx, unsigned int row_idx) const
		{
			auto p = CellCoords{col_idx, row_idx};
			if (unselected_cells.contains(p))
				return true;
			if (selected_cells.contains(p))
				return false;
			for (const auto & rect : rects)
				if (rect.contains(p))
					return rect.is_positive == false;
			if (selected_all)
			{
				if (selected_rows.contains(row_idx) || selected_cols.contains(col_idx))
					return true;
				return false;
			}
			return (unselected_rows.contains(row_idx) || unselected_cols.contains(col_idx));
		}

		/*
		bool is_cell_inside_selection(unsigned int col_idx, unsigned int row_idx) const
		{
			// check cols and rows
			for (auto n : selected_cols)
				if (col_idx == n)
					return true;
			for (auto n : selected_rows)
				if (row_idx == n)
					return true;

			auto p = CellCoords{col_idx, row_idx};

			// check rects
			for (auto & r : selected_rects)
				if (r.contains(p))
					return true;

			return false;
		}
		*/

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
			std::erase_if(selected_cells, [&](const auto & p){ return p.y == row_idx; });
		}
		void clear_selected_cells_in_col(  unsigned int col_idx)
		{
			std::erase_if(selected_cells, [&](const auto & p){ return p.x == col_idx; });
		}
		void clear_unselected_cells_in_row(unsigned int row_idx)
		{
			std::erase_if(unselected_cells, [&](const auto & p){ return p.y == row_idx && ! selected_cols.contains(p.x); });
		}
		void clear_unselected_cells_in_col(unsigned int col_idx)
		{
			std::erase_if(unselected_cells, [&](const auto & p){ return p.x == col_idx && ! selected_rows.contains(p.y); });
		}
		bool is_col_selected  (unsigned int col_idx) { return selected_cols  .contains(col_idx); }
		bool is_row_selected  (unsigned int row_idx) { return selected_rows  .contains(row_idx); }
		bool is_col_unselected(unsigned int col_idx) { return unselected_cols.contains(col_idx); }
		bool is_row_unselected(unsigned int row_idx) { return unselected_rows.contains(row_idx); }

		void toggle_selected_cell(unsigned int col_idx, unsigned int row_idx)
		{
			auto p = CellCoords{col_idx, row_idx};

			// search individual cells
			if (selected_cells.erase(p))
				return;
			if (unselected_cells.erase(p))
				return;

			// search rects
			for (auto & r : rects)
				if (r.contains(p))
				{
					if (r.is_positive)
						unselected_cells.insert(p);
					else
						selected_cells.insert(p);
					return;
				}

			// search cols and rows
			if (is_col_selected(col_idx) || is_row_selected(row_idx))
			{
				unselected_cells.insert(p);
				return;
			}
			if (is_col_unselected(col_idx) || is_row_unselected(row_idx))
			{
				selected_cells.insert(p);
				return;
			}

			// nothing particular
			selected_cells.insert(p);
		}
	};

	// basics
	Window * parent_window;
	T::TextEdit & editor;

	// cells
	std::vector<std::vector<CellData>> cell_data;
	const Text error_display;

	// headers
	unsigned int header_cols_height = 18;
	unsigned int header_rows_width = 40;
	std::vector<unsigned int> thickness_cols;
	std::vector<unsigned int> thickness_rows;
	std::deque<Text> header_captions_cols;
	std::deque<Text> header_captions_rows;

	// selection stuff
	selection_t selection;
	selection_t copied_or_cut;

	// grid status
	bool key_ctrl = false;
	bool key_shift = false;
	bool edit_mode = false;
	bool edit_mode_select_cell = false;
	int insert_or_replace_cell_name_idx = -1;
	int insert_or_replace_cell_name_len = 0;
	CellCoords active_cell;
	CellCoords edit_mode_selected_cell;

	// python interpreter
	py::scoped_interpreter guard;
	py::dict globals;
	py::dict locals;

	Grid(T::Window * window, T::TextEdit & edit)
		: T::Widget(window, {0,0,200,200})
		, parent_window(window)
		, editor(edit)
		, error_display(std::string("Error"), window, 255,0,0,255)
		, active_cell{std::numeric_limits<unsigned int>::max(),std::numeric_limits<unsigned int>::max()}
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
			std::cout << e.what() << " " << __FILE__ << ": " << __LINE__ << std::endl;
		}
		catch(...)
		{
			std::cout << "Unknown exception" << " " << __FILE__ << ": " << __LINE__ << std::endl;
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
		assert(row_count >= selection.selected_rows.size());

		// column count
		for (auto & row : cell_data)
			assert(row.size() == col_count);
		assert(col_count == thickness_cols.size());
		assert(col_count >= selection.selected_cols.size());

		// selection
		assert (selection.selected_cells.size() <= col_count * row_count);

		return true;
	}

	std::string get_cell_defining_code(unsigned int col_idx, unsigned int row_idx) const
	{
		const auto cell_name = get_cell_name_string(col_idx,row_idx);
		const auto col_str = column_name_from_int(col_idx);
		const auto col_int = std::to_string(col_idx);
		const auto row_str = std::to_string(row_idx);
		return std::string(cell_name)
			.append("=make_ourcell(None,")
			.append(col_int).append(",").append(row_str)
			.append(",False,False)\n")

			.append("_").append(cell_name)
			.append("=make_ourcell(").append(cell_name)
			.append(",").append(col_int).append(",").append(row_str)
			.append(",True,False)\n")

			.append(col_str).append("_").append(row_str)
			.append("=make_ourcell(").append(cell_name)
			.append(",").append(col_int).append(",").append(row_str)
			.append(",False,True)\n")

			.append("_").append(col_str).append("_").append(row_str)
			.append("=make_ourcell(").append(cell_name)
			.append(",").append(col_int).append(",").append(row_str)
			.append(",True,True)\n")
			;
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
				code.append(get_cell_defining_code(col_number, row_number));
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
			for (unsigned int row_number=before_idx,i=0 ; i<count ; ++i,++row_number)
				code.append(get_cell_defining_code(col_number, row_number));

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
			bool col_has_selected_cells = selection.does_col_have_selection(i);
			if (col_has_selected_cells || (!col_has_selected_cells && active_cell.x == i))
				this->drawable_area.fill_rect(x, 0, thickness-1, header_cols_height-1, selected_cells_overlay.r, selected_cells_overlay.g, selected_cells_overlay.b, selected_cells_overlay.a);

			this->drawable_area.copy_from_text_to_rect_center(header_captions_cols[i], x, 0, thickness, header_cols_height);

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
			bool row_has_selected_cells = selection.does_row_have_selection(i);
			if (row_has_selected_cells || (!row_has_selected_cells && active_cell.y == i))
				this->drawable_area.fill_rect(0, y, header_rows_width-1, thickness-1, selected_cells_overlay.r, selected_cells_overlay.g, selected_cells_overlay.b, selected_cells_overlay.a);

			this->drawable_area.copy_from_text_to_rect_center(header_captions_rows[i], 0, y, header_rows_width, thickness);

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
						this->drawable_area.copy_from_text_to_rect_center(error_display, x, y, thickness_col-1, thickness_row-1);
					else if (cell_data[row_idx][col_idx].get_horizontal_alignment() == horizontal_policy::alignment_t::center)
						this->drawable_area.copy_from_text_to_rect_center(cell_data[row_idx][col_idx].display, x, y, thickness_col-1, thickness_row-1);
					else if (cell_data[row_idx][col_idx].get_horizontal_alignment() == horizontal_policy::alignment_t::left)
						this->drawable_area.copy_from_text_to_rect_left(cell_data[row_idx][col_idx].display, x, y, thickness_col-1, thickness_row-1);
					else if (cell_data[row_idx][col_idx].get_horizontal_alignment() == horizontal_policy::alignment_t::right)
						this->drawable_area.copy_from_text_to_rect_right(cell_data[row_idx][col_idx].display, x, y, thickness_col-1, thickness_row-1);
				}

				if (active_cell.x == col_idx && active_cell.y == row_idx)
				{
					this->drawable_area.draw_rect(x, y, thickness_col-1, thickness_row-1, color_active_cell.r, color_active_cell.g, color_active_cell.b, color_active_cell.a);
					//this->drawable_area.draw_rect(x-1, y-1, thickness_col+1, thickness_row+1, color_active_cell.r, color_active_cell.g, color_active_cell.b, color_active_cell.a);
				}
				else if (edit_mode && edit_mode_select_cell && edit_mode_selected_cell.x == col_idx && edit_mode_selected_cell.y == row_idx)
				{
					this->drawable_area.draw_rect(x, y, thickness_col-1, thickness_row-1, color_active_cell.r, color_edit_mode_selected_cell.g, color_edit_mode_selected_cell.b, color_edit_mode_selected_cell.a);
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
		if (selection.is_cell_selected(col_idx, row_idx))
			return selected_cells_overlay;
		else if (selection.is_cell_unselected(col_idx, row_idx))
			return unselected_cells_overlay;
		else
			return color_cell_bg;
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

	bool type_is_text(const std::string & type)
	{
		return type == "str";
	}
	bool type_is_number(const std::string & type)
	{
		return false
			|| type == "int"
			|| type == "float"
			|| type == "complex"
			;
	}
	bool type_is_date(const std::string & type)
	{
		return false
			;
	}
	bool type_is_bool(const std::string & type)
	{
		return type == "bool";
	}
	bool type_is_list(const std::string & type)
	{
		return false
			|| type == "list"
			|| type == "tuple"
			|| type == "set"
			|| type == "dict"
			|| type == "range"
			|| type == "frozenset"
			;
	}
	bool type_is_binary(const std::string & type)
	{
		return false
			|| type == "bytes"
			|| type == "bytearray"
			|| type == "memoryview"
			;
	}
	horizontal_policy::alignment_t get_horizontal_alignment(const std::string & type)
	{
		// TODO: make it a configuration file
		if (type_is_text(type))
			return horizontal_policy::alignment_t::left;
		if (type_is_number(type))
			return horizontal_policy::alignment_t::right;
		if (type_is_date(type))
			return horizontal_policy::alignment_t::center;
		if (type_is_bool(type))
			return horizontal_policy::alignment_t::center;
		if (type_is_binary(type))
			return horizontal_policy::alignment_t::left;
		return horizontal_policy::alignment_t::center;
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
		set_formula_at(active_cell.x, active_cell.y, editor.get_text());

		auto p = CellCoords{col_idx, row_idx};
		if (active_cell != p)
		{
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

	void insert_or_replace_cell_name(unsigned col_idx, unsigned row_idx)
	{
		// TODO: use selection instead?
		auto cell_name = get_cell_name_utf8(col_idx, row_idx);
		if (insert_or_replace_cell_name_idx == -1)
		{
			// insert
			insert_or_replace_cell_name_idx = editor.get_text_length();
			insert_or_replace_cell_name_len = cell_name.length();
			editor.insert(cell_name);
		}
		else
		{
			assert(editor.get_text_length() >= insert_or_replace_cell_name_idx);
			editor.replace(insert_or_replace_cell_name_idx, insert_or_replace_cell_name_len, cell_name);
			insert_or_replace_cell_name_len = cell_name.length();
		}
	}

	icu::UnicodeString translate_formula(const icu::UnicodeString & formula, unsigned int offset_x, unsigned int offset_y)
	{
		return formula;
	}

	void paste(const selection_t & sel, unsigned int new_x, unsigned int new_y)
	{
		int offset_x = new_x - sel.reference_cell.x;
		int offset_y = new_y - sel.reference_cell.y;

		for (const auto & p : sel.selected_cells)
		{
			CellData * cell_data = get_cell_at(p.x, p.y);
			assert(cell_data);
			if ( ! cell_data)
				continue;
			if (p.x+offset_x < 0)
				// TODO: error message
				continue;
			if (p.y+offset_y < 0)
				// TODO: error message
				continue;
			auto new_formula = translate_formula(cell_data->formula, offset_x, offset_y);
			set_formula_at(p.x+offset_x, p.y+offset_y, new_formula);
			if (p == sel.reference_cell)
				editor.set_text(new_formula);
		}
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

				if (edit_mode)
				{
					edit_mode_select_cell = true;
					edit_mode_selected_cell.x = col_idx;
					edit_mode_selected_cell.y = row_idx;
					insert_or_replace_cell_name(col_idx, row_idx);
					this->set_needs_redraw();
					break;
				}

				auto old_active_cell = active_cell;
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
					// if ctrl, keep previous selection, wipe otherwise
					selection.clear();
				}
				if (key_shift)
				{
					if (selection.empty() || selection.is_cell_selected(old_active_cell.x, old_active_cell.y))
						selection.add_rect(old_active_cell, active_cell);
					else
						selection.sub_rect(old_active_cell, active_cell);
					active_cell = old_active_cell;
				}
				else if (key_ctrl)
				{
					// ctrl is held down

					if (col_idx == -1 && row_idx == -1)
					{
						selection.clear();
						changed |= ! selection.selected_all;
						selection.selected_all = true;
					}
					else if (col_idx == -1)
					{
						changed |= true;
						if (selection.selected_all)
						{
							bool was_unselected = selection.toggle_unselected_row((unsigned int)row_idx);
							if (was_unselected)
								selection.clear_selected_cells_in_row(row_idx);
							else
								selection.clear_unselected_cells_in_row(row_idx);
						}
						else
						{
							bool was_selected = selection.toggle_selected_row((unsigned int)row_idx);
							if (was_selected)
								selection.clear_unselected_cells_in_row(row_idx);
							else
								selection.clear_selected_cells_in_row(row_idx);
						}
					}
					else if (row_idx == -1)
					{
						changed |= true;
						if (selection.selected_all)
						{
							bool was_unselected = selection.toggle_unselected_col((unsigned int)col_idx);
							if (was_unselected)
								selection.clear_selected_cells_in_col(col_idx);
							else
								selection.clear_unselected_cells_in_col(col_idx);
						}
						else
						{
							bool was_selected = selection.toggle_selected_col((unsigned int)col_idx);
							if (was_selected)
								selection.clear_unselected_cells_in_col(col_idx);
							else
								selection.clear_selected_cells_in_col(col_idx);
						}
					}
					else
					{
						if (active_cell != old_active_cell && selection.empty())
						{
							selection.toggle_selected_cell(old_active_cell.x, old_active_cell.y);
						}
						changed |= true;
						selection.toggle_selected_cell(col_idx, row_idx);
					}
				}
				else
				{
					if (col_idx == -1 && row_idx == -1)
					{
						changed |= ! selection.selected_all;
						selection.selected_all = true;
					}
					else if (col_idx == -1)
					{
						if ( ! selection.is_row_selected(row_idx))
						{
							changed |= true;
							selection.selected_rows.insert(row_idx);
							selection.clear_unselected_cells_in_row(row_idx);
						}
					}
					else if (row_idx == -1)
					{
						if ( ! selection.is_col_selected(col_idx))
						{
							changed |= true;
							selection.selected_cols.insert(col_idx);
							selection.clear_unselected_cells_in_col(col_idx);
						}
					}
				}
				if (changed)
					this->set_needs_redraw();
				return changed;
			}
			case key:
			{
				if (ev.data.key.mod & key_data::Mods::LCTRL)
				{
					switch(ev.data.key.charcode)
					{
						case 'c':
						case 'x':
							copied_or_cut = selection;
							copied_or_cut.reference_cell = active_cell;
							copied_or_cut.cut = ev.data.key.charcode == 'x';
							if (copied_or_cut.empty())
								copied_or_cut.toggle_selected_cell(active_cell.x, active_cell.y);
							break;
						case 'v':
							std::cout << "paste" << std::endl;
							paste(copied_or_cut, active_cell.x, active_cell.y);
							break;
						default:
							std::cout << "CTRL-" << ev.data.key.charcode << std::endl;
							break;
					}
				}

				if (ev.data.key.keycode == Scancode::Ctrl)
				{
					key_ctrl = ev.data.key.pressed;
					break;
				}
				else if (ev.data.key.keycode == Scancode::Shift)
				{
					key_shift = ev.data.key.pressed;
					break;
				}
				if ( ! ev.data.key.pressed)
					break;
				bool changed = false;
				switch (ev.data.key.keycode)
				{
					case Scancode::Up:
						if (edit_mode)
						{
							changed |= !edit_mode_select_cell;
							edit_mode_select_cell = true;
							if (edit_mode_selected_cell.y != 0)
							{
								--edit_mode_selected_cell.y;
								changed |= true;
							}
							insert_or_replace_cell_name(edit_mode_selected_cell.x, edit_mode_selected_cell.y);
						}
						else
						{
							if (active_cell.y != 0)
								set_active_cell(active_cell.x, active_cell.y - 1);
							changed |= true;
						}
						break;
					case Scancode::Down:
						if (edit_mode)
						{
							changed |= !edit_mode_select_cell;
							edit_mode_select_cell = true;
							if (edit_mode_selected_cell.y < get_row_count()-1)
							{
								++edit_mode_selected_cell.y;
								changed |= true;
							}
							insert_or_replace_cell_name(edit_mode_selected_cell.x, edit_mode_selected_cell.y);
						}
						else
						{
							if (active_cell.y < get_row_count()-1)
							{
								set_active_cell(active_cell.x, active_cell.y + 1);
								changed |= true;
							}
						}
						break;
					case Scancode::Left:
						if (edit_mode)
						{
							changed |= !edit_mode_select_cell;
							edit_mode_select_cell = true;
							if (edit_mode_selected_cell.x > 0)
							{
								--edit_mode_selected_cell.x;
								changed |= true;
							}
							insert_or_replace_cell_name(edit_mode_selected_cell.x, edit_mode_selected_cell.y);
						}
						else
						{
							if (active_cell.x != 0)
								set_active_cell(active_cell.x - 1, active_cell.y);
							changed |= true;
						}
						break;
					case Scancode::Right:
						if (edit_mode)
						{
							changed |= !edit_mode_select_cell;
							edit_mode_select_cell = true;
							if (edit_mode_selected_cell.x < get_row_count()-1)
							{
								++edit_mode_selected_cell.x;
								changed |= true;
							}
							insert_or_replace_cell_name(edit_mode_selected_cell.x, edit_mode_selected_cell.y);
						}
						else
						{
							if (active_cell.x < get_col_count()-1)
								set_active_cell(active_cell.x + 1, active_cell.y);
							changed |= true;
						}
						break;
					case Scancode::Enter:
						if (edit_mode)
						{
							if (edit_mode_selected_cell != active_cell)
							{
								//editor.insert(get_cell_name(edit_mode_selected_cell.x, edit_mode_selected_cell.y));
								edit_mode_selected_cell = active_cell;
								insert_or_replace_cell_name_idx = -1;
								insert_or_replace_cell_name_len = -1;
								changed |= true;
							}
							else
							{
								set_active_cell(active_cell.x, active_cell.y + 1);
								changed |= true;
								edit_mode_select_cell = false;
								edit_mode = false;
							}
						}
						else
						{
							if (active_cell.y < get_row_count()-1)
							{
								set_active_cell(active_cell.x, active_cell.y + 1);
								changed |= true;
							}
						}
						break;
					case Scancode::F2:						
						edit_mode = false;
						editor.take_focus();
						break;
					case Scancode::Delete:
						if (edit_mode)
						{
							if (edit_mode_select_cell)
							{
								edit_mode_select_cell = false;
								edit_mode_selected_cell = active_cell;
								insert_or_replace_cell_name_idx = -1;
								insert_or_replace_cell_name_len = -1;
								changed |= true;
							}
							editor.handle_event(ev);
						}
						else
						{
							editor.clear();
							set_formula_at(active_cell.x, active_cell.y, "");
							changed |= true;
						}
						break;
					case Scancode::Backspace:
						if (edit_mode)
						{
							if (edit_mode_select_cell)
							{
								edit_mode_select_cell = false;
								edit_mode_selected_cell = active_cell;
								insert_or_replace_cell_name_idx = -1;
								insert_or_replace_cell_name_len = -1;
								changed |= true;
							}
							editor.handle_event(ev);
						}
						else
						{
							editor.clear();
							edit_mode = true;
							edit_mode_selected_cell = active_cell;
							changed |= true;
						}
						break;
					default:
						editor.handle_event(ev);
				}
				if (changed)
					this->set_needs_redraw();
				return changed;
			}
			case text:
				if ( ! edit_mode)
				{
					editor.clear();
					edit_mode = true;
					edit_mode_selected_cell = active_cell;
				}
				else
				{
					if (edit_mode_select_cell)
					{
						edit_mode_select_cell = false;
						edit_mode_selected_cell = active_cell;
						insert_or_replace_cell_name_idx = -1;
						insert_or_replace_cell_name_len = -1;
						this->set_needs_redraw();
					}
				}
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

	int i = 0;
	if (cell_name[i] == '_')
		i = 1;
	int start = i;
	while (cell_name[i] >= 'A' && cell_name[i] <= 'Z')
		++i;
	col = parse_col_name(cell_name.substr(start, i));
	if (cell_name[i] == '_')
		++i;
	row = std::stoi(cell_name.substr(i));
	return {col, row};
}

std::string get_cell_name_string(unsigned col_idx, unsigned row_idx)
{
	return column_name_from_int(col_idx).append(std::to_string(row_idx));
}
icu::UnicodeString get_cell_name_utf8(unsigned col_idx, unsigned row_idx)
{
	return icu::UnicodeString::fromUTF8(get_cell_name_string(col_idx, row_idx));
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
_colname__row_.set_val(result)

ourcalc_display_text = str(result)
ourcalc_display_type = result.get_final_val().__class__.__name__ if is_ourcell(result) else result.__class__.__name__
)";

	code.findAndReplace("_col_", icu::UnicodeString::fromUTF8(std::to_string(col)));
	code.findAndReplace("_row_", icu::UnicodeString::fromUTF8(std::to_string(row)));
	code.findAndReplace("_colname_"  , icu::UnicodeString::fromUTF8(column_name_from_int(col)));	
	code.findAndReplace("_formula_"  , formula.tempSubString(1));
	return code;
}
icu::UnicodeString get_parse_python_code(icu::UnicodeString & formula)
{
	icu::UnicodeString code = R"(
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

	//code.findAndReplace("_col_", icu::UnicodeString::fromUTF8(std::to_string(col)));
	//code.findAndReplace("_row_", icu::UnicodeString::fromUTF8(std::to_string(row)));
	//code.findAndReplace("_colname_"  , icu::UnicodeString::fromUTF8(column_name_from_int(col)));	
	code.findAndReplace("_formula_"  , formula.tempSubString(1));
	return code;
}
icu::UnicodeString get_string_python_code(icu::UnicodeString & formula, int col, int row)
{
	icu::UnicodeString code = R"(
result = try_parse_text('''_formula_''')
_colname__row_.set_val(result)

ourcalc_display_text = str(result)
ourcalc_display_type = result.get_final_val().__class__.__name__ if is_ourcell(result) else result.__class__.__name__
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

bool CellData::set_formula(icu::UnicodeString contents, int col, int row)
{
	if (formula == contents)
		return false;

	clear_dependencies(col, row);

	formula = contents;
	bool display_changed = reevaluate(col, row);

	// update dependent cells
	for (const auto & p : dependent_cells)
	{
		auto * cell = global_grid->get_cell_at(p.x, p.y);
		cell->reevaluate(p.x, p.y);
	}

	return display_changed;
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
			std::cout << e.what() << " " << __FILE__ << ": " << __LINE__ << std::endl;
			error = true;
			error_msg = e.what();
			display_changed = !there_was_en_error;
		}
		catch(...)
		{
			std::cout << "unknown exception" << " " << __FILE__ << ": " << __LINE__ << std::endl;
			error = true;
			error_msg = "Unknown exception while evaluating expression.";
			display_changed = !there_was_en_error;
		}
		return display_changed;
	}

	//std::cout << formula << std::endl;
	//std::cout << formula.substr(1) << std::endl;
	//std::cout << code << std::endl;
	auto formula_code = get_formula_python_code(formula, col, row);
	auto  parser_code = get_parse_python_code(formula);
	std::string utf8_formula_code;
	std::string utf8_parser_code;
	formula_code.toUTF8String(utf8_formula_code);
	 parser_code.toUTF8String(utf8_parser_code);
	//std::cout << utf8_formula_code << std::endl;
	try
	{
		// First parse the code

		py::exec(utf8_parser_code, globals, locals);
		std::vector<std::string> variables = split(locals["ourcalc_variables"].cast<std::string>(), ",");

		error = false;
		// Check for dependency cells that contain error
		for (const std::string & v : variables)
		{
			auto [ref_col, ref_row] = parse_cell_name(v);
			auto * cell = global_grid->get_cell_at(ref_col, ref_row);
			if (cell && cell->error)
			{
				error = true;
				error_msg = v + std::string(" has an error.");
				//return true;
			}
		}

		// add dependecies cells
		for (const std::string & v : variables)
		{
			auto [ref_col, ref_row] = parse_cell_name(v);
			auto * cell = global_grid->get_cell_at(ref_col, ref_row);
			if ( ! cell)
				continue;
			dependencies.insert(CellCoords{ref_col, ref_row});
			cell->add_dependent(col, row);
		}

		// check for circular dependencies
		if (this->do_dependencies_depend_on_us(dependencies, col, row))
		{
			error = true;
			error_msg = "Circula dependency";
		}

		// Now execute the code

		py::exec(utf8_formula_code, globals, locals);
		auto calculated_text = locals["ourcalc_display_text"].cast<std::string>();
		auto calculated_type = locals["ourcalc_display_type"].cast<std::string>();


		//std::cout << "Display text: " << display_text << std::endl;

		//std::cout << variables.size() << " variables in code: ";
		//for (auto & v : variables)
		//	std::cout << v;
		//std::cout << std::endl;

		if ( ! error)
		{
			type = calculated_type;
			display_changed = there_was_en_error;
			display_changed |= display.set_text(calculated_text);
		}
	}
	catch(std::exception & e)
	{
		std::cout << e.what() << " " << __FILE__ << ": " << __LINE__ << std::endl;
		error = true;
		error_msg = e.what();
		display_changed = !there_was_en_error;
	}
	catch(...)
	{
		std::cout << "unknown exception" << " " << __FILE__ << ": " << __LINE__ << std::endl;
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
		auto * cell = global_grid->get_cell_at(p.x, p.y);
		if ( ! cell)
			continue;
		cell->remove_dependent(col, row);
	}
	dependencies.clear();
}

bool CellData::do_dependencies_depend_on_us(decltype(dependencies) deps, unsigned int col, unsigned int row)
{
	auto this_pair = CellCoords{col, row};
	for (const auto & p : deps)
		if (p == this_pair)
			return true;
	deps.insert(this_pair);
	for (const auto & p : dependent_cells)
	{
		auto * cell = global_grid->get_cell_at(p.x, p.y);
		if ( ! cell)
			continue;
		if (cell->do_dependencies_depend_on_us(deps, p.x, p.y))
			return true;
	}
	return false;
}

horizontal_policy::alignment_t CellData::get_horizontal_alignment() const
{
	if (h_policy.alignment != horizontal_policy::alignment_t::none)
		return h_policy.alignment;
	return global_grid->get_horizontal_alignment(type);
}