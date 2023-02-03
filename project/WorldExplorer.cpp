#include "stdafx.h"
#include "WorldExplorer.h"

WorldExplorer::WorldExplorer(const WorldInfo& worldInfo)
{
	// Set the gridsize
	m_GridSize = 51;

	// Set the tile size
	m_TileSize = worldInfo.Dimensions.x / m_GridSize;

	// Set the left bottom of the world
	m_LeftBottom = { worldInfo.Center - worldInfo.Dimensions / 2.0f };

	// Reserve the right amount of gridcells
	m_Grid.reserve(m_GridSize * m_GridSize);

	// Fill the grid
	for (int y{}; y < m_GridSize; ++y)
	{
		for (int x{}; x < m_GridSize; ++x)
		{
			m_Grid.emplace_back(ExplorationGridTile{ x, y, false });
		}
	}
}

void WorldExplorer::Update(const Elite::Vector2& playerPosition, float orientation)
{
	// Calcalate the center of the world
	const Elite::Vector2 center{ m_GridSize / 2.0f * m_TileSize, m_GridSize / 2.0f * m_TileSize };

	// Calculate the player position in grid space
	const Elite::Vector2 gridPlayerPosition{ playerPosition + center };
	// Calculate the x and y coordinates of the grid cell that the player is in
	const int playerX{ static_cast<int>(gridPlayerPosition.x / m_TileSize) };
	const int playerY{ static_cast<int>(gridPlayerPosition.y / m_TileSize) };

	// Set the current grid cell discovered
	m_Grid[playerY * m_GridSize + playerX].discovered = true;
	// Reset the autodiscovered of the current grid cell
	m_Grid[playerY * m_GridSize + playerX].autodiscovered = false;

	// The distance that should be check in front of the player
	constexpr float fovDistance{ 5.0f };
	// The position to check in front of the player
	const Elite::Vector2 fovPosition{ gridPlayerPosition.x + cosf(orientation) * fovDistance, gridPlayerPosition.y + sinf(orientation) * fovDistance };
	// Calculate the x and y coordinates of the grid cell that the fov sees
	const int fovX{ static_cast<int>(fovPosition.x / m_TileSize) };
	const int fovY{ static_cast<int>(fovPosition.y / m_TileSize) };

	// Set the current grid cell discovered
	m_Grid[fovY * m_GridSize + fovX].discovered = true;
	// Reset the autodiscovered of the current grid cell
	m_Grid[playerY * m_GridSize + playerX].autodiscovered = false;
}

