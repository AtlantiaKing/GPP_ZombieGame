#include "stdafx.h"
#include "InventoryManager.h"
#include "WorldExplorer.h"

InventoryManager::InventoryManager(IExamInterface* pInterface)
	: m_pInterface{ pInterface }
{
	// Init the inventory
	for (ItemInfo& item : m_Inventory)
	{
		item.Type = eItemType::_LAST;
	}
}

UINT InventoryManager::GetFirstOpenSlot() const
{
	// For each item
	for (UINT i{}; i < m_InventoryAmount; ++i)
	{
		// If item has no type, return this item
		if (m_Inventory[i].Type == eItemType::_LAST) return i;
	}
	return 0;
}

void InventoryManager::Update(float health, float energy)
{
	// For each item
	for (UINT i{}; i < m_InventoryAmount; ++i)
	{
		// If the item is not food or medkit, continue to the next item
		if (m_Inventory[i].Type != eItemType::FOOD && m_Inventory[i].Type != eItemType::MEDKIT) continue;

		// Get the item info from the real inventory
		ItemInfo itemInInventory;
		if (m_pInterface->Inventory_GetItem(i, itemInInventory))
		{
			switch (itemInInventory.Type)
			{
			case eItemType::MEDKIT:
			{
				// Get the health of the medkit
				int healthInMedkit{ m_pInterface->Medkit_GetHealth(itemInInventory) };
				constexpr float maxHealth{ 10 };

				// If agent can use the full medkit
				if (maxHealth - health > healthInMedkit)
				{
					// Use and remove the medkit
					m_pInterface->Inventory_UseItem(i);
					m_pInterface->Inventory_RemoveItem(i);
					m_Inventory[i].Type = eItemType::_LAST;

					return;
				}
				break;
			}
			case eItemType::FOOD:
			{
				// Get the energy of the food
				const int foodInItem{ m_pInterface->Food_GetEnergy(itemInInventory) };
				constexpr float maxEnergy{ 10 };

				// If agent can use the full food
				if (maxEnergy - energy > foodInItem)
				{
					// Use and remove the food
					m_pInterface->Inventory_UseItem(i);
					m_pInterface->Inventory_RemoveItem(i);
					m_Inventory[i].Type = eItemType::_LAST;

					return;
				}
				break;
			}
			}
		}
	}
}

bool InventoryManager::PickUpEntity(const EntityInfo& entity)
{
	// Get iteminfo from the entity
	ItemInfo itemInfo;
	if (m_pInterface->Item_Grab(entity, itemInfo))
	{
		// Get the first open inventory slot
		const UINT nextIdx{ GetFirstOpenSlot() };
		// Add the item to the inventory
		const bool addedItem{ m_pInterface->Inventory_AddItem(nextIdx, itemInfo) };
		if (addedItem)
		{
			// If the item is garbage, destroy it
			// Else, save it in the local inventory
			if (itemInfo.Type == eItemType::GARBAGE)
			{
				m_pInterface->Inventory_RemoveItem(nextIdx);
				m_Inventory[nextIdx].Type = eItemType::_LAST;
			}
			else
			{
				m_Inventory[nextIdx] = itemInfo;
			}
		}

		return addedItem;
	}

	return false;
}

bool InventoryManager::ReplaceItemWithEntity(UINT index, const EntityInfo& entity)
{
	// Get the iteminfo from the real inventory
	ItemInfo itemInfo;
	if (!m_pInterface->Inventory_GetItem(index, itemInfo)) return false;

	// If the previous item is food or medkit, use it
	if (itemInfo.Type == eItemType::FOOD || itemInfo.Type == eItemType::MEDKIT)
	{
		if (!m_pInterface->Inventory_UseItem(index)) return false;
	}

	// Remove the previous item
	if (!m_pInterface->Inventory_RemoveItem(index)) return false;
	m_Inventory[index].Type = eItemType::_LAST;

	// Pick up the new item
	return PickUpEntity(entity);
}

bool InventoryManager::HasMedkit() const
{
	// For each item
	for (const ItemInfo& item : m_Inventory)
	{
		if (item.Type == eItemType::MEDKIT) return true;
	}
	return false;
}

