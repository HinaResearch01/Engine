#pragma once

#include <vector>
#include <tuple>
#include <algorithm>
#include "Framework/Actor/IActor.h"

namespace Tsumi::Framework {

class World;
class IActor;

template<class... Cs>
class ViewRange
{
public:
	ViewRange(World& w, const std::vector<IActor*>& base)
		: world_(&w), base_(&base) {
	}

	struct Iterator
	{
		World* world;
		std::vector<IActor*>::const_iterator it;
		std::vector<IActor*>::const_iterator end;

		Iterator& operator++()
		{
			do {
				++it;
			} while (it != end && !HasAll());
			return *this;
		}

		bool operator!=(const Iterator& r) const { return it != r.it; }

		auto operator*() const
		{
			IActor* a = *it;
			return std::tuple<Cs&...>(*a->GetComponent<Cs>()...);
		}

	private:
		bool HasAll() const
		{
			IActor* a = *it;
			return (a->HasComp<Cs>() && ...);
		}
	};

	Iterator begin()
	{
		auto it = base_->begin();
		while (it != base_->end() && !((*it)->HasComp<Cs>() && ...))
			++it;
		return { world_, it, base_->end() };
	}

	Iterator end()
	{
		return { world_, base_->end(), base_->end() };
	}

private:
	World* world_;
	const std::vector<IActor*>* base_;
};

}