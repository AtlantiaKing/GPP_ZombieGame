#pragma once
#include <Exam_HelperStructs.h>

struct CurrentHouse : public HouseInfo
{
	int curCornerIndex{};
};

struct FoundEntityInfo : public EntityInfo
{
	eItemType itemType{};
};