void WorldExplorer::DrawDebug(IExamInterface* pInterface) const
{
	// The color that a discovered cell should be
	const Elite::Vector3 discoveredTileColor
	{
		0.0,
		0.0f,
		1.0f
	};

	// The color that a to-be-discovered cell should be
	const Elite::Vector3 toBeDiscoveredTileColor
	{
		0.5f,
		0.5f,
		0.5f
	};

	// For each grid cell
	for (const ExplorationGridTile& gridTile : m_Grid)
	{
		// If the cell if not yet discovered, continue to the next cell
		if (!gridTile.discovered) continue;

		// Create the rect for this tile
		const std::vector<Elite::Vector2> rect
		{
			m_LeftBottom + Elite::Vector2{ gridTile.x * m_TileSize, gridTile.y * m_TileSize },
			m_LeftBottom + Elite::Vector2{ (gridTile.x + 1) * m_TileSize, gridTile.y * m_TileSize },
			m_LeftBottom + Elite::Vector2{ (gridTile.x + 1) * m_TileSize, (gridTile.y + 1) * m_TileSize },
			m_LeftBottom + Elite::Vector2{ gridTile.x * m_TileSize, (gridTile.y + 1) * m_TileSize }
		};

		// Draw the rect
		pInterface->Draw_Polygon(rect.data(), static_cast<int>(rect.size()), discoveredTileColor);
	}

	// The current tile to search around
	int curSearchTileX{ m_GridSize / 2 };
	int curSearchTileY{ m_GridSize / 2 };
	// The current radius to check
	int checkRadius{ m_StartSquare };

	// If there are no explore tiles
	if (m_ExploreTiles.empty())
	{
		// Add the square expansion to the radius
		checkRadius += m_SquareExpansionAmount;
	}

	if (!m_RevisitTiles.empty()) // If there are revisiting tiles
	{
		// For every revisiting tile
		for (const Elite::Vector2& tile : m_RevisitTiles)
		{
			// If this gridtile is already discovered, continue to the next tile
			if (m_Grid[static_cast<int>(tile.y) * m_GridSize + static_cast<int>(tile.x)].discovered) continue;

			// Create the rect for this tile
			const std::vector<Elite::Vector2> rect
			{
				m_LeftBottom + Elite::Vector2{ tile.x * m_TileSize, tile.y * m_TileSize },
				m_LeftBottom + Elite::Vector2{ (tile.x + 1) * m_TileSize, tile.y * m_TileSize },
				m_LeftBottom + Elite::Vector2{ (tile.x + 1) * m_TileSize, (tile.y + 1) * m_TileSize },
				m_LeftBottom + Elite::Vector2{ tile.x * m_TileSize, (tile.y + 1) * m_TileSize }
			};

			// Draw the rect
			pInterface->Draw_Polygon(rect.data(), static_cast<int>(rect.size()), toBeDiscoveredTileColor);
		}
	}
	else if(!m_ExploreTiles.empty()) // If there are explore tiles
	{
		// For each explore tile
		for (const Elite::Vector2& tile : m_ExploreTiles)
		{
			// Get the current explore tile
			curSearchTileX = static_cast<int>(tile.x);
			curSearchTileY = static_cast<int>(tile.y);

			// For each tile around the current explore tile
			for (int x{ curSearchTileX - checkRadius }; x <= curSearchTileX + checkRadius; ++x)
			{
				for (int y{ curSearchTileY - checkRadius }; y <= curSearchTileY + checkRadius; ++y)
				{
					// If the current tile is not on the boundary of the radius, continue to the next tile
					if (x > curSearchTileX - checkRadius && x < curSearchTileX + checkRadius
						&& y > curSearchTileY - checkRadius && y < curSearchTileY + checkRadius) continue;

					// If the current tile is already discovered, continue to the next tile
					if (m_Grid[y * m_GridSize + x].discovered) continue;

					// Create the rect for this tile
					const std::vector<Elite::Vector2> rect
					{
						m_LeftBottom + Elite::Vector2{ x * m_TileSize, y * m_TileSize },
						m_LeftBottom + Elite::Vector2{ (x + 1) * m_TileSize, y * m_TileSize },
						m_LeftBottom + Elite::Vector2{ (x + 1) * m_TileSize, (y + 1) * m_TileSize },
						m_LeftBottom + Elite::Vector2{ x * m_TileSize, (y + 1) * m_TileSize }
					};

					// Draw the rect
					pInterface->Draw_Polygon(rect.data(), static_cast<int>(rect.size()), toBeDiscoveredTileColor);
				}
			}
		}
	}
	else // Else draw exploration square
	{
		// For each tile around the exploration square
		for (int x{ curSearchTileX - checkRadius }; x <= curSearchTileX + checkRadius; ++x)
		{
			for (int y{ curSearchTileY - checkRadius }; y <= curSearchTileY + checkRadius; ++y)
			{
				// If the current tile is not on the boundary of the radius, continue to the next tile
				if (x > curSearchTileX - checkRadius && x < curSearchTileX + checkRadius
					&& y > curSearchTileY - checkRadius && y < curSearchTileY + checkRadius) continue;

				// If the current tile is already discovered, continue to the next tile
				if (m_Grid[y * m_GridSize + x].discovered) continue;

				// Create the rect for this tile
				const std::vector<Elite::Vector2> rect
				{
					m_LeftBottom + Elite::Vector2{ x * m_TileSize, y * m_TileSize },
					m_LeftBottom + Elite::Vector2{ (x + 1) * m_TileSize, y * m_TileSize },
					m_LeftBottom + Elite::Vector2{ (x + 1) * m_TileSize, (y + 1) * m_TileSize },
					m_LeftBottom + Elite::Vector2{ x * m_TileSize, (y + 1) * m_TileSize }
				};

				// Draw the rect
				pInterface->Draw_Polygon(rect.data(), static_cast<int>(rect.size()), toBeDiscoveredTileColor);
			}
		}
	}
}

Elite::Vector2 WorldExplorer::GetNearestUndiscoveredGrid(const Elite::Vector2& playerPosition)
{
	// Calculate the center
	const ExplorationGridTile& center{ m_Grid[m_Grid.size() / 2] };

	// Calculate the player position in grid space
	const Elite::Vector2 playerGridPosition
	{
		playerPosition.x / m_TileSize + center.x,
		playerPosition.y / m_TileSize + center.y
	};

	// The current go to tile
	int curX{};
	int curY{};

	// If we are revisiting buildings and there are revisiting tiles
	if (m_IsRevisitingBuildings && !m_RevisitTiles.empty())
	{
		// If we found a tile, return the position of this tile
		if (FindRevisitingTile(playerGridPosition, curX, curY)) 
			return { (curX - m_GridSize / 2) * m_TileSize, (curY - m_GridSize / 2) * m_TileSize };
	}
	
	// Find the closest exploration tile
	FindExplorationTile(center, playerGridPosition, curX, curY);

	// return the position of this tile
	return { (curX - m_GridSize / 2) * m_TileSize, (curY - m_GridSize / 2) * m_TileSize };
}