bool InventoryManager::HasFood() const
{
	for (const ItemInfo& item : m_Inventory)
	{
		if (item.Type == eItemType::FOOD) return true;
	}
	return false;
}

bool InventoryManager::HasPistol() const
{
	for (const ItemInfo& item : m_Inventory)
	{
		if (item.Type == eItemType::PISTOL) return true;
	}
	return false;
}

bool InventoryManager::HasShotgun() const
{
	for (const ItemInfo& item : m_Inventory)
	{
		if (item.Type == eItemType::SHOTGUN) return true;
	}
	return false;
}

bool InventoryManager::ShootPistol()
{
	// For each item
	for (int i{}; i < m_InventoryAmount; ++i)
	{
		const ItemInfo& item{ m_Inventory[i] };
		// If the item is not a pistol, continue to the next item
		if (item.Type != eItemType::PISTOL) continue;

		// Get the iteminfo from the real inventory
		ItemInfo pistol;
		if (m_pInterface->Inventory_GetItem(i, pistol))
		{
			// Use the pistol
			if (m_pInterface->Inventory_UseItem(i))
			{
				// If the pistol is empty
				if (m_pInterface->Weapon_GetAmmo(pistol) == 0)
				{
					// Remove the pistol
					m_pInterface->Inventory_RemoveItem(i);
					m_Inventory[i].Type = eItemType::_LAST;
				}

				return true;
			}
		}
	}
	return false;
}

bool InventoryManager::ShootShotgun()
{
	// For each item
	for (int i{}; i < m_InventoryAmount; ++i)
	{
		const ItemInfo& item{ m_Inventory[i] };
		// If the item is not a shotgun, continue to the next item
		if (item.Type != eItemType::SHOTGUN) continue;

		// Get the iteminfo from the real inventory
		ItemInfo shotgun;
		if (m_pInterface->Inventory_GetItem(i, shotgun))
		{
			// Use the shotgun
			if (m_pInterface->Inventory_UseItem(i))
			{
				// If the shotgun is empty
				if (m_pInterface->Weapon_GetAmmo(shotgun) == 0)
				{
					// Remove the shotgun
					m_pInterface->Inventory_RemoveItem(i);
					m_Inventory[i].Type = eItemType::_LAST;
				}

				return true;
			}
		}
	}
	return false;
}

