#include "Framework/Actor/IActor.h"
#include "Framework/World/World.h"

namespace Tsumi::Framework {

IActor::IActor() {
	state_ = State::Active;
	EnsureTransform();
}

IActor::IActor(const std::string& name)
	: name_(name) {
	state_ = State::Active;
	EnsureTransform();
}

IActor::~IActor() {
	std::lock_guard<std::mutex> lock(mutex_);

	for (auto& kv : comps_) {
		NotifyComponentRemoved(kv.second.get());
	}

	comps_.clear();
	transComp_.reset();
}

void IActor::Finalize()
{
	if (!world_) {
		comps_.clear();
		return;
	}

	for (auto& [type, comp] : comps_) {
		world_->OnComponentRemoved(this, comp.get());
	}

	comps_.clear();
}

void IActor::EnsureTransform() {
	if (transComp_) return;

	transComp_ = std::make_shared<TransformComponent>();
	transComp_->SetOwner(this);
	transComp_->Init();
	comps_[typeid(TransformComponent)] = transComp_;
}

void IActor::NotifyComponentAdded(IComponent* comp) {
	if (world_) {
		world_->OnComponentAdded(this, comp);
	}
}

void IActor::NotifyComponentRemoved(IComponent* comp) {
	if (world_) {
		world_->OnComponentRemoved(this, comp);
	}
}

} 
