/*=============================================================================*/
// Copyright 2020-2021 Elite Engine
/*=============================================================================*/
// Behaviors.h: Implementation of certain reusable behaviors
/*=============================================================================*/
#include "EBlackboard.h"
#include "EBehaviorTree.h"
#include "ExtendedStructs.h"
#include "InventoryManager.h"
#include "Steering.h"
#include <Exam_HelperStructs.h>
#include <EliteMath/EVector2.h>
#ifndef ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
#define ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS

//-----------------------------------------------------------------
// Behaviors
//-----------------------------------------------------------------

namespace BT_Actions
{
	// Add flee and lookat from the current entity target to the steering
	Elite::BehaviorState AddToFleeAndLookAt(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return Elite::BehaviorState::Failure;

		Steering* pSteering;
		if (!pBlackboard->GetData("Steering", pSteering))
			return Elite::BehaviorState::Failure;

		Elite::Vector2 target;
		if (!pBlackboard->GetData("EntityTarget", target))
			return Elite::BehaviorState::Failure;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		pSteering->AddFlee(target, agentInfo);
		pSteering->LookAt(target);

		return Elite::BehaviorState::Success;
	}

	// Add seek to the current entity target to the steering
	Elite::BehaviorState AddToEntitySeek(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return Elite::BehaviorState::Failure;

		Steering* pSteering;
		if (!pBlackboard->GetData("Steering", pSteering))
			return Elite::BehaviorState::Failure;

		Elite::Vector2 target;
		if (!pBlackboard->GetData("EntityTarget", target))
			return Elite::BehaviorState::Failure;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// Get the closest navmesh point
		const Elite::Vector2 nextTargetPos = pInterface->NavMesh_GetClosestPathPoint(target);

		// Add seek to the navmesh target
		pSteering->AddSeek(nextTargetPos, agentInfo);

		std::vector<EntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityFovVec", pEntityVec))
			return Elite::BehaviorState::Failure;

		// If there is enough stamina, run
		if (agentInfo.RunMode)
		{
			if (agentInfo.Stamina > 2.0f) pSteering->Run();
		}
		else
		{
			if (agentInfo.Stamina >= 10.0f) pSteering->Run();
		}

		return Elite::BehaviorState::Success;
	}

	// Add seek to the current house target to the steering
	Elite::BehaviorState AddToHouseSeek(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return Elite::BehaviorState::Failure;

		Steering* pSteering;
		if (!pBlackboard->GetData("Steering", pSteering))
			return Elite::BehaviorState::Failure;

		Elite::Vector2 target;
		if (!pBlackboard->GetData("HouseTarget", target))
			return Elite::BehaviorState::Failure;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// Get the closest navmesh point
		const Elite::Vector2 nextTargetPos = pInterface->NavMesh_GetClosestPathPoint(target);

		// Add seek to the navmesh target
		pSteering->AddSeek(nextTargetPos, agentInfo);

		// If there is enough stamina, run
		if (agentInfo.RunMode)
		{
			if (agentInfo.Stamina > 2.0f) pSteering->Run();
		}
		else
		{
			if (agentInfo.Stamina >= 10.0f) pSteering->Run();
		}

		return Elite::BehaviorState::Success;
	}

	// Look at a purge zone in fov
	Elite::BehaviorState LookAtPurgeZone(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return Elite::BehaviorState::Failure;

		Steering* pSteering;
		if (!pBlackboard->GetData("Steering", pSteering))
			return Elite::BehaviorState::Failure;

		std::vector<EntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityFovVec", pEntityVec))
			return Elite::BehaviorState::Failure;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// Search for the purgezone
		for (const EntityInfo& entity : *pEntityVec)
		{
			if (entity.Type == eEntityType::PURGEZONE)
			{
				PurgeZoneInfo zoneInfo;
				pInterface->PurgeZone_GetInfo(entity, zoneInfo);

				// Look at the center of the purge zone
				pSteering->LookAt(zoneInfo.Center);
			}
		}

		return Elite::BehaviorState::Success;
	}

