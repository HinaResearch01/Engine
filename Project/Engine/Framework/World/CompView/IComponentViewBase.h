#pragma once

namespace Tsumi::Framework {

// 前方宣言
class IActor;

/*  */
struct IComponentViewBase
{
	virtual ~IComponentViewBase() = default;
	virtual void Refresh(IActor* actor) = 0;
	virtual void Remove(IActor* actor) = 0;
	virtual void Clear() = 0;
};

}