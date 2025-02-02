
#pragma once

enum event_type
{
	nop = 0,
	window_shown = 1,
	mouse,
	key,
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

struct event
{
	event_type type;
	union data
	{
		int nothin_sandwich;
		mouse_data mouse;
		key_data   key;
	} data;
};
