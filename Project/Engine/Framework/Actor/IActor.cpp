#include "Framework/Actor/IActor.h"
#include "Framework/World/World.h"

namespace Tsumi::Framework {

IActor::IActor() {
	EnsureTransform();
}

void IActor::EnsureTransform() {
	if (comps_.contains(typeid(TransformComponent))) return;

	auto tr = std::make_unique<TransformComponent>();
	tr->SetOwner(this);
	tr->Init();

	comps_[typeid(TransformComponent)] = std::move(tr);
}

void IActor::NotifyComponentAdded(IComponent* c) {
	if (!world_ || !c) return;
	world_->OnComponentAdded(this, c);
}

void IActor::NotifyComponentRemoved(IComponent* c) {
	if (!world_ || !c) return;
	world_->OnComponentRemoved(this, c);
}

} 
