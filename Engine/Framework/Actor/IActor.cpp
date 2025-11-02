#include "IActor.h"

Tsumi::Framework::IActor::IActor(const std::string& name)
	: name_(name)
{
	// 状態はActiveで初期化
	state_ = State::Active;

	// TransformComponentの生成
	transComp_ = std::make_shared<TransformComponent>();
	transComp_->SetOwner(this);
	transComp_->Init();
}