	// Shoot with the prefered gun depending on the situation
	Elite::BehaviorState Shoot(Elite::Blackboard* pBlackboard)
	{
		InventoryManager* pInventory;
		if (!pBlackboard->GetData("Inventory", pInventory))
			return Elite::BehaviorState::Failure;

		std::vector<EntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityFovVec", pEntityVec))
			return Elite::BehaviorState::Failure;

		// Get nr of enemies
		int nrEnemies{};
		for (const EntityInfo& entity : *pEntityVec)
		{
			if (entity.Type == eEntityType::ENEMY) ++nrEnemies;
		}

		// When multiple enemies in fov, shoot with shotgun, else pistol if possible
		if (nrEnemies > 1)
		{
			if (!pInventory->ShootShotgun())
			{
				pInventory->ShootPistol();
			}
		}
		else
		{
			if (!pInventory->ShootPistol())
			{
				pInventory->ShootShotgun();
			}
		}

		// Reset looking for enemy
		pBlackboard->ChangeData("LookForEnemyTimer", 0.0f);
		pBlackboard->ChangeData("LookingForEnemy", false);

		return Elite::BehaviorState::Success;
	}

	// Only rotate from the current enemy
	Elite::BehaviorState TurnToLookForEnemy(Elite::Blackboard* pBlackboard)
	{
		Steering* pSteering;
		if (!pBlackboard->GetData("Steering", pSteering))
			return Elite::BehaviorState::Failure;

		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return Elite::BehaviorState::Failure;

		Elite::Vector2 target;
		if (!pBlackboard->GetData("EntityTarget", target))
			return Elite::BehaviorState::Failure;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// Force the agent to rotate
		pSteering->Rotate(agentInfo.MaxAngularSpeed);

		float lookingForEnemyTimer;
		if (!pBlackboard->GetData("LookForEnemyTimer", lookingForEnemyTimer))
			return Elite::BehaviorState::Failure;

		float deltaTime;
		if (!pBlackboard->GetData("DeltaTime", deltaTime))
			return Elite::BehaviorState::Failure;

		// Decrement the enemylook timer with deltatime
		lookingForEnemyTimer -= deltaTime;

		// Store the enemy look timer
		pBlackboard->ChangeData("LookForEnemyTimer", lookingForEnemyTimer);

		// If the look enemy timer is equal or less then zero, disable looking for an enemy
		if (lookingForEnemyTimer <= 0)
		{
			pBlackboard->ChangeData("LookingForEnemy", false);
		}

		return Elite::BehaviorState::Success;
	}

	// Flee and rotate from the current enemy
	Elite::BehaviorState LookForEnemy(Elite::Blackboard* pBlackboard)
	{
		Steering* pSteering;
		if (!pBlackboard->GetData("Steering", pSteering))
			return Elite::BehaviorState::Failure;

		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return Elite::BehaviorState::Failure;

		Elite::Vector2 target;
		if (!pBlackboard->GetData("EntityTarget", target))
			return Elite::BehaviorState::Failure;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// Force the agent to rotate
		pSteering->Rotate(agentInfo.MaxAngularSpeed);
		// Flee from target
		pSteering->AddFlee(target, agentInfo);

		float lookingForEnemyTimer;
		if (!pBlackboard->GetData("LookForEnemyTimer", lookingForEnemyTimer))
			return Elite::BehaviorState::Failure;

		float deltaTime;
		if (!pBlackboard->GetData("DeltaTime", deltaTime))
			return Elite::BehaviorState::Failure;

		// Decrement the enemylook timer with deltatime
		lookingForEnemyTimer -= deltaTime;

		// Store the enemy look timer
		pBlackboard->ChangeData("LookForEnemyTimer", lookingForEnemyTimer);

		// If the look enemy timer is equal or less then zero, disable looking for an enemy
		if (lookingForEnemyTimer <= 0)
		{
			pBlackboard->ChangeData("LookingForEnemy", false);
		}

		return Elite::BehaviorState::Success;
	}

	// Stand still
	Elite::BehaviorState StandStill(Elite::Blackboard* pBlackboard)
	{
		Steering* pSteering;
		if (!pBlackboard->GetData("Steering", pSteering))
			return Elite::BehaviorState::Failure;

		// Reset the steering
		pSteering->Reset();

		return Elite::BehaviorState::Success;
	}

