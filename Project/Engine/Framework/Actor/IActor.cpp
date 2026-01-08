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

	comps_.clear();
	transComp_.reset();
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
	if (transComp_) return;

	transComp_ = std::make_shared<TransformComponent>();
	transComp_->SetOwner(this);
	transComp_->Init();
	comps_[typeid(TransformComponent)] = transComp_;
}

} 
