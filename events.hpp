
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
	enum Mods
	{
	    NONE     = 0x0000,
	    LSHIFT   = 0x0001,
	    RSHIFT   = 0x0002,
	    LCTRL    = 0x0040,
	    RCTRL    = 0x0080,
	    LALT     = 0x0100,
	    RALT     = 0x0200,
	    LGUI     = 0x0400,
	    RGUI     = 0x0800,
	    NUM      = 0x1000,
	    CAPS     = 0x2000,
	    MODE     = 0x4000,
	    RESERVED = 0x8000
	};

	bool pressed;
	bool released;
	int keycode;
	int charcode;
	int mod; // combination of Mods
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
