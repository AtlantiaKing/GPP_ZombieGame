#pragma once
#include <Exam_HelperStructs.h>
#include <IExamInterface.h>
#include "ExtendedStructs.h"

class WorldExplorer;

class InventoryManager final
{
public:
	InventoryManager(IExamInterface* pInterface);

	void Update(float health, float energy);
	bool PickUpEntity(const EntityInfo& entity);
	bool ReplaceItemWithEntity(UINT index, const EntityInfo& entity);

	bool ShootPistol();
	bool ShootShotgun();
	UINT IsBetterInventoryPossible(const EntityInfo& entity, const std::vector<FoundEntityInfo>& foundEntities) const;
	bool IsInventoryFull() const;
	bool HasMedkit() const;
	bool HasFood() const;
	bool HasPistol() const;
	bool HasShotgun() const;
private:
	IExamInterface* m_pInterface{};

	UINT GetFirstOpenSlot() const;

	constexpr static int m_InventoryAmount{ 5 };
	ItemInfo m_Inventory[m_InventoryAmount]{};
};

