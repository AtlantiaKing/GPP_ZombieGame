#pragma once
#include <Exam_HelperStructs.h>
#include <IExamInterface.h>

class WorldExplorer final
{
public:
	WorldExplorer(const WorldInfo& worldInfo);

	void Update(const Elite::Vector2& playerPosition, float orientation);

	void DrawDebug(IExamInterface* pInterface) const;
	Elite::Vector2 GetNearestUndiscoveredGrid(const Elite::Vector2& playerPosition);
	void AddExploreTile(const Elite::Vector2& position);
	void AddRevisitTile(const Elite::Vector2& position);
	bool IsDoneExploring() const;
	bool IsRevisitingBuildings() const;
	void Reset();
private:
	struct ExplorationGridTile
	{
		int x{};
		int y{};
		bool discovered{};
		bool autodiscovered{};
	};

	bool FindRevisitingTile(const Elite::Vector2& playerPos, int& x, int& y);
	void FindExplorationTile(const ExplorationGridTile& center, const Elite::Vector2& playerPos, int& x, int& y);
	void AutoDiscoverTile(int x, int y);

	std::vector<Elite::Vector2> m_ExploreTiles{};
	std::vector<Elite::Vector2> m_RevisitTiles{};
	
	size_t m_NrHouses{};
	bool m_IsRevisitingBuildings{};
	bool m_IsDoneExploring{};
	Elite::Vector2 m_LeftBottom{};
	float m_TileSize{};
	int m_GridSize{};
	std::vector<ExplorationGridTile> m_Grid{};

	const int m_StartSquare{ 2 };
	const int m_SquareExpansionAmount{ 10 };
};