	// Pick up the current loot
	Elite::BehaviorState PickUpLoot(Elite::Blackboard* pBlackboard)
	{
		InventoryManager* pInventory;
		if (!pBlackboard->GetData("Inventory", pInventory))
			return Elite::BehaviorState::Failure;

		EntityInfo curLoot;
		if (!pBlackboard->GetData("CurLoot", curLoot))
			return Elite::BehaviorState::Failure;

		std::vector<FoundEntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityAllVec", pEntityVec))
			return Elite::BehaviorState::Failure;
		
		// Try to pick up current loot
		if (pInventory->PickUpEntity(curLoot))
		{
			// Remove the entity from the entity list
			for (size_t i{}; i < pEntityVec->size(); ++i)
			{
				const FoundEntityInfo& entity{ (*pEntityVec)[i] };
				if (entity.Location.DistanceSquared(curLoot.Location) < 0.2f)
				{
					(*pEntityVec)[i] = (*pEntityVec)[pEntityVec->size() - 1];
					pEntityVec->pop_back();
					break;
				}
			}

			std::cout << "Picked up item\n";

			return Elite::BehaviorState::Success;
		}

		return Elite::BehaviorState::Failure;
	}

	// Pick up the current loot and rearrange the inventory
	Elite::BehaviorState PickUpLootAndRearrangeInventory(Elite::Blackboard* pBlackboard)
	{
		InventoryManager* pInventory;
		if (!pBlackboard->GetData("Inventory", pInventory))
			return Elite::BehaviorState::Failure;

		EntityInfo curLoot;
		if (!pBlackboard->GetData("CurLoot", curLoot))
			return Elite::BehaviorState::Failure;

		UINT replaceIndex;
		if (!pBlackboard->GetData("ReplaceIndex", replaceIndex))
			return Elite::BehaviorState::Failure;

		std::vector<FoundEntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityAllVec", pEntityVec))
			return Elite::BehaviorState::Failure;

		// Try replacing something in the inventory with the current loot
		if (pInventory->ReplaceItemWithEntity(replaceIndex, curLoot))
		{
			// Remove the entity from the entity list
			for (size_t i{}; i < pEntityVec->size(); ++i)
			{
				const FoundEntityInfo& entity{ (*pEntityVec)[i] };
				if (entity.Location.DistanceSquared(curLoot.Location) < 0.2f)
				{
					(*pEntityVec)[i] = (*pEntityVec)[pEntityVec->size() - 1];
					pEntityVec->pop_back();
					break;
				}
			}

			std::cout << "Picked up item\n";

			return Elite::BehaviorState::Success;
		}

		return Elite::BehaviorState::Failure;
	}

	// Remember the current loot
	Elite::BehaviorState RememberCurrentLoot(Elite::Blackboard* pBlackboard)
	{
		EntityInfo curLoot;
		if (!pBlackboard->GetData("CurLoot", curLoot))
			return Elite::BehaviorState::Failure;

		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return Elite::BehaviorState::Failure;

		std::vector<FoundEntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityAllVec", pEntityVec))
			return Elite::BehaviorState::Failure;

		// Loop over already remembered items and if it contains the current item, return
		for (const EntityInfo& entity : *pEntityVec)
		{
			if (entity.Location.DistanceSquared(curLoot.Location) < 0.2f)
			{
				return Elite::BehaviorState::Failure;
			}
		}

		ItemInfo itemInfo;
		if (!pInterface->Item_GetInfo(curLoot, itemInfo))
			return Elite::BehaviorState::Failure;

		// Create a new foundEntity object
		FoundEntityInfo foundEntity{};
		foundEntity.Location = curLoot.Location;
		foundEntity.Type = curLoot.Type;
		foundEntity.itemType = itemInfo.Type;

		// Store the found entity in the entity container
		pEntityVec->push_back(foundEntity);

		std::cout << "Remembering this item\n";

		return Elite::BehaviorState::Success;
	}