UINT InventoryManager::IsBetterInventoryPossible(const EntityInfo& entity, const std::vector<FoundEntityInfo>& foundEntities) const
{
	ItemInfo itemInfo;
	if (!m_pInterface->Item_GetInfo(entity, itemInfo)) return 10;

	const AgentInfo agentInfo{ m_pInterface->Agent_GetInfo() };

	int pistolCount{};
	int lowestPistolAmmo{ INT_MAX };
	UINT pistolIndex{};

	int shotgunCount{};
	int lowestShotgunAmmo{ INT_MAX };
	UINT shotgunIndex{};

	int medkitCount{};
	int lowestHealth{ INT_MAX };
	UINT medkitIndex{};

	int foodCount{};
	int lowestFood{ INT_MAX };
	UINT foodIndex{};

	// For each item
	for (UINT i{}; i < m_InventoryAmount; ++i)
	{
		// If the item has no type, continue to the next item
		if (m_Inventory[i].Type == eItemType::_LAST) continue;

		// Get the iteminfo from the real inventory
		// Increment the correct itemtype and save the weakest item
		ItemInfo itemInInventory;
		if (m_pInterface->Inventory_GetItem(i, itemInInventory))
		{
			switch (itemInInventory.Type)
			{
			case eItemType::PISTOL:
			{
				++pistolCount;
				const int ammo{ m_pInterface->Weapon_GetAmmo(itemInInventory) };
				if (ammo < lowestPistolAmmo)
				{
					lowestPistolAmmo = ammo;
					pistolIndex = i;
				}
				break;
			}
			case eItemType::SHOTGUN:
			{
				++shotgunCount;
				const int ammo{ m_pInterface->Weapon_GetAmmo(itemInInventory) };
				if (ammo < lowestShotgunAmmo)
				{
					lowestShotgunAmmo = ammo;
					shotgunIndex = i;
				}
				break;
			}
			case eItemType::MEDKIT:
			{
				++medkitCount;
				const int health{ m_pInterface->Medkit_GetHealth(itemInInventory) };
				if (health < lowestHealth)
				{
					lowestHealth = health;
					medkitIndex = i;
				}
				break;
			}
			case eItemType::FOOD:
			{
				++foodCount;
				const int food{ m_pInterface->Food_GetEnergy(itemInInventory) };
				if (food < lowestFood && 10.0f - agentInfo.Energy >= lowestFood)
				{
					lowestFood = food;
					foodIndex = i;
				}
				break;
			}
			case eItemType::GARBAGE:
			{
				return i;
			}
			}
		}
	}

	constexpr int maxGunsToRemember{ 3 };
	constexpr int gotoPistolCount{ 2 };
	constexpr int gotoItemCount{ 1 };

	switch (itemInfo.Type)
	{
	case eItemType::PISTOL:
	{
		const int ammo{ m_pInterface->Weapon_GetAmmo(itemInfo) };
		int knownPistols{};
		for (const FoundEntityInfo& foundEntity : foundEntities)
		{
			if (foundEntity.itemType == eItemType::PISTOL) ++knownPistols;
		}

		if (knownPistols > maxGunsToRemember || pistolCount > 0 && lowestPistolAmmo < ammo) return pistolIndex;
		else if (pistolCount == 0)
		{
			if (shotgunCount > gotoItemCount) return shotgunIndex;
			if (medkitCount > gotoItemCount) return medkitIndex;
			if (foodCount > gotoItemCount) return foodIndex;
		}
		break;
	}
	case eItemType::SHOTGUN:
	{
		const int ammo{ m_pInterface->Weapon_GetAmmo(itemInfo) };
		int knownShotguns{};
		for (const FoundEntityInfo& foundEntity : foundEntities)
		{
			if (foundEntity.itemType == eItemType::SHOTGUN) ++knownShotguns;
		}

		if (knownShotguns > maxGunsToRemember || shotgunCount > 0 && lowestShotgunAmmo < ammo) return shotgunIndex;
		else if (shotgunCount == 0)
		{
			if (pistolCount > gotoPistolCount) return pistolIndex;
			if (medkitCount > gotoItemCount) return medkitIndex;
			if (foodCount > gotoItemCount) return foodIndex;
		}
		break;
	}
	case eItemType::MEDKIT:
	{
		const int health{ m_pInterface->Medkit_GetHealth(itemInfo) };
		if (medkitCount > 0 && (lowestHealth < health)) return medkitIndex;
		else if (medkitCount == 0)
		{
			if (pistolCount > gotoPistolCount) return pistolIndex;
			if (shotgunCount > gotoItemCount) return shotgunIndex;
			if (foodCount > gotoItemCount) return foodIndex;
		}
		break;
	}
	case eItemType::FOOD:
	{
		const int food{ m_pInterface->Food_GetEnergy(itemInfo) };
		if (foodCount > 0 && (lowestFood < food)) return foodIndex;
		else if (foodCount == 0)
		{
			if (pistolCount > gotoPistolCount) return pistolIndex;
			if (shotgunCount > gotoItemCount) return shotgunIndex;
			if (medkitCount > gotoItemCount) return medkitIndex;
		}
		break;
	}
	case eItemType::GARBAGE:
	{
		if (pistolCount > gotoPistolCount + 1 || (pistolCount > 1 && lowestPistolAmmo <= 2)) return pistolIndex;
		if (shotgunCount > gotoItemCount + 1 || lowestShotgunAmmo <= 2) return shotgunIndex;
		if (medkitCount > gotoItemCount + 1 || (medkitCount > 1 && lowestHealth <= 2)) return medkitIndex;
		if (foodCount > gotoItemCount + 1 || lowestFood <= 2) return foodIndex;
		break;
	}
	}

	return 10;
}

bool InventoryManager::IsInventoryFull() const
{
	int inventorySize{};
	for (UINT i{}; i < m_InventoryAmount; ++i)
	{
		if (m_Inventory[i].Type != eItemType::_LAST) continue;

		++inventorySize;
	}

	return inventorySize == 0;
}
