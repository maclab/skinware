/*
 * Copyright (C) 2007-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of shCompiler.
 *
 * shCompiler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * shCompiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shCompiler.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <list>

using namespace std;

template<class T>
class shKLimitedList
{
public:
	int k;
	int historyMax;
	int size;
	bool changed;
	T empty;
	bool useHistory;
	list<T> history;			// The nearer to head, the more recent thrown away
	list<T> lst;				// The nearer to head, the earlier inserted
	list<T> temporary;
	shKLimitedList(int kk, int hm, T empT)
	{
		k = kk;
		historyMax = hm;
		size = 0;
		empty = empT;
		changed = true;
		useHistory = false;
	}
	list<T> &k_elements()
	{
		if (size == k)
			return lst;
		if (!changed)
			return temporary;
		temporary = lst;
		for (int i = size; i < k; ++i)
			temporary.push_back(empty);
		changed = false;
		return temporary;
	}
	shKLimitedList<T> &operator <<(T &t)
	{
		changed = true;
		lst.push_back(t);
		if (size == k)
		{
			if (useHistory)
			{
				history.push_front(*lst.begin());
				lst.pop_front();
				if ((int)history.size() > historyMax)
					history.pop_back();
			}
			else
				lst.pop_front();
		}
		else
			++size;
		return *this;
	}
	void insertEmpty(int c)
	{
		if (c > k)
			c = k;						// To prevent inserting empty inside history
		for (int i = 0; i < c; ++i)
			*this << empty;
	}
	void flushList()						// flush the list of tokens, but not the history
	{
		size = 0;
		lst.clear();
		changed = true;
	}
	void clear()							// clear everything
	{
		size = 0;
		lst.clear();
		history.clear();
		temporary.clear();
		changed = true;
	}
	shKLimitedList<T> &operator =(shKLimitedList<T> &l)
	{
		k = l.k;
		historyMax = l.historyMax;
		size = l.size;
		changed = l.changed;
		empty = l.empty;
		useHistory = l.useHistory;
		history = l.history;
		lst = l.lst;
		temporary = l.temporary;
		return *this;
	}
	shKLimitedList(shKLimitedList<T> &l)				// The copy constructor
	{
		*this = l;
	}
};
