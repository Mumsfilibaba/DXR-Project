#pragma once
#include "Defines.h"
#include "Types.h"

/*
* EComparisonFunc
*/

enum class EComparisonFunc
{
	COMPARISON_FUNC_NEVER			= 1,
	COMPARISON_FUNC_LESS			= 2,
	COMPARISON_FUNC_EQUAL			= 3,
	COMPARISON_FUNC_LESS_EQUAL		= 4,
	COMPARISON_FUNC_GREATER			= 5,
	COMPARISON_FUNC_NOT_EQUAL		= 6,
	COMPARISON_FUNC_GREATER_EQUAL	= 7,
	COMPARISON_FUNC_ALWAYS			= 8
};

const char* ToString(EComparisonFunc ComparisonFunc)
{
	switch (ComparisonFunc)
	{
	case EComparisonFunc::COMPARISON_FUNC_NEVER:			return "BLEND_FACTOR_ZERO";
	case EComparisonFunc::COMPARISON_FUNC_LESS:				return "BLEND_FACTOR_ONE";
	case EComparisonFunc::COMPARISON_FUNC_EQUAL:			return "BLEND_FACTOR_SRC_COLOR";
	case EComparisonFunc::COMPARISON_FUNC_LESS_EQUAL:		return "BLEND_FACTOR_INV_SRC_COLOR";
	case EComparisonFunc::COMPARISON_FUNC_GREATER:			return "BLEND_FACTOR_SRC_ALPHA";
	case EComparisonFunc::COMPARISON_FUNC_NOT_EQUAL:		return "BLEND_FACTOR_INV_SRC_ALPHA";
	case EComparisonFunc::COMPARISON_FUNC_GREATER_EQUAL:	return "BLEND_FACTOR_DEST_ALPHA";
	case EComparisonFunc::COMPARISON_FUNC_ALWAYS:			return "BLEND_FACTOR_INV_DEST_ALPHA";
	default:												return "";
	}
}