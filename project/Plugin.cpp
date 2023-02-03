#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"
#include "WorldExplorer.h"
#include "Behaviors.h"
#include "Steering.h"

using namespace std;

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "AtlantiaKing";
	info.Student_FirstName = "Sander";
	info.Student_LastName = "De Keukelaere";
	info.Student_Class = "2DAE15";

	const WorldInfo worldInfo{ m_pInterface->World_GetInfo() };
	m_pExplorer = new WorldExplorer{ worldInfo };

	m_pInventoryManager = new InventoryManager{ m_pInterface };

	m_pSteering = new Steering{};

	Elite::Blackboard* pBlackboard = new Elite::Blackboard();
	pBlackboard->AddData("Interface", m_pInterface);
	pBlackboard->AddData("Explorer", m_pExplorer);
	pBlackboard->AddData("Inventory", m_pInventoryManager);
	pBlackboard->AddData("HouseFovVec", &m_HousesInFOV);
	pBlackboard->AddData("HouseAllVec", &m_Houses);
	pBlackboard->AddData("EntityFovVec", &m_EntitiesInFOV);
	pBlackboard->AddData("EntityAllVec", &m_Entities);
	pBlackboard->AddData("CurHouse", CurrentHouse{});
	pBlackboard->AddData("CurLoot", EntityInfo{});
	pBlackboard->AddData("HouseTarget", Elite::Vector2{});
	pBlackboard->AddData("EntityTarget", Elite::Vector2{});
	pBlackboard->AddData("Steering", m_pSteering);
	pBlackboard->AddData("ReplaceIndex", UINT(0));
	pBlackboard->AddData("LookingForEnemy", false);
	pBlackboard->AddData("LookForEnemyTimer", 0.0f);
	pBlackboard->AddData("DeltaTime", 0.0f);
	
	Elite::BehaviorTree* pBehaviorTree
	{
		new Elite::BehaviorTree
		{
			pBlackboard,
			new Elite::BehaviorSelector
			{
				{
					// Try to shoot enemies
					new Elite::BehaviorSequence
					{
						{
							new Elite::BehaviorConditional{ BT_Conditions::IsEnemyInFront },
							new Elite::BehaviorConditional{ BT_Conditions::IsGunInInventory },
							new Elite::BehaviorAction{ BT_Actions::Shoot }
						}
					},
					// Try to spot enemies
					new Elite::BehaviorSequence
					{
						{
							new Elite::BehaviorConditional{ BT_Conditions::IsEnemyInFOV },
							new Elite::BehaviorConditional{ BT_Conditions::IsGunInInventory },
							new Elite::BehaviorAction{ BT_Actions::AddToFleeAndLookAt },
							new Elite::BehaviorConditional{ BT_Conditions::IsInsidePurgeZone },
							new Elite::BehaviorAction{ BT_Actions::AddToEntitySeek },
						}
					},
					// Try to look at enemies
					new Elite::BehaviorSequence
					{
						{
							new Elite::BehaviorSelector
							{
								{
									new Elite::BehaviorConditional{ BT_Conditions::IsLookingForEnemy },
									new Elite::BehaviorConditional{ BT_Conditions::IsHitByEnemy }
								}
							},
							new Elite::BehaviorConditional{ BT_Conditions::IsGunInInventory },
							new Elite::BehaviorInvertor 
							{
								new Elite::BehaviorConditional{ BT_Conditions::IsInsidePurgeZone },
							},
							new Elite::BehaviorSelector
							{
								{
									new Elite::BehaviorSequence
									{
										{
											new Elite::BehaviorConditional{ BT_Conditions::IsPurgeZoneInFront },
											new Elite::BehaviorAction{ BT_Actions::TurnToLookForEnemy }
										}
									},
									new Elite::BehaviorAction{ BT_Actions::LookForEnemy }
								}
							}
						}
					},
					// Try to avoid purge zones
					new Elite::BehaviorSequence
					{
						{
							new Elite::BehaviorConditional{ BT_Conditions::IsPurgeZoneInFront },
							new Elite::BehaviorSelector
							{
								{
									new Elite::BehaviorSequence
									{
										{
											new Elite::BehaviorConditional{ BT_Conditions::IsInsidePurgeZone },
											new Elite::BehaviorAction{ BT_Actions::AddToEntitySeek },
											new Elite::BehaviorAction{ BT_Actions::LookAtPurgeZone }
										}
									},
									new Elite::BehaviorAction{ BT_Actions::StandStill }
								}
							}
						}
					},
					// Try to pick up loot
					new Elite::BehaviorSequence
					{
						{
							new Elite::BehaviorConditional{ BT_Conditions::IsLootInRange },
							new Elite::BehaviorSelector
							{
								{
									new Elite::BehaviorSequence
									{
										{
											new Elite::BehaviorConditional{ BT_Conditions::IsInventoryNotFull },
											new Elite::BehaviorAction{ BT_Actions::PickUpLoot }
										}
									},
									new Elite::BehaviorSequence
									{
										{
											new Elite::BehaviorConditional{ BT_Conditions::IsBetterInventoryPossible },
											new Elite::BehaviorAction{ BT_Actions::PickUpLootAndRearrangeInventory }
										}
									},
									new Elite::BehaviorAction{ BT_Actions::RememberCurrentLoot }
								}
							}
							
						}
					},
					// Try to spot loot
					new Elite::BehaviorSequence
					{
						{
							new Elite::BehaviorConditional{ BT_Conditions::IsLootInFov },
							new Elite::BehaviorInvertor
							{
								new Elite::BehaviorConditional{ BT_Conditions::IsLootAlreadySeen }
							},
							new Elite::BehaviorAction{ BT_Actions::AddToEntitySeek }
						}
					},
					// Move around the building in search of loot
					new Elite::BehaviorSequence
					{
						{
							new Elite::BehaviorConditional{ BT_Conditions::IsInsideHouse },
							new Elite::BehaviorAction{ BT_Actions::SetTargetToCorner },
							new Elite::BehaviorAction{ BT_Actions::AddToHouseSeek }
						}
					},
					// Try moving to house
					new Elite::BehaviorSequence
					{
						{
							new Elite::BehaviorConditional{ BT_Conditions::IsMovingTowardsHouse },
							new Elite::BehaviorAction{ BT_Actions::AddToHouseSeek }
						}
					},
					// Try to spot houses
					new Elite::BehaviorSequence
					{
						{
							new Elite::BehaviorConditional{ BT_Conditions::IsNewHouseInFOV },
							new Elite::BehaviorSequence
							{
								{
									new Elite::BehaviorAction{ BT_Actions::AddToHouseSeek },
									new Elite::BehaviorAction{ BT_Actions::AddHouse }
								}
							}
						}
					},
					// Fill inventory with known items
					new Elite::BehaviorSequence
					{
						{
							 new Elite::BehaviorConditional{ BT_Conditions::RemembersNeededItem },
							 new Elite::BehaviorAction{ BT_Actions::AddToEntitySeek }
						}
					},
					// Fall back to world exploration
					new Elite::BehaviorAction{ BT_Actions::Explore },
					new Elite::BehaviorAction{ BT_Actions::RevisitHouses }
				}
			}
		}
	};

	m_DecisionTree = pBehaviorTree;
}

