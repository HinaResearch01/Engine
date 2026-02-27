#include "TestCameraMoveComp.h"
#include "Framework/World/World.h"
#include "Framework/System/Input/InputSystem.h"
#include "Framework/Actor/IActor.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "imgui.h"

using namespace Tsumi::Framework;

void TestCameraMoveComp::Update(float dt)
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
	float speed = moveSpeed_ * dt;
	if (input->IsHold(Input::Key::LSHIFT)) speed *= 5.0f;

	tme::math::Vec3f forward = trans->forward; 
	tme::math::Vec3f right = trans->right;     
	tme::math::Vec3f up = { 0.0f, 1.0f, 0.0f };

	bool isMoved = false;

	if (input->IsHold(Input::Key::W)) { trans->srt.translate += forward * speed; isMoved = true; }
	if (input->IsHold(Input::Key::S)) { trans->srt.translate -= forward * speed; isMoved = true; }
	if (input->IsHold(Input::Key::D)) { trans->srt.translate += right * speed; isMoved = true; }
	if (input->IsHold(Input::Key::A)) { trans->srt.translate -= right * speed; isMoved = true; }
	if (input->IsHold(Input::Key::E)) { trans->srt.translate += up * speed; isMoved = true; }
	if (input->IsHold(Input::Key::Q)) { trans->srt.translate -= up * speed; isMoved = true; }

	// Rotation (Arrows)
	float rotSpeed = rotateSpeed_ * dt;
	if (input->IsHold(Input::Key::UP)) { trans->srt.rotate.x += rotSpeed; isMoved = true; }
	if (input->IsHold(Input::Key::DOWN)) { trans->srt.rotate.x -= rotSpeed; isMoved = true; }
	if (input->IsHold(Input::Key::LEFT)) { trans->srt.rotate.y += rotSpeed; isMoved = true; }
	if (input->IsHold(Input::Key::RIGHT)) { trans->srt.rotate.y -= rotSpeed; isMoved = true; }

	if (isMoved) {
		trans->MarkDirty();
	}
}

void TestCameraMoveComp::OnInspectorGui()
{
	if (ImGui::TreeNode("Camera Movement")) {
		ImGui::DragFloat("Move Speed", &moveSpeed_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("Rotate Speed", &rotateSpeed_, 0.1f, 0.0f, 200.0f);
		
		if (ImGui::Button("Reset Position")) {
			auto* trans = GetOwner()->GetComponent<TransformComponent>();
			if (trans) {
				trans->srt.translate = { 120.0f, 115.0f, -40.0f };
				trans->srt.rotate = { -30.0f, 35.0f, 0.0f };
				trans->MarkDirty();
			}
		}

		ImGui::TreePop();
	}
}
