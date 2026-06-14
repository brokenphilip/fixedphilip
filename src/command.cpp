#include <cstring>

#include "command.h"

namespace fixedphilip
{
	Command::Command(const char* name, const char* description, RunFn* run)
	{
		this->name = name;
		this->description = description;
		this->run = run;

		// if the linked list is empty, we're the first command
		if (!first)
		{
			first = this;
			last = this;
			return;
		}

		Command* current_iter = first;
		Command* previous_iter = nullptr;
		while (current_iter)
		{
			// does our command alphabetically precede the currently iterated command?
			if (strcmp(name, current_iter->name) < 0)
			{
				// squeeze inbetween the two commands
				next = current_iter;
				prev = previous_iter;

				// now update the two commands we squeezed between
				current_iter->prev = this;
				if (previous_iter)
				{
					previous_iter->next = this;
				}
				else
				{
					// no previous iterator - we're the first command
					first = this;
				}
				return;
			}

			// it does not, so proceed with next iteration
			previous_iter = current_iter;
			current_iter = current_iter->next;
		}

		// we are the last command
		prev = previous_iter;
		previous_iter->next = this;
		last = this;
	}

	Command::~Command()
	{
		if (prev)
		{
			prev->next = next;
		}
		else
		{
			// no previous command - we're the first
			first = next;
		}

		if (next)
		{
			next->prev = prev;
		}
		else
		{
			// no following command - we're the last
			last = prev;
		}
	}
}