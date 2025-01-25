
#pragma once

#include <vector>
#include <utility>
#include <functional>

template<typename E>
struct monitorable;

template<typename E>
struct monitor
{
	std::vector<std::tuple<monitorable<E>*, E, std::function<void()>>> expected;

	void expect(monitorable<E> * obj, E ev, std::function<void()> func)
	{
		expected.push_back(std::make_tuple(obj, ev, func));
	}

	virtual void _notify(monitorable<E> * m, E & e)
	{
		size_t n = std::erase_if(expected, [&](decltype(expected)::value_type & t)
				{
					//typename decltype(expected)::value_type & t = *it;
					if (std::get<0>(t) == m && std::get<1>(t) == e)
					{
						std::get<2>(t)();
						return true;
					}
					return false;
				}
			);
		if ( ! n)
			notify(m, e);
	}

	virtual void notify(monitorable<E> *, E &) = 0;
};

template<typename E>
struct monitorable
{
	std::vector<std::pair<E,monitor<E>*>> monitors;

	void notify_monitors(E t)
	{
		for(auto & p : monitors)
			if (p.first == t)
				p.second->_notify(this, t);
	}
	void add_monitor(monitor<E> * m, E e)
	{
		monitors.push_back(std::make_pair(e,m));
	}
};
