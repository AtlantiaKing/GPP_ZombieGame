#pragma once
#include <Exam_HelperStructs.h>
class Steering final
{
public:
	void AddSeek(const Elite::Vector2& target, const AgentInfo& agent);
	void AddFlee(const Elite::Vector2& target, const AgentInfo& agent);
	void LookAt(const Elite::Vector2& target);
	void Rotate(float velocity);
	void Run();
	void Reset();

	SteeringPlugin_Output Update(const AgentInfo& agent);
private:
	Elite::Vector2 m_SeekDir{};
	Elite::Vector2 m_FleeDir{};
	bool m_IsLookingAt{};
	Elite::Vector2 m_LookAtTarget{};
	float m_AngularVelocity{};
	bool m_IsRunning{};
};

