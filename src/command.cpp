#include "command.h"

#include <cstring>

namespace fixedphilip
{
	Command::Command(const char* name, const char* description, RunFn* run)
	{
		this->name = name;
		this->description = description;
		this->run = run;

		// if the linked list is empty, we're the first module
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
			// does our module alphabetically precede the currently iterated module?
			if (strcmp(name, current_iter->name) < 0)
			{
				// squeeze inbetween the two modules
				next = current_iter;
				prev = previous_iter;

				// now update the two modules we squeezed between
				current_iter->prev = this;
				if (previous_iter)
				{
					previous_iter->next = this;
				}
				else
				{
					// no previous iterator - we're the first module
					first = this;
				}
				return;
			}

			// it does not, so proceed with next iteration
			previous_iter = current_iter;
			current_iter = current_iter->next;
		}

		// we are the last module
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
			// no previous module - we're the first
			first = next;
		}

		if (next)
		{
			next->prev = prev;
		}
		else
		{
			// no following module - we're the last
			last = prev;
		}
	}
}