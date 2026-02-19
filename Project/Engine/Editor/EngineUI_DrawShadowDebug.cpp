void EngineUI::DrawShadowDebug()
{
	if (!gameContext_) return;
	auto* world = gameContext_->GetWorld();
	if (!world) return;

	auto* renderSys = world->GetSystem<Tsumi::Framework::RenderSystem>();
	if (!renderSys) return;

	auto* shadowMap = renderSys->GetShadowMap();
	if (!shadowMap) {
		ImGui::Text("No Shadow Map active");
		return;
	}

	ImGui::Text("Shadow Map Debug View");
	ImGui::Text("Size: %d, Cascades: %d", shadowMap->GetSize(), shadowMap->GetCascadeCount());

	for (uint32_t i = 0; i < shadowMap->GetCascadeCount(); ++i) {
		ImGui::Text("Cascade %d", i);
		auto& srv = shadowMap->GetDebugSRV(i);
		if (srv.valid()) {
			// Show R channel as white (or red)
			// ImGui doesn't support swizzling in Image, so it will likely show Red channel as Red.
			ImGui::Image((ImTextureID)srv.gpu.ptr, { 256, 256 }, { 0,0 }, { 1,1 }, { 1,1,1,1 }, { 1,1,1,1 });
		}
		else {
			ImGui::Text("Invalid SRV");
		}
		
		if ((i + 1) % 2 != 0) ImGui::SameLine();
	}
	ImGui::NewLine();
}
