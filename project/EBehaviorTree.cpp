//=== General Includes ===
#include "stdafx.h"
#include "EBehaviorTree.h"
using namespace Elite;

//-----------------------------------------------------------------
// BEHAVIOR TREE COMPOSITES (IBehavior)
//-----------------------------------------------------------------
#pragma region COMPOSITES
//SELECTOR
BehaviorState BehaviorSelector::Execute(Blackboard* pBlackBoard)
{
	// Loop over all children in m_ChildBehaviors
	for (auto& child : m_ChildBehaviors)
	{
		//Every Child: Execute and store the result in m_CurrentState
		m_CurrentState = child->Execute(pBlackBoard);

		//Check the currentstate and apply the selector Logic:
		switch (m_CurrentState)
		{
		case BehaviorState::Success:
		case BehaviorState::Running:
			return m_CurrentState;
		case BehaviorState::Failure:
			continue;
		}
	}

	//All children failed
	m_CurrentState = BehaviorState::Failure;
	return m_CurrentState;
}
//SEQUENCE
BehaviorState BehaviorSequence::Execute(Blackboard* pBlackBoard)
{
	// Loop over all children in m_ChildBehaviors
	for (auto& child : m_ChildBehaviors)
	{
		//Every Child: Execute and store the result in m_CurrentState
		m_CurrentState = child->Execute(pBlackBoard);

		//Check the currentstate and apply the selector Logic:
		switch (m_CurrentState)
		{
		case BehaviorState::Running:
		case BehaviorState::Failure:
			return m_CurrentState;
		case BehaviorState::Success:
			continue;
		}
	}

	//All children succeeded 
	m_CurrentState = BehaviorState::Success;
	return m_CurrentState;
}
//PARTIAL SEQUENCE
BehaviorState BehaviorPartialSequence::Execute(Blackboard* pBlackBoard)
{
	while (m_CurrentBehaviorIndex < m_ChildBehaviors.size())
	{
		m_CurrentState = m_ChildBehaviors[m_CurrentBehaviorIndex]->Execute(pBlackBoard);
		switch (m_CurrentState)
		{
		case BehaviorState::Failure:
			m_CurrentBehaviorIndex = 0;
			return m_CurrentState;
		case BehaviorState::Success:
			++m_CurrentBehaviorIndex;
			m_CurrentState = BehaviorState::Running;
			return m_CurrentState;
		case BehaviorState::Running:
			return m_CurrentState;
		}
	}

	m_CurrentBehaviorIndex = 0;
	m_CurrentState = BehaviorState::Success;
	return m_CurrentState;
}
//INVERTOR
BehaviorState BehaviorInvertor::Execute(Blackboard* pBlackBoard)
{
	// Loop over all children in m_ChildBehaviors
	auto& child{ m_ChildBehaviors[0] };

	m_CurrentState = child->Execute(pBlackBoard);
	switch (m_CurrentState)
	{
	case BehaviorState::Failure:
		return BehaviorState::Success;
	case BehaviorState::Success:
		return BehaviorState::Failure;
		return m_CurrentState;
	case BehaviorState::Running:
		return BehaviorState::Running;
	}

	//All children failed
	m_CurrentState = BehaviorState::Failure;
	return m_CurrentState;
}
#pragma endregion
//-----------------------------------------------------------------
// BEHAVIOR TREE CONDITIONAL (IBehavior)
//-----------------------------------------------------------------
BehaviorState BehaviorConditional::Execute(Blackboard* pBlackBoard)
{
	if (m_fpConditional == nullptr)
		return BehaviorState::Failure;

	switch (m_fpConditional(pBlackBoard))
	{
	case true:
		m_CurrentState = BehaviorState::Success;
		return m_CurrentState;
	case false:
		m_CurrentState = m_CurrentState = BehaviorState::Failure;
		return m_CurrentState;
	}

	return BehaviorState::Failure;
}
//-----------------------------------------------------------------
// BEHAVIOR TREE ACTION (IBehavior)
//-----------------------------------------------------------------
BehaviorState BehaviorAction::Execute(Blackboard* pBlackBoard)
{
	if (m_fpAction == nullptr)
		return BehaviorState::Failure;

	m_CurrentState = m_fpAction(pBlackBoard);
	return m_CurrentState;
}