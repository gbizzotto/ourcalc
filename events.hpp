
#pragma once

enum event_type
{
	nop = 0,
	window_shown = 1,
	window_resized,
	mouse,
	key,
	text,
};

struct mouse_data
{
	bool pressed;
	bool released;
	int button;
	int x, y;
};
struct key_data
{
	bool pressed;
	bool released;
	int keycode;
	int charcode;
};
struct window_resized_data
{
	int w, h;
};
struct text_data
{
	char * composition;
	int cursor_pos;
	int selection_len;
};

struct event
{
	event_type type;
	union data
	{
		int nothin_sandwich;
		mouse_data mouse;
		key_data   key;
		window_resized_data window_resized;
		text_data  text;
	} data;
};
