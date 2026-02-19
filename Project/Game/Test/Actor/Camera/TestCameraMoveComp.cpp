#include "TestCameraMoveComp.h"
#include "Framework/Actor/IActor.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "Framework/System/Input/InputSystem.h"
#include "Framework/World/World.h"

using namespace Tsumi::Framework;

void TestCameraMoveComp::Init()
{
}

void TestCameraMoveComp::Update(float deltaTime)
{
	auto* owner = GetOwner();
	if (!owner) return;

	auto* trans = owner->GetComponent<TransformComponent>();
	if (!trans) return;

	auto* world = owner->GetWorld();
	if (!world) return;

	auto* input = world->GetSystem<InputSystem>();
	if (!input) return;

	// Movement
	float speed = moveSpeed_ * deltaTime;
	if (input->IsHold(Input::Key::LSHIFT)) speed *= 2.0f;

	Tsumi::Math::Vec3f forward = trans->forward; 
	Tsumi::Math::Vec3f right = trans->right;     
	Tsumi::Math::Vec3f up = { 0.0f, 1.0f, 0.0f };

	if (input->IsHold(Input::Key::W)) trans->srt.translate += forward * speed;
	if (input->IsHold(Input::Key::S)) trans->srt.translate -= forward * speed;
	if (input->IsHold(Input::Key::D)) trans->srt.translate += right * speed;
	if (input->IsHold(Input::Key::A)) trans->srt.translate -= right * speed;
	if (input->IsHold(Input::Key::E)) trans->srt.translate += up * speed;
	if (input->IsHold(Input::Key::Q)) trans->srt.translate -= up * speed;

	// Rotation (Arrows)
	float rotSpeed = rotateSpeed_ * deltaTime;
	if (input->IsHold(Input::Key::UP)) trans->srt.rotate.x -= rotSpeed;
	if (input->IsHold(Input::Key::DOWN)) trans->srt.rotate.x += rotSpeed;
	if (input->IsHold(Input::Key::LEFT)) trans->srt.rotate.y -= rotSpeed;
	if (input->IsHold(Input::Key::RIGHT)) trans->srt.rotate.y += rotSpeed;
}