//Called only once
void Plugin::DllInit()
{
	//Called when the plugin is loaded
}

//Called only once
void Plugin::DllShutdown()
{
	//Called wheb the plugin gets unloaded
	delete m_pExplorer;
}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	srand(static_cast<unsigned int>(time(NULL)));
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be useful to inspect certain behaviors (Default = false)
	params.LevelFile = "GameLevel.gppl";
	params.AutoGrabClosestItem = false; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
	params.StartingDifficultyStage = 1;
	params.InfiniteStamina = false;
	params.SpawnDebugPistol = false;
	params.SpawnDebugShotgun = false;
	params.SpawnPurgeZonesOnMiddleClick = true;
	params.PrintDebugMessages = true;
	params.ShowDebugItemNames = true;
	params.SpawnZombieOnRightClick = true;
	params.Seed = rand();
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	if (m_pInterface->Input_IsMouseButtonUp(Elite::InputMouseButton::eLeft))
	{
		//Update target based on input
		Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
		const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(mouseData.X), static_cast<float>(mouseData.Y));
		m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Space))
	{
		m_CanRun = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Left))
	{
		m_AngSpeed -= Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Right))
	{
		m_AngSpeed += Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_G))
	{
		m_GrabItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_U))
	{
		m_UseItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_R))
	{
		m_RemoveItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_Space))
	{
		m_CanRun = false;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Delete))
	{
		m_pInterface->RequestShutdown();
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_P))
	{
		if (m_InventorySlot > 0)
			--m_InventorySlot;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_L))
	{
		if (m_InventorySlot < 4)
			++m_InventorySlot;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Q))
	{
		ItemInfo info = {};
		m_pInterface->Inventory_GetItem(m_InventorySlot, info);
		std::cout << (int)info.Type << std::endl;
	}
}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{	
	// Store the current FOV data
	m_HousesInFOV = GetHousesInFOV();
	m_EntitiesInFOV = GetEntitiesInFOV();

	//Use the Interface (IAssignmentInterface) to 'interface' with the AI_Framework
	const AgentInfo agentInfo = m_pInterface->Agent_GetInfo();

	// Update the World Explorer
	m_pExplorer->Update(agentInfo.Position, agentInfo.Orientation);

	// Update the Inventory
	m_pInventoryManager->Update(agentInfo.Health, agentInfo.Energy);

	// Update the decision making
	m_DecisionTree->Update(dt);

	// Retrieve the steering output from the decision tree
	auto steering = m_pSteering->Update(agentInfo);

	//INVENTORY USAGE DEMO
	//********************

	if (m_UseItem)
	{
		//Use an item (make sure there is an item at the given inventory slot)
		m_pInterface->Inventory_UseItem(m_InventorySlot);
	}

	if (m_RemoveItem)
	{
		//Remove an item from a inventory slot
		m_pInterface->Inventory_RemoveItem(m_InventorySlot);
	}

//@End (Demo Purposes)
	m_GrabItem = false; //Reset State
	m_UseItem = false;
	m_RemoveItem = false;

	return steering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });

	m_pExplorer->DrawDebug(m_pInterface);

	for (const EntityInfo& entity : m_Entities)
	{
		m_pInterface->Draw_SolidCircle(entity.Location, 1.0f, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0);
	}
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}

vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}