	// Set the house target to a corner of the house
	Elite::BehaviorState SetTargetToCorner(Elite::Blackboard* pBlackboard)
	{
		CurrentHouse curHouse;
		if (!pBlackboard->GetData("CurHouse", curHouse))
			return Elite::BehaviorState::Failure;

		Elite::Vector2 target;
		if (!pBlackboard->GetData("HouseTarget", target))
			return Elite::BehaviorState::Failure;

		Elite::Vector2 entityTarget;
		if (!pBlackboard->GetData("EntityTarget", entityTarget))
			return Elite::BehaviorState::Failure;

		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return Elite::BehaviorState::Failure;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// Should we change the target
		bool changeCorner{};

		constexpr float distanceEpsilon{ 2.0f };
		if (target.DistanceSquared(curHouse.Center) < distanceEpsilon)	// Just entered the house
		{
			changeCorner = true;
		}
		else if (entityTarget.DistanceSquared(agentInfo.Position) < distanceEpsilon)	// Just picked up an item
		{
			changeCorner = true;
		}
		else if(target.DistanceSquared(agentInfo.Position) < distanceEpsilon) // Got close to a corner so it got reset 
		{
			changeCorner = true;

			// Increment the corneridx
			++curHouse.curCornerIndex;
			pBlackboard->ChangeData("CurHouse", curHouse);
		}

		// If we should not change corner, do nothing
		if (!changeCorner) return Elite::BehaviorState::Success;

		// The current corner to move to
		Elite::Vector2 curCorner{};

		// Set the corner position to the right position
		switch (curHouse.curCornerIndex)
		{
		case 0:
		{
			curCorner =
			{
				curHouse.Center.x - curHouse.Size.x / 4.0f,
				curHouse.Center.y + curHouse.Size.y / 4.0f
			};
			break;
		}
		case 1:
		{
			curCorner =
			{
				curHouse.Center.x + curHouse.Size.x / 4.0f,
				curHouse.Center.y - curHouse.Size.y / 4.0f
			};
			break;
		}
		case 2:
		{
			curCorner =
			{
				curHouse.Center.x + curHouse.Size.x / 4.0f,
				curHouse.Center.y + curHouse.Size.y / 4.0f
			};
			break;
		}
		case 3:
		{
			curCorner =
			{
				curHouse.Center.x - curHouse.Size.x / 4.0f,
				curHouse.Center.y - curHouse.Size.y / 4.0f
			};
			break;
		}
		case 4:
		{
			std::cout << "Finished looting house\n";

			return Elite::BehaviorState::Failure;
		}
		}

		// Store the new corner in the house target
		pBlackboard->ChangeData("HouseTarget", curCorner);

		return Elite::BehaviorState::Success;
	}

	// Add the current house to the list of houses
	Elite::BehaviorState AddHouse(Elite::Blackboard* pBlackboard)
	{
		WorldExplorer* pExplorer;
		if (!pBlackboard->GetData("Explorer", pExplorer))
			return Elite::BehaviorState::Failure;

		CurrentHouse curHouse;
		if (!pBlackboard->GetData("CurHouse", curHouse))
			return Elite::BehaviorState::Failure;

		std::vector<HouseInfo>* pHousesVec;
		if (!pBlackboard->GetData("HouseAllVec", pHousesVec))
			return Elite::BehaviorState::Failure;

		// Add the current house to the house container
		pHousesVec->push_back(static_cast<HouseInfo>(curHouse));

		// If the agent is not revisiting buildings, add it the currenthouse as an explore tile
		if (!pExplorer->IsRevisitingBuildings())
		{
			pExplorer->AddExploreTile(curHouse.Center);
		}

		std::cout << "Found a new house\n";
		return Elite::BehaviorState::Success;
	}

	// Explore
	Elite::BehaviorState Explore(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return Elite::BehaviorState::Failure;

		WorldExplorer* pExplorer;
		if (!pBlackboard->GetData("Explorer", pExplorer))
			return Elite::BehaviorState::Failure;

		Steering* pSteering;
		if (!pBlackboard->GetData("Steering", pSteering))
			return Elite::BehaviorState::Failure;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// Get the closest undiscovered tile on the grid
		const Elite::Vector2 checkpointLocation{ pExplorer->GetNearestUndiscoveredGrid(agentInfo.Position) };

		// If the agent is done exploring, do nothing
		if (pExplorer->IsDoneExploring()) return Elite::BehaviorState::Failure;

		// Get the closest point on the nav mesh path
		const Elite::Vector2 nextTargetPos = pInterface->NavMesh_GetClosestPathPoint(checkpointLocation);

		// Seek to the target
		pSteering->AddSeek(nextTargetPos, agentInfo);

		// Calculate the angle between the velocity and the orientation of the agent
		const Elite::Vector2 lookDirection{ cosf(agentInfo.Orientation), sinf(agentInfo.Orientation) };
		const float angleDelta{ atan2f(agentInfo.LinearVelocity.Cross(lookDirection), agentInfo.LinearVelocity.Dot(lookDirection)) };

		// Swap angular velocity if the angle is wider then 45°
		// Else keep the same angular velocity as before
		constexpr float rotateThreshold{ 45.0f * TO_RADIANS };
		float angularVelocity{};
		if (angleDelta < -rotateThreshold)
		{
			angularVelocity = agentInfo.MaxAngularSpeed;
		}
		else if (angleDelta > rotateThreshold)
		{
			angularVelocity = -agentInfo.MaxAngularSpeed;
		}
		else
		{
			angularVelocity = agentInfo.AngularVelocity;
		}
		// Apply the angular velocity to the steering
		pSteering->Rotate(angularVelocity);
		
		// If there is enough stamina, run
		if (agentInfo.RunMode)
		{
			if (agentInfo.Stamina > 2.0f) pSteering->Run();
		}
		else
		{
			if (agentInfo.Stamina >= 10.0f) pSteering->Run();
		}

		// Reset the entity and house targets
		pBlackboard->ChangeData("EntityTarget", Elite::Vector2{});
		pBlackboard->ChangeData("HouseTarget", Elite::Vector2{});

		// Return success
		return Elite::BehaviorState::Success;
	}

