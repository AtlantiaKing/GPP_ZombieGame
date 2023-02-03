#include "stdafx.h"
#include "Steering.h"

void Steering::AddSeek(const Elite::Vector2& target, const AgentInfo& agent)
{
    const Elite::Vector2 dir{ target - agent.Position };
    m_SeekDir += dir.GetNormalized();
}

void Steering::AddFlee(const Elite::Vector2& target, const AgentInfo& agent)
{
    const Elite::Vector2 dir{ agent.Position - target };
    m_FleeDir += dir.GetNormalized();
}

void Steering::LookAt(const Elite::Vector2& target)
{
    m_IsLookingAt = true;
    m_LookAtTarget = target;
}

void Steering::Rotate(float velocity)
{
    m_AngularVelocity = velocity;
}

void Steering::Run()
{
    m_IsRunning = true;
}

void Steering::Reset()
{
    m_SeekDir.x = 0.0f;
    m_SeekDir.y = 0.0f;
    m_FleeDir.x = 0.0f;
    m_FleeDir.y = 0.0f;
    m_IsLookingAt = false;
    m_IsRunning = false;
    m_AngularVelocity = 0.0f;
}

SteeringPlugin_Output Steering::Update(const AgentInfo& agent)
{
    SteeringPlugin_Output steering{};

    // Calculate the combined flee and seek
    steering.LinearVelocity += m_FleeDir + m_SeekDir;
    steering.LinearVelocity.Normalize();
    steering.LinearVelocity *= agent.MaxLinearSpeed;

    if (m_IsLookingAt) // If lookat is enabled
    {
        // Calcualte the direction between the player and the target
        const Elite::Vector2 dir{ Elite::Vector2{ m_LookAtTarget - agent.Position }.GetNormalized() };
        // Calcualte the look direction
        const Elite::Vector2 lookDir{ cosf(agent.Orientation), sinf(agent.Orientation) };

        // Set the angular velocity depending on the angle beetween the two vectors
        steering.AngularVelocity = lookDir.Cross(dir) < 0.0f ? -agent.MaxAngularSpeed : agent.MaxAngularSpeed;

        // Disable auto orient
        steering.AutoOrient = false;
    }
    else if (abs(m_AngularVelocity) > FLT_EPSILON) // If a forced angular velocity is set
    {
        // Disable auto orient
        steering.AutoOrient = false;

        // Apply the forced angular velocity
        steering.AngularVelocity = m_AngularVelocity;
    }

    // Apply the run mode
    steering.RunMode = m_IsRunning;

    // Reset the member variables for the next frame
    Reset();

    // Return the calculated steering
    return steering;
}
