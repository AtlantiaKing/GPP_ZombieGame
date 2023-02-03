#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "ExtendedStructs.h"
#include "EBehaviorTree.h"

class IBaseInterface;
class IExamInterface;
class WorldExplorer;
class InventoryManager;
class Steering;

class Plugin : public IExamPlugin
{
public:
	Plugin() {};
	virtual ~Plugin() {};

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void Update(float dt) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:
	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	WorldExplorer* m_pExplorer{};
	InventoryManager* m_pInventoryManager{};
	Steering* m_pSteering{};

	std::vector<HouseInfo> m_Houses{};
	std::vector<HouseInfo> m_HousesInFOV{};

	std::vector<EntityInfo> m_EntitiesInFOV{};
	std::vector<FoundEntityInfo> m_Entities{};

	Elite::Vector2 m_Target = {};
	bool m_CanRun = false; //Demo purpose
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose

	UINT m_InventorySlot = 0;

	Elite::BehaviorTree* m_DecisionTree{};

	std::vector<HouseInfo> GetHousesInFOV() const;
	std::vector<EntityInfo> GetEntitiesInFOV() const;
};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}