	// Add all known houses to the world exploration to be revisited
	Elite::BehaviorState RevisitHouses(Elite::Blackboard* pBlackboard)
	{
		std::vector<HouseInfo>* pHouseVec;
		if (!pBlackboard->GetData("HouseAllVec", pHouseVec))
			return Elite::BehaviorState::Failure;

		WorldExplorer* pExplorer;
		if (!pBlackboard->GetData("Explorer", pExplorer))
			return Elite::BehaviorState::Failure;

		// Reset the explorer
		pExplorer->Reset();

		// Add every house as a revisit tile
		for (const HouseInfo& house : *pHouseVec)
		{
			pExplorer->AddRevisitTile(house.Center);
		}

		// Clear the house container
		pHouseVec->clear();

		std::vector<FoundEntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityAllVec", pEntityVec))
			return Elite::BehaviorState::Failure;

		// Remove every item that is a garbage from the remembered item container
		pEntityVec->erase(
			std::remove_if(
				pEntityVec->begin(), 
				pEntityVec->end(), 
				[](const FoundEntityInfo& entity) { return entity.itemType == eItemType::GARBAGE; }),
			pEntityVec->end());

		return Elite::BehaviorState::Success;
	}
}

namespace BT_Conditions
{
	// Is agent right in front of the agent
	bool IsEnemyInFront(Elite::Blackboard* pBlackboard)
	{
		std::vector<EntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityFovVec", pEntityVec))
			return false;

		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return false;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// Calculate the look direction
		const Elite::Vector2 lookDir{ cosf(agentInfo.Orientation), sinf(agentInfo.Orientation) };

		// The threshold when enemies are in front of the player
		constexpr float dotThreshold{ 0.002f };

		// For each enemy
		for (const EntityInfo& entity : *pEntityVec)
		{
			if (entity.Type == eEntityType::ENEMY)
			{
				// Calculate the vector from agent to enemy
				Elite::Vector2 dir{ entity.Location - agentInfo.Position };
				dir.Normalize();
				
				// If the dot product between the agentEnemyVector and the lookdirection is close to 1, return true
				if (dir.Dot(lookDir) > 1.0f - dotThreshold) return true;
			}
		}

		return false;
	}

	// Sees enemy?
	bool IsEnemyInFOV(Elite::Blackboard* pBlackboard)
	{
		std::vector<EntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityFovVec", pEntityVec))
			return false;

		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return false;

		AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// For each enemy
		for (const EntityInfo& entity : *pEntityVec)
		{
			if (entity.Type == eEntityType::ENEMY)
			{
				// Set the first enemy as entity target
				pBlackboard->ChangeData("EntityTarget", entity.Location);
				return true;
			}
		}

		return false;
	}

	// Is the agent looking for an enemy?
	bool IsLookingForEnemy(Elite::Blackboard* pBlackboard)
	{
		bool isLookingForEnemy;
		if (!pBlackboard->GetData("LookingForEnemy", isLookingForEnemy))
			return false;

		return isLookingForEnemy;
	}

