#include "GameContext.h"
#include "Test/TestWorld.h"

using namespace Tsumi::Framework;

GameContext::GameContext()
{
	// 使用シーンの登録
	RegisterScene<TestWorld>("Test");

	// 初期シーン
	ChangeScene("Test");
}
