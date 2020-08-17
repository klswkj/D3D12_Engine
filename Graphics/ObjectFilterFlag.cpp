#include "ObjectFilterFlag.h"

enum class eObjectFilterFlag
{
	kNone = 0x0,
	kOpaque = 0x1,
	kCutout = 0x2,
	kTransparent = 0x4,
	kAll = 0xF
};

bool operator&&(eObjectFilterFlag lhs, eObjectFilterFlag rhs)
{
	return static_cast<bool>(static_cast<int>(lhs) && static_cast<int>(rhs));
}