	// Is the agent hit by an enemy?
	bool IsHitByEnemy(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return false;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// If the agent was bitten
		if (agentInfo.WasBitten)
		{
			// Calculate the look direction
			const Elite::Vector2 lookDir{ cosf(agentInfo.Orientation), sinf(agentInfo.Orientation) };

			pBlackboard->ChangeData("EntityTarget", agentInfo.Position - lookDir);

			// Reset the looking for enemy timer
			pBlackboard->ChangeData("LookingForEnemy", true);
			constexpr float lookAroundTimer{ 2.0f };
			pBlackboard->ChangeData("LookForEnemyTimer", lookAroundTimer);
		}

		return agentInfo.WasBitten;
	}

	// Is a gun in the inventory?
	bool IsGunInInventory(Elite::Blackboard* pBlackboard)
	{
		InventoryManager* pInventory;
		if (!pBlackboard->GetData("Inventory", pInventory))
			return false;

		return pInventory->HasPistol() || pInventory->HasShotgun();
	}

	// Sees a purge zone right in front of the agent?
	bool IsPurgeZoneInFront(Elite::Blackboard* pBlackboard)
	{
		std::vector<EntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityFovVec", pEntityVec))
			return false;

		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return false;

		// How close the agent should be to the purge zone return true
		constexpr float inFrontDistance{ 4.0f };

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// Search for the purgezone
		for (const EntityInfo& entity : *pEntityVec)
		{
			if (entity.Type == eEntityType::PURGEZONE)
			{
				PurgeZoneInfo zoneInfo;
				pInterface->PurgeZone_GetInfo(entity, zoneInfo);

				// Calculate the vector from center of purgezone to the agent
				Elite::Vector2 centerPlayer{ agentInfo.Position - zoneInfo.Center };
				// Get the distance between the player and the purgezone center
				const float playerDistanceFromZone{ centerPlayer.Normalize() };
				// Calculate the radius that the agent should stay away from
				const float runRadius{ zoneInfo.Radius + 4.0f };

				// If the agent is outside the runradius, stop the loop
				if (playerDistanceFromZone > runRadius) break;

				return true;
			}
		}

		return false;
	}

	// Is agent inside a purge zone?
	bool IsInsidePurgeZone(Elite::Blackboard* pBlackboard)
	{
		std::vector<EntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityFovVec", pEntityVec))
			return false;

		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return false;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// How close the agent should be to the purge zone return true
		constexpr float inFrontDistance{ 5.0f };

		// Search for a purge zone
		for (const EntityInfo& entity : *pEntityVec)
		{
			if (entity.Type == eEntityType::PURGEZONE)
			{
				PurgeZoneInfo zoneInfo;
				pInterface->PurgeZone_GetInfo(entity, zoneInfo);

				// Calculate the vector from center of purgezone to the agent
				Elite::Vector2 centerPlayer{ agentInfo.Position - zoneInfo.Center };
				// Get the distance between the player and the purgezone center
				const float playerDistanceFromZone{ centerPlayer.Normalize() };

				// If the player is outside the radius, stop the loop
				if (playerDistanceFromZone > zoneInfo.Radius) break;

				// Calculate the radius that the agent should stay away from
				const float runRadius{ zoneInfo.Radius + inFrontDistance };
				// Calculate the point where to run to
				const Elite::Vector2 runPoint{ zoneInfo.Center + centerPlayer * runRadius };

				// Apply the runpoint
				pBlackboard->ChangeData("EntityTarget", runPoint);

				return true;
			}
		}

		return false;
	}

	// Can pick up loot?
	bool IsLootInRange(Elite::Blackboard* pBlackboard)
	{
		std::vector<EntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityFovVec", pEntityVec))
			return false;

		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return false;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// Has found loot
		bool foundLoot{};
		// The closest food entity
		EntityInfo closestLoot{};
		// The closest distance
		float closestDistance{ FLT_MAX };

		// For each entity
		for (const EntityInfo& entity : *pEntityVec)
		{
			// If the entity is not an agent, continue to the next entity
			if (entity.Type != eEntityType::ITEM) continue;

			// Calculate the distance between entity and agent
			const float curDistance{ agentInfo.Position.DistanceSquared(entity.Location) };

			// If the current distance is smaller then the current closest distance
			if (curDistance < closestDistance)
			{
				// Store the current entity and distance
				closestLoot = entity;
				closestDistance = curDistance;
				foundLoot = true;
			}
		}

		// If the agent does not see food, return
		if (!foundLoot) return false;

		// Is loot in grab range
		const bool isLootInRange{ closestDistance < agentInfo.GrabRange * agentInfo.GrabRange };

		// If the loot is in grab range, store the current loot
		if (isLootInRange) pBlackboard->ChangeData("CurLoot", closestLoot);

		return isLootInRange;
	}

