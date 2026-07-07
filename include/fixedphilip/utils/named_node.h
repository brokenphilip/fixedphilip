#pragma once

#include <cstring> // strcmp

namespace fixedphilip::utils
{
	template <typename T>
	class named_node
	{
	private:
		static inline T* first_ = nullptr;
		static inline T* last_ = nullptr;

		T* prev_ = nullptr;
		T* next_ = nullptr;

		const char* name_;
	public:
		inline named_node(const char* name) : name_(name)
		{
			// if the linked list is empty, we're the first node
			if (!first_)
			{
				first_ = static_cast<T*>(this);
				last_ = static_cast<T*>(this);
				return;
			}

			T* current_iter = first_;
			T* previous_iter = nullptr;
			while (current_iter)
			{
				// named nodes are sorted alphabetically
				if (strcmp(static_cast<T*>(this)->name_, current_iter->name_) < 0)
				{
					// squeeze inbetween the two nodes
					next_ = current_iter;
					prev_ = previous_iter;

					// now update the two nodes we squeezed between
					current_iter->prev_ = static_cast<T*>(this);
					if (previous_iter)
					{
						previous_iter->next_ = static_cast<T*>(this);
					}
					else
					{
						// no previous iterator - we're the first node
						first_ = static_cast<T*>(this);
					}
					return;
				}

				// it does not, so proceed with next iteration
				previous_iter = current_iter;
				current_iter = current_iter->next_;
			}

			// we are the last node
			prev_ = previous_iter;
			previous_iter->next_ = static_cast<T*>(this);
			last_ = static_cast<T*>(this);
		}

		inline ~named_node()
		{
			if (prev_)
			{
				prev_->next_ = next_;
			}
			else
			{
				// no previous node - we're the first
				first_ = next_;
			}

			if (next_)
			{
				next_->prev_ = prev_;
			}
			else
			{
				// no following node - we're the last
				last_ = prev_;
			}
		}

		static inline auto first() { return first_; }
		static inline auto last() { return last_; }

		inline auto previous() { return prev_; }
		inline auto next() { return next_; }

		inline auto name() { return name_; }
	};
}