void WorldExplorer::AddExploreTile(const Elite::Vector2& position)
{
	// Calculate the center
	const Elite::Vector2 center{ m_GridSize / 2.0f * m_TileSize, m_GridSize / 2.0f * m_TileSize };

	// Calculate the grid position
	const Elite::Vector2 gridPosition{ position + center };
	const int gridX{ static_cast<int>(gridPosition.x / m_TileSize) };
	const int gridY{ static_cast<int>(gridPosition.y / m_TileSize) };

	// Add the tile to the exploring tiles
	m_ExploreTiles.push_back(Elite::Vector2{ static_cast<float>(gridX), static_cast<float>(gridY) });

	// Reset exploring
	m_IsDoneExploring = false;
}

void WorldExplorer::AddRevisitTile(const Elite::Vector2& position)
{
	// Calculate the center
	const Elite::Vector2 center{ m_GridSize / 2.0f * m_TileSize, m_GridSize / 2.0f * m_TileSize };

	// Calculate the grid position
	const Elite::Vector2 gridPosition{ position + center };
	const int gridX{ static_cast<int>(gridPosition.x / m_TileSize) };
	const int gridY{ static_cast<int>(gridPosition.y / m_TileSize) };

	// Add the tile to the revisiting tiles
	m_RevisitTiles.push_back(Elite::Vector2{ static_cast<float>(gridX), static_cast<float>(gridY) });

	// Reset exploring and activate revisiting buildings
	m_IsDoneExploring = false;
	m_IsRevisitingBuildings = true;

	// Increment the number of houses
	++m_NrHouses;
}

bool WorldExplorer::IsDoneExploring() const
{
	return (m_IsRevisitingBuildings && m_RevisitTiles.empty()) || m_IsDoneExploring;
}

bool WorldExplorer::IsRevisitingBuildings() const
{
	return m_IsRevisitingBuildings;
}

void WorldExplorer::Reset()
{
	// Reset the number of houses
	m_NrHouses = 0;

	// Reset the discovery of every tile
	for (ExplorationGridTile& tile : m_Grid)
	{
		tile.discovered = false;
	}
}

bool WorldExplorer::FindRevisitingTile(const Elite::Vector2& playerPos, int& x, int& y)
{
	float curDistance{ FLT_MAX };

	// The amount of rooms to check regarding the distance
	constexpr size_t nrFarRoomsToCheck{ 3 };

	// For each tile in revisiting tile
	for (size_t i{}; i < m_RevisitTiles.size(); ++i)
	{
		// If we just started rediscovering 
		if (m_NrHouses - m_RevisitTiles.size() < nrFarRoomsToCheck)
		{
			// Get the first revisiting tile in the container
			const int curSearchTileX{ static_cast<int>(m_RevisitTiles[0].x) };
			const int curSearchTileY{ static_cast<int>(m_RevisitTiles[0].y) };

			// If the current tile is already discovered
			if (m_Grid[curSearchTileY * m_GridSize + curSearchTileX].discovered)
			{
				// Remove the current tile and continue to the next tile
				m_RevisitTiles[0] = m_RevisitTiles[m_RevisitTiles.size() - 1];
				m_RevisitTiles.pop_back();
				continue;
			}

			x = curSearchTileX;
			y = curSearchTileY;

			// Return the position of the current tile
			return true;
		}
		else
		{
			// Get the current tile
			const int curSearchTileX{ static_cast<int>(m_RevisitTiles[i].x) };
			const int curSearchTileY{ static_cast<int>(m_RevisitTiles[i].y) };

			// If the current tile is already discovered
			if (m_Grid[curSearchTileY * m_GridSize + curSearchTileX].discovered)
			{
				// Remove the current tile and continue to the next tile
				m_RevisitTiles[i] = m_RevisitTiles[m_RevisitTiles.size() - 1];
				m_RevisitTiles.pop_back();
				continue;
			}

			// Calculate the distance between the player and the current tile
			const float sqrDist{ playerPos.DistanceSquared(Elite::Vector2(curSearchTileX + 0.5f, curSearchTileY + 0.5f)) };

			// If the current distance is less then the current smallest distance
			if (sqrDist < curDistance)
			{
				// Store the current tile and distance
				x = curSearchTileX;
				y = curSearchTileY;
				curDistance = sqrDist;
			}

			return true;
		}
	}

	return false;
}