	// Is the inventory full?
	bool IsInventoryNotFull(Elite::Blackboard* pBlackboard)
	{
		InventoryManager* pInventory;
		if (!pBlackboard->GetData("Inventory", pInventory))
			return false;

		return !pInventory->IsInventoryFull();
	}

	// Is a better inventory possible with the current item?
	bool IsBetterInventoryPossible(Elite::Blackboard* pBlackboard)
	{
		std::vector<FoundEntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityAllVec", pEntityVec))
			return false;

		InventoryManager* pInventory;
		if (!pBlackboard->GetData("Inventory", pInventory))
			return false;

		EntityInfo curLoot;
		if (!pBlackboard->GetData("CurLoot", curLoot))
			return false;

		const UINT indexToReplace{ pInventory->IsBetterInventoryPossible(curLoot, *pEntityVec) };

		if (indexToReplace != 10) // 10 is error code
		{
			pBlackboard->ChangeData("ReplaceIndex", indexToReplace);
		}

		return indexToReplace != 10;
	}

	// Sees loot?
	bool IsLootInFov(Elite::Blackboard* pBlackboard)
	{
		std::vector<EntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityFovVec", pEntityVec))
			return false;

		// For each entity in fov
		for (const EntityInfo& entity : *pEntityVec)
		{
			// If the entity is an item, return true
			if (entity.Type == eEntityType::ITEM) return true;
		}

		return false;
	}

	// Is current loot already seen?
	bool IsLootAlreadySeen(Elite::Blackboard* pBlackboard)
	{
		std::vector<EntityInfo>* pEntityFovVec;
		if (!pBlackboard->GetData("EntityFovVec", pEntityFovVec))
			return false;

		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return false;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// Sort all the entities in fov by distance from agent
		std::sort(
			pEntityFovVec->begin(),
			pEntityFovVec->end(),
			[&](const EntityInfo& first, const EntityInfo& second) 
			{
				const float firstDist{ first.Location.DistanceSquared(agentInfo.Position) };
				const float secondDist{ second.Location.DistanceSquared(agentInfo.Position) };

				return firstDist < secondDist;
			}
		);

		std::vector<FoundEntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityAllVec", pEntityVec))
			return false;

		// For each entity in fov
		for (const EntityInfo& fovEntity : *pEntityFovVec)
		{
			// If the entity is not an item, continue to the next entity
			if (fovEntity.Type != eEntityType::ITEM) continue;

			bool alreadySeen{};
			// For each remembered item
			for (const FoundEntityInfo& entity : *pEntityVec)
			{
				// If the locations of the items overlap, set already seen to true
				if (fovEntity.Location.DistanceSquared(entity.Location) < 0.2f)
				{
					alreadySeen = true;
					break;
				}
			}

			// If the item is already seen, continue to the next item
			if (alreadySeen) continue;

			// Store the item in entity target
			pBlackboard->ChangeData("EntityTarget", fovEntity.Location);
			return false;
		}

		return true;
	}

	// Sees house?
	bool IsNewHouseInFOV(Elite::Blackboard* pBlackboard)
	{
		std::vector<HouseInfo>* pHouseVec;
		if (!pBlackboard->GetData("HouseFovVec", pHouseVec))
			return false;

		std::vector<HouseInfo>* pSeenHousesVec;
		if (!pBlackboard->GetData("HouseAllVec", pSeenHousesVec))
			return false;

		// For each house in fov
		for (const HouseInfo& house : *pHouseVec)
		{
			bool newHouse{ true };

			// For each already seen house
			for (const HouseInfo& visitedHouse : *pSeenHousesVec)
			{
				// If the locations of the houses match, set new house to false
				if (house.Center.DistanceSquared(visitedHouse.Center) < 0.2f)
				{
					newHouse = false;
				}
			}

			// If the current house is not a new house, continue to the next house
			if (!newHouse) continue;

			// Create a new currenthouse object
			CurrentHouse newHouseInfo{};
			newHouseInfo.Center = house.Center;
			newHouseInfo.Size = house.Size;

			// Store the current house
			pBlackboard->ChangeData("CurHouse", newHouseInfo);

			// Set the house target to the current house
			pBlackboard->ChangeData("HouseTarget", house.Center);
			return true;
		}

		pBlackboard->ChangeData("HouseTarget", Elite::Vector2{});
		pBlackboard->ChangeData("CurHouse", CurrentHouse{});
		return false;
	}

