#pragma once
#include <utility>
#include <functional>

/*
* HashHelpers
*/

template<typename T>
inline void HashCombine(size_t& OutHash, const T& Value)
{
	std::hash<T> Hasher;
	OutHash ^= Hasher(Value) + 0x9e3779b9 + (OutHash << 6) + (OutHash >> 2);
}

/*
* std::hash for DirectXMath
*/

namespace std
{
	template<> struct hash<XMFLOAT4>
	{
		size_t operator()(const XMFLOAT4& XmFloat) const
		{
			std::hash<float> Hasher;

			size_t Hash = Hasher(XmFloat.x);
			HashCombine<float>(Hash, XmFloat.y);
			HashCombine<float>(Hash, XmFloat.z);
			HashCombine<float>(Hash, XmFloat.w);

			return Hash;
		}
	};

	template<> struct hash<XMFLOAT3>
	{
		size_t operator()(const XMFLOAT3& XmFloat) const
		{
			std::hash<float> Hasher;

			size_t Hash = Hasher(XmFloat.x);
			HashCombine<float>(Hash, XmFloat.y);
			HashCombine<float>(Hash, XmFloat.z);

			return Hash;
		}
	};

	template<> struct hash<XMFLOAT2>
	{
		size_t operator()(const XMFLOAT2& XmFloat) const
		{
			std::hash<float> Hasher;

			size_t Hash = Hasher(XmFloat.x);
			HashCombine<float>(Hash, XmFloat.y);

			return Hash;
		}
	};
}