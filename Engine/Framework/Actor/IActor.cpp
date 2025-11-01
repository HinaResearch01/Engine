#include "IActor.h"

Tsumi::Framework::IActor::IActor(const std::string& name)
	: name_(name)
{
	state_ = State::Active;
}