	// Is Moving Towards A House?
	bool IsMovingTowardsHouse(Elite::Blackboard* pBlackboard)
	{
		std::vector<HouseInfo>* pHouseVec;
		if (!pBlackboard->GetData("HouseAllVec", pHouseVec))
			return false;

		Elite::Vector2 target;
		if (!pBlackboard->GetData("HouseTarget", target))
			return false;

		// For each house in memory
		for (const HouseInfo& house : *pHouseVec)
		{
			// If the target matches the location of a house, return true
			if (house.Center.DistanceSquared(target) < 2.0f) return true;
		}


		CurrentHouse curHouse;
		if (!pBlackboard->GetData("CurHouse", curHouse))
			return false;

		// If the current house has a valid size and the corneridx is less then 4, we are still looting a house
		if (curHouse.Size.x > FLT_EPSILON && curHouse.curCornerIndex < 4) return true;

		return false;
	}

	// Is inside house?
	bool IsInsideHouse(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return false;

		CurrentHouse curHouse{};
		if (!pBlackboard->GetData("CurHouse", curHouse))
			return false;

		// If the size of the current house is not valid, return false
		if (curHouse.Size.x < FLT_EPSILON) return false;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		// If the agent is inside the bounds of a house, return true
		if (agentInfo.Position.x > curHouse.Center.x - curHouse.Size.x / 2 &&
			agentInfo.Position.x < curHouse.Center.x + curHouse.Size.x / 2 &&
			agentInfo.Position.y > curHouse.Center.y - curHouse.Size.y / 2 &&
			agentInfo.Position.y < curHouse.Center.y + curHouse.Size.y / 2)
		{
			return true;
		}

		return false;
	}

	// Does the agent remember a needed item?
	bool RemembersNeededItem(Elite::Blackboard* pBlackboard)
	{
		std::vector<FoundEntityInfo>* pEntityVec;
		if (!pBlackboard->GetData("EntityAllVec", pEntityVec))
			return false;

		IExamInterface* pInterface;
		if (!pBlackboard->GetData("Interface", pInterface))
			return false;

		const AgentInfo agentInfo{ pInterface->Agent_GetInfo() };

		InventoryManager* pInventory;
		if (!pBlackboard->GetData("Inventory", pInventory))
			return false;

		constexpr float rangeToLook{ 250.0f };

		pInterface->Draw_Circle(agentInfo.Position, rangeToLook, { 0.0f, 0.0f, 1.0f });

		// Set the needed item to the most needed item
		eItemType neededItem{ eItemType::RANDOM_DROP };
		if (!pInventory->HasFood()) neededItem = eItemType::FOOD;
		else if (!pInventory->HasMedkit()) neededItem = eItemType::MEDKIT;
		else if (!pInventory->HasPistol()) neededItem = eItemType::PISTOL;
		else if (!pInventory->HasShotgun()) neededItem = eItemType::SHOTGUN;

		// If no item is needed, return false
		if (neededItem == eItemType::RANDOM_DROP) return false;

		int curItem{ -1 };
		float closestDistance{ FLT_MAX };

		// For each remembered item
		for (size_t i{}; i < pEntityVec->size(); ++i)
		{
			// Get the current entity
			const FoundEntityInfo& entity{ (*pEntityVec)[i] };

			if (entity.itemType != neededItem) continue;

			// Calculate the distance between the agent and the entity
			const float distanceFromItem{ agentInfo.Position.DistanceSquared(entity.Location) };

			// If the item is not food, check if the item is inside the range
			if (entity.itemType != eItemType::FOOD && distanceFromItem > rangeToLook * rangeToLook) continue;

			// If the current distance is smaller then the currest smallest distance
			if (distanceFromItem < closestDistance)
			{
				// Store the current item and distance
				closestDistance = distanceFromItem;
				curItem = static_cast<int>(i);
			}
		}

		// If no item has been found, return false
		if (curItem < 0) return false;

		// Store the item location in the entity target
		pBlackboard->ChangeData("EntityTarget", (*pEntityVec)[curItem].Location);
		return true;
	}
}

#endif