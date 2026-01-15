#include "Framework/Actor/IActor.h"
#include "Framework/World/World.h"

namespace Tsumi::Framework {

IActor::IActor() {
	state_ = State::Active;
	EnsureTransform();
}

IActor::~IActor() {
	std::lock_guard<std::mutex> lock(mutex_);
	comps_.clear();
}

void IActor::Finalize()
{
	if (!world_) {
		comps_.clear();
		return;
	}

	comps_.clear();
}

void IActor::EnsureTransform() {
	if (comps_.contains(typeid(TransformComponent)))
		return;

	auto trans = std::make_shared<TransformComponent>();
	trans->SetOwner(this);
	trans->Init();

	comps_[typeid(TransformComponent)] = trans;
}

} 
