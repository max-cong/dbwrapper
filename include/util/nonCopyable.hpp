#pragma once
class nonCopyable
{
public:
	nonCopyable(const nonCopyable &) = delete;			  // deleted
	nonCopyable &operator=(const nonCopyable &) = delete; // deleted
	nonCopyable() = default;							  // available

	nonCopyable(nonCopyable &&fp) = delete;
	nonCopyable const &operator=(nonCopyable &&fp) = delete;
};