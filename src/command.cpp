#include <fixedphilip/command.h>

#include <cstring> // strcmp

namespace fixedphilip
{
	command::command(const char* name, const char* description, run_function* run)
	{
		name_ = name;
		description_ = description;
		run_ = run;

		// if the linked list is empty, we're the first command
		if (!first_)
		{
			first_ = this;
			last_ = this;
			return;
		}

		command* current_iter = first_;
		command* previous_iter = nullptr;
		while (current_iter)
		{
			// does our command alphabetically precede the currently iterated command?
			if (strcmp(name, current_iter->name_) < 0)
			{
				// squeeze inbetween the two commands
				next_ = current_iter;
				prev_ = previous_iter;

				// now update the two commands we squeezed between
				current_iter->prev_ = this;
				if (previous_iter)
				{
					previous_iter->next_ = this;
				}
				else
				{
					// no previous iterator - we're the first command
					first_ = this;
				}
				return;
			}

			// it does not, so proceed with next iteration
			previous_iter = current_iter;
			current_iter = current_iter->next_;
		}

		// we are the last command
		prev_ = previous_iter;
		previous_iter->next_ = this;
		last_ = this;
	}

	command::~command()
	{
		if (prev_)
		{
			prev_->next_ = next_;
		}
		else
		{
			// no previous command - we're the first
			first_ = next_;
		}

		if (next_)
		{
			next_->prev_ = prev_;
		}
		else
		{
			// no following command - we're the last
			last_ = prev_;
		}
	}
}