void WorldExplorer::FindExplorationTile(const ExplorationGridTile& center, const Elite::Vector2& playerPos, int& x, int& y)
{
	// Has found a tile to go to
	bool hasFoundTile{};

	// The current square radius
	int radius{ 2 };
	// The current tile we are searching around
	int curSearchTileX{ center.x };
	int curSearchTileY{ center.y };

	// Are we using explore tiles
	bool isUsingExploreTiles{};

	// If there are exploring tiles
	if (!m_ExploreTiles.empty())
	{
		// Set the search tile to the first explore tile
		curSearchTileX = static_cast<int>(m_ExploreTiles[0].x);
		curSearchTileY = static_cast<int>(m_ExploreTiles[0].y);
		isUsingExploreTiles = true;
	}

	// The current smallest distance
	float curDistance{ FLT_MAX };

	// While you have not found a tile yet
	while (!hasFoundTile)
	{
		// For each tile around the current explore tile
		for (int curX{ curSearchTileX - radius }; curX <= curSearchTileX + radius; ++curX)
		{
			for (int curY{ curSearchTileY - radius }; curY <= curSearchTileY + radius; ++curY)
			{
				// If the current tile is not on the boundary of the radius, continue to the next tile
				if (curX > curSearchTileX - radius && curX < curSearchTileX + radius
					&& curY > curSearchTileY - radius && curY < curSearchTileY + radius) continue;

				// If the current tile is discovered, continue to the next tile
				if (m_Grid[curY * m_GridSize + curX].discovered) continue;

				// Auto discover the current tile
				AutoDiscoverTile(curX, curY);

				// If the current tile is auto discovered, continue to the next tile
				if (m_Grid[curY * m_GridSize + curX].discovered) continue;

				hasFoundTile = true;

				// Calculate the distance between the player and the current tile
				const float sqrDist{ playerPos.DistanceSquared(Elite::Vector2(curX + 0.5f, curY + 0.5f)) };

				// If the current distance is smaller then the current smallest distance
				if (sqrDist < curDistance)
				{
					// Store the current tile and distance
					x = curX;
					y = curY;
					curDistance = sqrDist;
				}
			}
		}

		// If we found a tile, break the loop
		if (hasFoundTile) break;

		if (isUsingExploreTiles) // If using explore tiles
		{
			std::cout << "I have checked everything around this building." << "\n";

			// Remove the current explore tile
			m_ExploreTiles[0] = m_ExploreTiles[m_ExploreTiles.size() - 1];
			m_ExploreTiles.pop_back();

			if (static_cast<int>(m_ExploreTiles.size()) > 0) // If there are still explore tiles to be checked
			{
				std::cout << m_ExploreTiles.size() << " more surroundings of buildings to check.\n";
				
				// Set the current search tile to the first explore tile in the container
				curSearchTileX = static_cast<int>(m_ExploreTiles[0].x);
				curSearchTileY = static_cast<int>(m_ExploreTiles[0].y);
			}
			else // There are no more explore tiles
			{
				std::cout << "Continueing world exploration\n";
				
				// Set the current search tile to the center of the world
				curSearchTileX = center.x;
				curSearchTileY = center.y;
				isUsingExploreTiles = false;
			}
		}
		else // If we are not using explore tiles
		{
			if (radius <= m_StartSquare) // If we completed the first square
			{
				// Add the square expansion to the radius
				radius += m_SquareExpansionAmount;
			}
			else // If we completed the second square
			{
				// Set is done exploring, triggering revisiting
				m_IsDoneExploring = true;
				break;
			}
		}
	}
}

void WorldExplorer::AutoDiscoverTile(int x, int y)
{
	// Number of discovered tiles surrounding the current tile
	int nrSurroundedDiscovered{};

	// For every tile surrounding the current tile
	for (int offsetX{ -1 }; offsetX <= 1; ++offsetX)
	{
		for (int offsetY{ -1 }; offsetY <= 1; ++offsetY)
		{
			// Discard the corners
			if (abs(offsetX) && abs(offsetY)) continue;
			// Discard the center
			if (!offsetX && !offsetY) continue;
			// Discard autodiscovered tiles
			if (m_Grid[(y + offsetY) * m_GridSize + x + offsetX].autodiscovered) continue;

			// If the surrounding tile is discovered, increment number of discovered surroundings
			if (m_Grid[(y + offsetY) * m_GridSize + x + offsetX].discovered) ++nrSurroundedDiscovered;
		}
	}

	// If there are more then 1 discovered surrounding tiles
	if (nrSurroundedDiscovered > 1)
	{
		// Autodiscover the current tile
		m_Grid[y * m_GridSize + x].discovered = true;
		m_Grid[y * m_GridSize + x].autodiscovered = true;
	}
}
