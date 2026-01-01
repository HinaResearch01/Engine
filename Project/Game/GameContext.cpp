#include "GameContext.h"
#include "Test/TestScene.h"

using namespace Tsumi::Framework;

GameContext::GameContext()
{
	// 使用シーンの登録
	RegisterScene<TestScene>("Test");

	// 初期シーン
	ChangeScene("Test");
}
