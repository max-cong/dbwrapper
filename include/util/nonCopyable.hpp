#pragma once
class nonCopyable
{
public:
	nonCopyable(const nonCopyable&) = delete; // deleted
	nonCopyable& operator = (const nonCopyable&) = delete; // deleted
	nonCopyable() = default;   // available
};