#include <cstring>

#include "command.h"

namespace fixedphilip
{
	command::command(const char* name, const char* description, run_function* run)
	{
		_name = name;
		_description = description;
		_run = run;

		// if the linked list is empty, we're the first command
		if (!_first)
		{
			_first = this;
			_last = this;
			return;
		}

		command* current_iter = _first;
		command* previous_iter = nullptr;
		while (current_iter)
		{
			// does our command alphabetically precede the currently iterated command?
			if (strcmp(name, current_iter->_name) < 0)
			{
				// squeeze inbetween the two commands
				_next = current_iter;
				_prev = previous_iter;

				// now update the two commands we squeezed between
				current_iter->_prev = this;
				if (previous_iter)
				{
					previous_iter->_next = this;
				}
				else
				{
					// no previous iterator - we're the first command
					_first = this;
				}
				return;
			}

			// it does not, so proceed with next iteration
			previous_iter = current_iter;
			current_iter = current_iter->_next;
		}

		// we are the last command
		_prev = previous_iter;
		previous_iter->_next = this;
		_last = this;
	}

	command::~command()
	{
		if (_prev)
		{
			_prev->_next = _next;
		}
		else
		{
			// no previous command - we're the first
			_first = _next;
		}

		if (_next)
		{
			_next->_prev = _prev;
		}
		else
		{
			// no following command - we're the last
			_last = _prev;
		}
	}
}