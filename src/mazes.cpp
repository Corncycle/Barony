/*-------------------------------------------------------------------------------

	mazes.hpp

	Implement random generation for `gnomishmines` level when called
	by `generateDungeon`

	Most of this is copied from `generateDungeon`. Comments written for
	specifically this file are preceded by //!!

	Edits made by Corncycle

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "files.hpp"
#include "items.hpp"
#include "prng.hpp"
#include "monster.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "book.hpp"
#include "net.hpp"
#include "paths.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "scores.hpp"
#include "mod_tools.hpp"
#include "menu.hpp"
#include "mazes.hpp"

// padding indicates minimum padding between edges of room and a non-WALL tile
std::tuple<int, int> findCoordsForRoomInPreLevel(int* level, int rows, int cols, int height, int width, int padding) {
	int numPossibleLocations = rows * cols;
	bool* possibleLocations = new bool[rows * cols];

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			possibleLocations[i * cols + j] = true;
		}
	}

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if ((i % 2 == 1) || (j % 2 == 1)) {
				possibleLocations[i * cols + j] = false;
				numPossibleLocations -= 1;
			}
			else if (i < padding || j < padding || i + height > rows - padding || j + width > cols - padding) {
				possibleLocations[i * cols + j] = false;
				numPossibleLocations -= 1;
			}
			for (int ii = i; ii < std::min(i + height, rows); ii++) {
				for (int jj = j; jj < std::min(j + width, cols); jj++) {
					if (level[ii * cols + jj] != WALL && possibleLocations[i * cols + j]) {
						possibleLocations[i * cols + j] = false;
						numPossibleLocations -= 1;
					}
				}
			}
		}
	}

	/*for (int pi = 0; pi < rows; pi++) {
		for (int pj = 0; pj < cols; pj++) {
			std::cout << (possibleLocations[pi * cols + pj] ? "*" : " ");
		}
		std::cout << std::endl;
	}

	std::cout << "Number of possible locations: " << numPossibleLocations << std::endl;*/

	int pickedLoc = prng_get_int() % numPossibleLocations;

	int locsSeen = -1;
	int i = 0;
	int j = 0;

	while (true) {
		if (possibleLocations[i * cols + j]) {
			locsSeen += 1;
			if (locsSeen == pickedLoc) {
				break;
			}
		}
		j += 1;
		if (j >= cols) {
			j = 0;
			i += 1;
			if (i >= rows) {
				i = 0;
			}
		}
	}

	return { i, j };
}

void fillRectangleInPreLevel(int* level, int rows, int cols, int iStart, int jStart, int iEnd, int jEnd, int fillNum) {
	for (int i = iStart; i < iEnd; i++) {
		for (int j = jStart; j < jEnd; j++) {
			level[i * cols + j] = fillNum;
		}
	}
}

void createRoomInPreLevel(int* level, int rows, int cols, int iStart, int jStart, int iEnd, int jEnd, int wallNum, int fillNum) {
	fillRectangleInPreLevel(level, rows, cols, iStart, jStart, iEnd, jStart + 1, wallNum);
	fillRectangleInPreLevel(level, rows, cols, iStart, jEnd - 1, iEnd, jEnd, wallNum);
	fillRectangleInPreLevel(level, rows, cols, iStart, jStart, iStart + 1, jEnd, wallNum);
	fillRectangleInPreLevel(level, rows, cols, iEnd - 1, jStart, iEnd, jEnd, wallNum);
	fillRectangleInPreLevel(level, rows, cols, iStart + 1, jStart + 1, iEnd - 1, jEnd - 1, fillNum);
}

void createRoomWithDoorsInPreLevel(int* level, int rows, int cols, int iStart, int jStart, int iEnd, int jEnd, int wallNum, int fillNum, int doorNum, int entranceNum, int topDoor, int rightDoor, int bottomDoor, int leftDoor) {
	createRoomInPreLevel(level, rows, cols, iStart, jStart, iEnd, jEnd, wallNum, fillNum);
	if (topDoor > -1) {
		level[iStart * cols + jStart + topDoor] = doorNum;
		level[(iStart - 1) * cols + jStart + topDoor] = entranceNum;
	}
	if (rightDoor > -1) {
		level[(iStart + rightDoor) * cols + jEnd - 1] = doorNum;
		level[(iStart + rightDoor) * cols + jEnd] = entranceNum;
	}
	if (bottomDoor > -1) {
		level[(iEnd - 1) * cols + jStart + bottomDoor] = doorNum;
		level[(iEnd)*cols + jStart + bottomDoor] = entranceNum;
	}
	if (leftDoor > -1) {
		level[(iStart + leftDoor) * cols + jStart] = doorNum;
		level[(iStart + leftDoor) * cols + jStart - 1] = entranceNum;
	}
}

// returned int: N = 0, E = 1, S = 2, W = 3
int getOpenDir(int* level, int rows, int cols, int i, int j) {
	int numOpen = 0;
	if ((i >= 2) && (level[(i - 2) * cols + j] == WALL)) {
		numOpen += 1;
	}
	if ((i <= rows - 3) && (level[(i + 2) * cols + j] == WALL)) {
		numOpen += 1;
	}
	if ((j >= 2) && (level[(i * cols + j - 2)] == WALL)) {
		numOpen += 1;
	}
	if ((j <= cols - 3) && (level[i * cols + j + 2] == WALL)) {
		numOpen += 1;
	}

	if (!numOpen) {
		return -1;
	}

	int pull = prng_get_int() % numOpen;
	int counter = 0;

	if ((i >= 2) && (level[(i - 2) * cols + j] == WALL)) {
		if (counter == pull) {
			return UP;
		}
		counter += 1;
	}
	if ((i <= rows - 3) && (level[(i + 2) * cols + j] == WALL)) {
		if (counter == pull) {
			return DOWN;
		}
		counter += 1;
	}
	if ((j >= 2) && (level[(i * cols + j - 2)] == WALL)) {
		if (counter == pull) {
			return LEFT;
		}
		counter += 1;
	}
	if ((j <= cols - 3) && (level[i * cols + j + 2] == WALL)) {
		if (counter == pull) {
			return RIGHT;
		}
	}
	return -1;
}

int hasPopulatedNeighbor(int* level, int rows, int cols, int i, int j) {
	if ((i >= 2) && (level[(i - 2) * cols + j] == MAZEFLOOR)) {
		return UP;
	}
	if ((i <= rows - 3) && (level[(i + 2) * cols + j] == MAZEFLOOR)) {
		return DOWN;
	}
	if ((j >= 2) && (level[(i * cols + j - 2)] == MAZEFLOOR)) {
		return LEFT;
	}
	if ((j <= cols - 3) && (level[i * cols + j + 2] == MAZEFLOOR)) {
		return RIGHT;
	}
	return NONE;
}

bool tileCanBeSafelyRemoved(int* level, int rows, int cols, int i, int j) {
	return ((level[(i + 1) * cols + j] == MAZEFLOOR && level[(i - 1) * cols + j] == MAZEFLOOR)
		|| (level[i * cols + j + 1] == MAZEFLOOR && level[i * cols + j - 1] == MAZEFLOOR));
}

void genMazeInPreLevel(int* level, int rows, int cols, int starti, int startj) {
	int ci = starti; // current i
	int cj = startj; // current j

	level[ci * cols + cj] = 3;

	int direction;
	int ci2;
	int cj2;
	int direction2;

	while (1) {
		//printLevelPretty(level, rows, cols);
		direction = getOpenDir(level, rows, cols, ci, cj);
		if (direction >= 0) {
			switch (direction) {
			case UP:
				level[(ci - 1) * cols + cj] = MAZEFLOOR;
				level[(ci - 2) * cols + cj] = MAZEFLOOR;
				ci -= 2;
				break;
			case RIGHT:
				level[ci * cols + cj + 1] = MAZEFLOOR;
				level[ci * cols + cj + 2] = MAZEFLOOR;
				cj += 2;
				break;
			case DOWN:
				level[(ci + 1) * cols + cj] = MAZEFLOOR;
				level[(ci + 2) * cols + cj] = MAZEFLOOR;
				ci += 2;
				break;
			case LEFT:
				level[ci * cols + cj - 1] = MAZEFLOOR;
				level[ci * cols + cj - 2] = MAZEFLOOR;
				cj -= 2;
				break;
			}
		}
		else {
			ci2 = starti;
			cj2 = startj;
			while (ci2 < rows) {
				if (level[ci2 * cols + cj2] != WALL) {
					if (cj2 >= cols - 2) {
						ci2 += 2;
						cj2 = startj;
					}
					else {
						cj2 += 2;
					}
					continue;
				}

				direction2 = hasPopulatedNeighbor(level, rows, cols, ci2, cj2);
				if (direction2 >= 0) {
					//std::cout << "NEED TO FIND A NEW SPOT: MOVING IN DIRECTION " << direction2 << std::endl;
					level[ci2 * cols + cj2] = MAZEFLOOR;
					switch (direction2) {
					case UP:
						level[(ci2 - 1) * cols + cj2] = MAZEFLOOR;
						break;
					case RIGHT:
						level[ci2 * cols + cj2 + 1] = MAZEFLOOR;
						break;
					case DOWN:
						level[(ci2 + 1) * cols + cj2] = MAZEFLOOR;
						break;
					case LEFT:
						level[ci2 * cols + cj2 - 1] = MAZEFLOOR;
						break;
					}
					ci = ci2;
					cj = cj2;
					break;
				}
				else {
					if (cj2 >= cols - 2) {
						ci2 += 2;
						cj2 = startj;
					}
					else {
						cj2 += 2;
					}
				}
			}
			if (ci2 >= rows) {
				break;
			}
		}
	}

	// try 100 times to remove a wall from the maze
	int i;
	int j;
	for (int c = 0; c < 150; c++) {
		i = (prng_get_int() % (rows - 2)) + 1;
		j = (prng_get_int() % (cols - 2)) + 1;
		if (level[i * cols + j] == WALL && tileCanBeSafelyRemoved(level, rows, cols, i, j)) {
			level[i * cols + j] = FLOOR;
		}
	}
}

int generateCustomMazeLevel(char* levelset, Uint32 seed, std::tuple<int, int, int, int> mapParameters) {
	char* sublevelname, * subRoomName;
	char sublevelnum[3];
	map_t* tempMap, * subRoomMap;
	list_t mapList, * newList, * subRoomList, subRoomMapList;
	node_t* node, * node2, * node3, * nextnode, * subRoomNode;
	Sint32 c, i, j;
	Sint32 numlevels, levelnum, levelnum2, subRoomNumLevels;
	Sint32 x, y, z;
	Sint32 x0, y0, x1, y1;
	door_t* door, * newDoor;
	bool* possiblelocations, * possiblelocations2, * possiblerooms;
	bool* firstroomtile;
	Sint32 numpossiblelocations, pickedlocation, subroomPickRoom;
	Entity* entity, * entity2, * childEntity;
	Uint32 levellimit;
	list_t doorList;
	node_t* doorNode, * subRoomDoorNode;
	bool shoplevel = false;
	map_t shopmap;
	map_t secretlevelmap;
	int secretlevelexit = 0;
	bool* trapexcludelocations;
	bool* monsterexcludelocations;
	bool* lootexcludelocations;

	printlog("generateCustomMaze called");

	if (std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) == -1
		&& std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) == -1
		&& std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) == -1
		&& std::get<LEVELPARAM_DISABLE_NORMAL_EXIT>(mapParameters) == 0)
	{
		printlog("generating a dungeon from level set '%s' (seed %d)...\n", levelset, seed);
	}
	else
	{
		char generationLog[256] = "generating a dungeon from level set '%s'";
		char tmpBuffer[32];
		if (std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) != -1)
		{
			snprintf(tmpBuffer, 31, ", secret chance %d%%%%", std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters));
			strcat(generationLog, tmpBuffer);
		}
		if (std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) != -1)
		{
			snprintf(tmpBuffer, 31, ", darkmap chance %d%%%%", std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters));
			strcat(generationLog, tmpBuffer);
		}
		if (std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) != -1)
		{
			snprintf(tmpBuffer, 31, ", minotaur chance %d%%%%", std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters));
			strcat(generationLog, tmpBuffer);
		}
		if (std::get<LEVELPARAM_DISABLE_NORMAL_EXIT>(mapParameters) != 0)
		{
			snprintf(tmpBuffer, 31, ", disabled normal exit", std::get<LEVELPARAM_DISABLE_NORMAL_EXIT>(mapParameters));
			strcat(generationLog, tmpBuffer);
		}
		strcat(generationLog, ", (seed %d)...\n");
		printlog(generationLog, levelset, seed);

		conductGameChallenges[CONDUCT_MODDED] = 1;
	}

	std::string fullMapPath;
	fullMapPath = physfsFormatMapName(levelset);

	int checkMapHash = -1;
	if (fullMapPath.empty() || loadMap(fullMapPath.c_str(), &map, map.entities, map.creatures, &checkMapHash) == -1)
	{
		printlog("error: no level of set '%s' could be found.\n", levelset);
		return -1;
	}
	if (checkMapHash == 0)
	{
		conductGameChallenges[CONDUCT_MODDED] = 1;
	}

	// test comment
	// store this map's seed
	mapseed = seed;
	prng_seed_bytes(&mapseed, sizeof(mapseed));

	// generate a custom monster curve if file exists
	monsterCurveCustomManager.readFromFile();

	// determine whether shop level or not
	//!! maze levels are never shop levels (custom shops handled separately)
	/*if (gameplayCustomManager.processedShopFloor(currentlevel, secretlevel, map.name, shoplevel))
	{
		// function sets shop level for us.
	}
	else if (prng_get_uint() % 2 && currentlevel > 1 && strncmp(map.name, "Underworld", 10) && strncmp(map.name, "Hell", 4))
	{
		shoplevel = true;
	}*/

	// determine whether minotaur level or not
	//!! maze levels are never minotaur levels (custom minotaur handled separately)
	/*if ((svFlags & SV_FLAG_MINOTAURS) && gameplayCustomManager.processedMinotaurSpawn(currentlevel, secretlevel, map.name))
	{
		// function sets mino level for us.
	}
	else if (std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) != -1)
	{
		if (prng_get_uint() % 100 < std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) && (svFlags & SV_FLAG_MINOTAURS))
		{
			minotaurlevel = 1;
		}
	}
	else if ((currentlevel < 25 && (currentlevel % LENGTH_OF_LEVEL_REGION == 2 || currentlevel % LENGTH_OF_LEVEL_REGION == 3))
		|| (currentlevel > 25 && (currentlevel % LENGTH_OF_LEVEL_REGION == 2 || currentlevel % LENGTH_OF_LEVEL_REGION == 4)))
	{
		if (prng_get_uint() % 2 && (svFlags & SV_FLAG_MINOTAURS))
		{
			minotaurlevel = 1;
		}
	}*/

	// dark level
	//!! maze levels are never dark levels
	/*if (gameplayCustomManager.processedDarkFloor(currentlevel, secretlevel, map.name))
	{
		// function sets dark level for us.
		if (darkmap)
		{
			messagePlayer(clientnum, language[1108]);
		}
	}
	else if (!secretlevel)
	{
		if (std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) != -1)
		{
			if (prng_get_uint() % 100 < std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters))
			{
				darkmap = true;
				messagePlayer(clientnum, language[1108]);
			}
			else
			{
				darkmap = false;
			}
		}
		else if (currentlevel % LENGTH_OF_LEVEL_REGION >= 2)
		{
			if (prng_get_uint() % 4 == 0)
			{
				darkmap = true;
				messagePlayer(clientnum, language[1108]);
			}
		}
	}

	// secret stuff
	if (!secretlevel)
	{
		if (std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) != -1)
		{
			if (prng_get_uint() % 100 < std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters))
			{
				secretlevelexit = 7;
			}
			else
			{
				secretlevelexit = 0;
			}
		}
		else if ((currentlevel == 3 && prng_get_uint() % 2) || currentlevel == 2)
		{
			secretlevelexit = 1;
		}
		else if (currentlevel == 7 || currentlevel == 8)
		{
			secretlevelexit = 2;
		}
		else if (currentlevel == 11 || currentlevel == 13)
		{
			secretlevelexit = 3;
		}
		else if (currentlevel == 16 || currentlevel == 18)
		{
			secretlevelexit = 4;
		}
		else if (currentlevel == 28)
		{
			secretlevelexit = 5;
		}
		else if (currentlevel == 33)
		{
			secretlevelexit = 6;
		}
	}*/

	mapList.first = nullptr;
	mapList.last = nullptr;
	doorList.first = nullptr;
	doorList.last = nullptr;

	// load shop room
	//!! (don't)
	/*if (shoplevel)
	{
		sublevelname = (char*)malloc(sizeof(char) * 128);
		for (numlevels = 0; numlevels < 100; numlevels++)
		{
			strcpy(sublevelname, "shop");
			snprintf(sublevelnum, 3, "%02d", numlevels);
			strcat(sublevelname, sublevelnum);

			fullMapPath = physfsFormatMapName(sublevelname);

			if (fullMapPath.empty())
			{
				break;    // no more levels to load
			}
		}
		if (numlevels)
		{
			int shopleveltouse = prng_get_uint() % numlevels;
			if (!strncmp(map.name, "Citadel", 7))
			{
				strcpy(sublevelname, "shopcitadel");
			}
			else
			{
				strcpy(sublevelname, "shop");
				snprintf(sublevelnum, 3, "%02d", shopleveltouse);
				strcat(sublevelname, sublevelnum);
			}

			fullMapPath = physfsFormatMapName(sublevelname);

			shopmap.tiles = nullptr;
			shopmap.entities = (list_t*)malloc(sizeof(list_t));
			shopmap.entities->first = nullptr;
			shopmap.entities->last = nullptr;
			shopmap.creatures = new list_t;
			shopmap.creatures->first = nullptr;
			shopmap.creatures->last = nullptr;
			if (fullMapPath.empty() || loadMap(fullMapPath.c_str(), &shopmap, shopmap.entities, shopmap.creatures, &checkMapHash) == -1)
			{
				list_FreeAll(shopmap.entities);
				free(shopmap.entities);
				list_FreeAll(shopmap.creatures);
				delete shopmap.creatures;
				if (shopmap.tiles)
				{
					free(shopmap.tiles);
				}
			}
			if (checkMapHash == 0)
			{
				conductGameChallenges[CONDUCT_MODDED] = 1;
			}
		}
		else
		{
			shoplevel = false;
		}
		free(sublevelname);
	}*/

	sublevelname = (char*)malloc(sizeof(char) * 128);

	// a maximum of 100 (0-99 inclusive) sublevels can be added to the pool
	//!! there are only 3 levels to be loaded for gnomish mines
	for (numlevels = 0; numlevels < 100; ++numlevels)
	{
		strcpy(sublevelname, levelset);
		snprintf(sublevelnum, 3, "%02d", numlevels);
		strcat(sublevelname, sublevelnum);

		fullMapPath = physfsFormatMapName(sublevelname);
		if (fullMapPath.empty())
		{
			break;    // no more levels to load
		}

		// allocate memory for the next sublevel and attempt to load it
		tempMap = (map_t*)malloc(sizeof(map_t));
		tempMap->tiles = nullptr;
		tempMap->entities = (list_t*)malloc(sizeof(list_t));
		tempMap->entities->first = nullptr;
		tempMap->entities->last = nullptr;
		tempMap->creatures = new list_t;
		tempMap->creatures->first = nullptr;
		tempMap->creatures->last = nullptr;
		if (fullMapPath.empty() || loadMap(fullMapPath.c_str(), tempMap, tempMap->entities, tempMap->creatures, &checkMapHash) == -1)
		{
			mapDeconstructor((void*)tempMap);
			continue; // failed to load level
		}
		if (checkMapHash == 0)
		{
			conductGameChallenges[CONDUCT_MODDED] = 1;
		}

		// level is successfully loaded, add it to the pool
		newList = (list_t*)malloc(sizeof(list_t));
		newList->first = nullptr;
		newList->last = nullptr;
		node = list_AddNodeLast(&mapList);
		node->element = newList;
		node->deconstructor = &listDeconstructor;

		node = list_AddNodeLast(newList);
		node->element = tempMap;
		node->deconstructor = &mapDeconstructor;

		// more nodes are created to record the exit points on the sublevel
		for (y = 0; y < tempMap->height; y++)
		{
			for (x = 0; x < tempMap->width; x++)
			{
				if (x == 0 || y == 0 || x == tempMap->width - 1 || y == tempMap->height - 1)
				{
					if (!tempMap->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * tempMap->height])
					{
						door = (door_t*)malloc(sizeof(door_t));
						door->x = x;
						door->y = y;
						if (x == tempMap->width - 1)
						{
							door->dir = 0;
						}
						else if (y == tempMap->height - 1)
						{
							door->dir = 1;
						}
						else if (x == 0)
						{
							door->dir = 2;
						}
						else if (y == 0)
						{
							door->dir = 3;
						}
						node2 = list_AddNodeLast(newList);
						node2->element = door;
						node2->deconstructor = &defaultDeconstructor;
					}
				}
			}
		}
	}

	subRoomName = (char*)malloc(sizeof(char) * 128);
	subRoomMapList.first = nullptr;
	subRoomMapList.last = nullptr;
	char letterString[2];
	letterString[1] = '\0';
	int subroomCount[100] = { 0 };

	// a maximum of 100 (0-99 inclusive) sublevels can be added to the pool
	//!! no subrooms in maze levels
	/*for (subRoomNumLevels = 0; subRoomNumLevels <= numlevels; subRoomNumLevels++)
	{
		for (char letter = 'a'; letter <= 'z'; letter++)
		{
			// look for mapnames ending in a letter a to z
			strcpy(subRoomName, levelset);
			snprintf(sublevelnum, 3, "%02d", subRoomNumLevels);
			letterString[0] = letter;
			strcat(subRoomName, sublevelnum);
			strcat(subRoomName, letterString);

			fullMapPath = physfsFormatMapName(subRoomName);

			if (fullMapPath.empty())
			{
				break;    // no more levels to load
			}

			// check if there is another subroom to load
			//if ( !dataPathExists(fullMapPath.c_str()) )
			//{
			//	break;    // no more levels to load
			//}

			printlog("[SUBMAP GENERATOR] Found map lv %s, count: %d", subRoomName, subroomCount[subRoomNumLevels]);
			++subroomCount[subRoomNumLevels];

			// allocate memory for the next subroom and attempt to load it
			subRoomMap = (map_t*)malloc(sizeof(map_t));
			subRoomMap->tiles = nullptr;
			subRoomMap->entities = (list_t*)malloc(sizeof(list_t));
			subRoomMap->entities->first = nullptr;
			subRoomMap->entities->last = nullptr;
			subRoomMap->creatures = new list_t;
			subRoomMap->creatures->first = nullptr;
			subRoomMap->creatures->last = nullptr;
			if (fullMapPath.empty() || loadMap(fullMapPath.c_str(), subRoomMap, subRoomMap->entities, subRoomMap->creatures, &checkMapHash) == -1)
			{
				mapDeconstructor((void*)subRoomMap);
				continue; // failed to load level
			}
			if (checkMapHash == 0)
			{
				conductGameChallenges[CONDUCT_MODDED] = 1;
			}

			// level is successfully loaded, add it to the pool
			subRoomList = (list_t*)malloc(sizeof(list_t));
			subRoomList->first = nullptr;
			subRoomList->last = nullptr;
			node = list_AddNodeLast(&subRoomMapList);
			node->element = subRoomList;
			node->deconstructor = &listDeconstructor;

			node = list_AddNodeLast(subRoomList);
			node->element = subRoomMap;
			node->deconstructor = &mapDeconstructor;

			// more nodes are created to record the exit points on the sublevel
			
		}
	}*/

	// generate dungeon level...
	//!! generation will be handled by the separate maze generating functions defined above this function,
	//!! but we'll copy a lot of this code just to move in the 3 needed sublevels.
	//!! i removed all of this block of code and pasted the parts of it that we
	//!! need after the maze was generated
	int roomcount = 0;

	//!! the prelevel is just a 48x48 integer grid with integers to represent a tile at that location
	//!! tile values found the enum `tiles` in `mazes.hpp`. after the prelevel is generated with a
	//!! maze and rooms, it is brought into barony's tile system
	int* prelevel = new int[48 * 48] {};
	//!! initialize the prelevel with walls to be carved out	
	createRoomInPreLevel(prelevel, 48, 48, 0, 0, 48, 48, HARDWALL, WALL);

	auto spawn = findCoordsForRoomInPreLevel(prelevel, 48, 48, 9, 9, 16);
	int spawni = std::get<0>(spawn);
	int spawnj = std::get<1>(spawn);

	// all rooms should have an odd length (so their walls align with the maze)
	// findCoordsForRoom will 
	createRoomWithDoorsInPreLevel(prelevel, 48, 48, spawni, spawnj, spawni + 9, spawnj + 9, HARDFLOOR, HARDFLOOR, FLOOR, WALL, 5, 5, 5, 5);

	auto shop = findCoordsForRoomInPreLevel(prelevel, 48, 48, 7 + 4, 7 + 4, 0);
	int shopi = std::get<0>(shop) + 1;
	int shopj = std::get<1>(shop) + 1;

	createRoomWithDoorsInPreLevel(prelevel, 48, 48, shopi, shopj, shopi + 7, shopj + 7, HARDFLOOR, HARDFLOOR, FLOOR, FLOOR, 2, 2, 4);

	auto secret = findCoordsForRoomInPreLevel(prelevel, 48, 48, 9 + 4, 9 + 4, 0);
	int secreti = std::get<0>(secret) + 2;
	int secretj = std::get<1>(secret) + 2;

	createRoomWithDoorsInPreLevel(prelevel, 48, 48, secreti, secretj, secreti + 9, secretj + 9, HARDFLOOR, HARDFLOOR, FLOOR, WALL, 3, 3, 3, 3);

	genMazeInPreLevel(prelevel, 48, 48, 1, 1);

	createRoomWithDoorsInPreLevel(prelevel, 48, 48, spawni, spawnj, spawni + 9, spawnj + 9, WALL, FLOOR, FLOOR, FLOOR, 5, 5, 5, 5);

	createRoomInPreLevel(prelevel, 48, 48, shopi, shopj, shopi + 7, shopj + 7, FLOOR, FLOOR);
	createRoomWithDoorsInPreLevel(prelevel, 48, 48, shopi + 1, shopj + 1, shopi + 6, shopj + 6, WALL, FLOOR, FLOOR, FLOOR, -1, -1, 2);

	createRoomWithDoorsInPreLevel(prelevel, 48, 48, secreti, secretj, secreti + 9, secretj + 9, WALL, FLOOR, FLOOR, FLOOR, 3, 3, 3, 3);

	// use the prelevel to populate the actual map with walls
	for (int xx = 0; xx < map.width; xx++) {
		for (int yy = 0; yy < map.height; yy++) {
			for (int zz = 1; zz < 2; zz++) {
				switch (prelevel[yy * 48 + xx]) {
					case FLOOR:
					case HARDFLOOR:
					case MAZEFLOOR: {
						map.tiles[zz + yy * MAPLAYERS + xx * MAPLAYERS * map.height] = 0;
						break;
					}
					case HARDWALL:
					case WALL: {
						map.tiles[zz + yy * MAPLAYERS + xx * MAPLAYERS * map.height] = 10;
					}
					
				}
			}
		}
	}

	printlog("DONE WITH MAZE GEN");
	delete[] prelevel;

	//!! attach the 3 fixed sublevels to the map
	if (numlevels > 1)
	{
		possiblelocations = (bool*)malloc(sizeof(bool) * map.width * map.height);
		trapexcludelocations = (bool*)malloc(sizeof(bool) * map.width * map.height);
		monsterexcludelocations = (bool*)malloc(sizeof(bool) * map.width * map.height);
		lootexcludelocations = (bool*)malloc(sizeof(bool) * map.width * map.height);
		for (y = 0; y < map.height; y++)
		{
			for (x = 0; x < map.width; x++)
			{
				if (x < 2 || y < 2 || x > map.width - 3 || y > map.height - 3)
				{
					possiblelocations[x + y * map.width] = false;
				}
				else
				{
					possiblelocations[x + y * map.width] = true;
				}
				trapexcludelocations[x + y * map.width] = false;
				if (map.flags[MAP_FLAG_DISABLEMONSTERS] == true)
				{
					// the base map excludes all monsters
					monsterexcludelocations[x + y * map.width] = true;
				}
				else
				{
					monsterexcludelocations[x + y * map.width] = false;
				}
				if (map.flags[MAP_FLAG_DISABLELOOT] == true)
				{
					// the base map excludes all monsters
					lootexcludelocations[x + y * map.width] = true;
				}
				else
				{
					lootexcludelocations[x + y * map.width] = false;
				}
			}
		}
		possiblelocations2 = (bool*)malloc(sizeof(bool) * map.width * map.height);
		firstroomtile = (bool*)malloc(sizeof(bool) * map.width * map.height);
		possiblerooms = (bool*)malloc(sizeof(bool) * numlevels);
		for (c = 0; c < numlevels; c++)
		{
			possiblerooms[c] = true;
		}
		levellimit = (map.width * map.height);
		for (c = 0; c < 3; c++)
		{
			if (!numlevels)
			{
				break;
			}
			levelnum = c; // draw randomly from the pool
			//!! no, take the sublevels in order 00 to 02
			//!! 00 = spawn, 01 = shop, 02 = secret

			// traverse the map list to the picked level
			node = mapList.first;
			i = 0;
			j = -1;
			while (1)
			{
				if (possiblerooms[i])
				{
					++j;
					if (j == levelnum)
					{
						break;
					}
				}
				node = node->next;
				++i;
			}
			levelnum2 = i;
			node = ((list_t*)node->element)->first;
			doorNode = node->next;
			tempMap = (map_t*)node->element;

			// now copy all the geometry from the sublevel to the chosen location
			//!! ?????????????? no idea what this is doing. look into this
			if (c == 0)
			{
				for (z = 0; z < map.width * map.height; ++z)
				{
					firstroomtile[z] = false;
				}
			}

			switch (c) {
			case 0: {
				x = spawnj;
				y = spawni;
				break;
			}
			case 1: {
				x = shopj;
				y = shopi;
				break;
			}
			case 2: {
				x = secretj;
				y = secreti;
				break;
			}
			}
			x1 = x + tempMap->width;
			y1 = y + tempMap->height;

			for (z = 0; z < MAPLAYERS; z++)
			{
				for (y0 = y; y0 < y1; y0++)
				{
					for (x0 = x; x0 < x1; x0++)
					{
						map.tiles[z + y0 * MAPLAYERS + x0 * MAPLAYERS * map.height] = tempMap->tiles[z + (y0 - y) * MAPLAYERS + (x0 - x) * MAPLAYERS * tempMap->height];

						if (z == 0)
						{
							possiblelocations[x0 + y0 * map.width] = false;
							if (tempMap->flags[MAP_FLAG_DISABLETRAPS] == 1)
							{
								trapexcludelocations[x0 + y0 * map.width] = true;
								//map.tiles[z + y0 * MAPLAYERS + x0 * MAPLAYERS * map.height] = 83;
							}
							if (tempMap->flags[MAP_FLAG_DISABLEMONSTERS] == 1)
							{
								monsterexcludelocations[x0 + y0 * map.width] = true;
							}
							if (tempMap->flags[MAP_FLAG_DISABLELOOT] == 1)
							{
								lootexcludelocations[x0 + y0 * map.width] = true;
							}
							if (c == 0)
							{
								firstroomtile[y0 + x0 * map.height] = true;
							}
							else if (c == 2 && shoplevel)
							{
								firstroomtile[y0 + x0 * map.height] = true;
								if (x0 - x > 0 && y0 - y > 0 && x0 - x < tempMap->width - 1 && y0 - y < tempMap->height - 1)
								{
									shoparea[y0 + x0 * map.height] = true;
								}
							}
						}

						// remove any existing entities in this region too
						for (node = map.entities->first; node != nullptr; node = nextnode)
						{
							nextnode = node->next;
							Entity* entity = (Entity*)node->element;
							if ((int)entity->x == x0 << 4 && (int)entity->y == y0 << 4)
							{
								list_RemoveNode(entity->mynode);
							}
						}
					}
				}
			}

			// copy the entities as well from the tempMap.
			for (node = tempMap->entities->first; node != nullptr; node = node->next)
			{
				entity = (Entity*)node->element;
				childEntity = newEntity(entity->sprite, 1, map.entities, nullptr);

				// entity will return nullptr on getStats called in setSpriteAttributes as behaviour &actmonster is not set.
				// check if the monster sprite is correct and set the behaviour manually for getStats.
				if (checkSpriteType(entity->sprite) == 1 && multiplayer != CLIENT)
				{
					entity->behavior = &actMonster;
				}

				setSpriteAttributes(childEntity, entity, entity);
				childEntity->x = entity->x + x * 16;
				childEntity->y = entity->y + y * 16;
				//printlog("1 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);

				if (entity->behavior == actMonster || entity->behavior == actPlayer)
				{
					entity->addToCreatureList(map.creatures);
				}
			}

			// finally, copy the doors into a single doors list
			while (doorNode != nullptr)
			{
				door = (door_t*)doorNode->element;
				newDoor = (door_t*)malloc(sizeof(door_t));
				newDoor->x = door->x + x;
				newDoor->y = door->y + y;
				newDoor->dir = door->dir;
				node = list_AddNodeLast(&doorList);
				node->element = newDoor;
				node->deconstructor = &defaultDeconstructor;
				doorNode = doorNode->next;
			}

			++roomcount;
		}

		free(possiblerooms);
		free(possiblelocations2);
	}
	else
	{
		free(subRoomName);
		free(sublevelname);
		list_FreeAll(&subRoomMapList);
		list_FreeAll(&mapList);
		list_FreeAll(&doorList);
		//!! printlog("error: not enough levels to begin generating dungeon.\n");
		//!! in our case, we always expect 3 sublevels. if not, this is a problem
		printlog("error: couldn't load gnomish mines sublevels for some reason!\n");
		return -1;
	}

	// post-processing:

	// doors
	//!! this block of code prevents doors from being on adjacent tiles, and
	//!! guarantees at least one open tile outside of doorways. both of these
	//!! are guaranteed by maze generation already, so this block is commented

	/*for (node = doorList.first; node != nullptr; node = node->next)
	{
		door = (door_t*)node->element;
		for (node2 = map.entities->first; node2 != nullptr; node2 = node2->next)
		{
			entity = (Entity*)node2->element;
			if (entity->x / 16 == door->x && entity->y / 16 == door->y && (entity->sprite == 2 || entity->sprite == 3))
			{
				switch (door->dir)
				{
				case 0: // east
					map.tiles[OBSTACLELAYER + door->y * MAPLAYERS + (door->x + 1) * MAPLAYERS * map.height] = 0;
					for (node3 = map.entities->first; node3 != nullptr; node3 = nextnode)
					{
						entity = (Entity*)node3->element;
						nextnode = node3->next;
						if (entity->sprite == 2 || entity->sprite == 3)
						{
							if ((int)(entity->x / 16) == door->x + 2 && (int)(entity->y / 16) == door->y)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y + 1)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y - 1)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
						}
					}
					break;
				case 1: // south
					map.tiles[OBSTACLELAYER + (door->y + 1) * MAPLAYERS + door->x * MAPLAYERS * map.height] = 0;
					for (node3 = map.entities->first; node3 != nullptr; node3 = nextnode)
					{
						entity = (Entity*)node3->element;
						nextnode = node3->next;
						if (entity->sprite == 2 || entity->sprite == 3)
						{
							if ((int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y + 2)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y + 1)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y + 1)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y + 1)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
						}
					}
					break;
				case 2: // west
					map.tiles[OBSTACLELAYER + door->y * MAPLAYERS + (door->x - 1) * MAPLAYERS * map.height] = 0;
					for (node3 = map.entities->first; node3 != nullptr; node3 = nextnode)
					{
						entity = (Entity*)node3->element;
						nextnode = node3->next;
						if (entity->sprite == 2 || entity->sprite == 3)
						{
							if ((int)(entity->x / 16) == door->x - 2 && (int)(entity->y / 16) == door->y)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y + 1)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y - 1)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
						}
					}
					break;
				case 3: // north
					map.tiles[OBSTACLELAYER + (door->y - 1) * MAPLAYERS + door->x * MAPLAYERS * map.height] = 0;
					for (node3 = map.entities->first; node3 != nullptr; node3 = nextnode)
					{
						entity = (Entity*)node3->element;
						nextnode = node3->next;
						if (entity->sprite == 2 || entity->sprite == 3)
						{
							if ((int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y - 2)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y - 1)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y - 1)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
							else if ((int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y - 1)
							{
								list_RemoveNode(entity->mynode);
								break;
							}
						}
					}
					break;
				}
			}
		}
	}*/
	bool foundsubmaptile = false;
	// if for whatever reason some submap 201 tiles didn't get filled in, let's get rid of those.
	for (z = 0; z < MAPLAYERS; ++z)
	{
		for (y = 1; y < map.height; ++y)
		{
			for (x = 1; x < map.height; ++x)
			{
				if (map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] == 201)
				{
					map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] = 0;
					foundsubmaptile = true;
				}
			}
		}
	}
	if (foundsubmaptile)
	{
		printlog("[SUBMAP GENERATOR] Found some junk tiles!");
	}

	for (node = map.entities->first; node != nullptr; node = node->next)
	{
		// fix gate air-gap borders on citadel map next to perimeter gates.
		if (!strncmp(map.name, "Citadel", 7))
		{
			Entity* gateEntity = (Entity*)node->element;
			if (gateEntity->sprite == 19 || gateEntity->sprite == 20) // N/S E/W gates take these sprite numbers in the editor.
			{
				int gatex = static_cast<int>(gateEntity->x) / 16;
				int gatey = static_cast<int>(gateEntity->y) / 16;
				for (z = OBSTACLELAYER; z < MAPLAYERS; ++z)
				{
					if (gateEntity->x / 16 == 1) // along leftmost edge
					{
						if (!map.tiles[z + gatey * MAPLAYERS + (gatex + 1) * MAPLAYERS * map.height])
						{
							map.tiles[z + gatey * MAPLAYERS + (gatex + 1) * MAPLAYERS * map.height] = 230;
							//messagePlayer(0, "replaced at: %d, %d", gatex, gatey);
						}
					}
					else if (gateEntity->x / 16 == 51) // along rightmost edge
					{
						if (!map.tiles[z + gatey * MAPLAYERS + (gatex - 1) * MAPLAYERS * map.height])
						{
							map.tiles[z + gatey * MAPLAYERS + (gatex - 1) * MAPLAYERS * map.height] = 230;
							//messagePlayer(0, "replaced at: %d, %d", gatex, gatey);
						}
					}
					else if (gateEntity->y / 16 == 1) // along top edge
					{
						if (!map.tiles[z + (gatey + 1) * MAPLAYERS + gatex * MAPLAYERS * map.height])
						{
							map.tiles[z + (gatey + 1) * MAPLAYERS + gatex * MAPLAYERS * map.height] = 230;
							//messagePlayer(0, "replaced at: %d, %d", gatex, gatey);
						}
					}
					else if (gateEntity->y / 16 == 51) // along bottom edge
					{
						if (!map.tiles[z + (gatey - 1) * MAPLAYERS + gatex * MAPLAYERS * map.height])
						{
							map.tiles[z + (gatey - 1) * MAPLAYERS + gatex * MAPLAYERS * map.height] = 230;
							//messagePlayer(0, "replaced at: %d, %d", gatex, gatey);
						}
					}
				}
			}
		}
	}

	bool customTrapsForMapInUse = false;
	struct CustomTraps
	{
		bool boulders = false;
		bool arrows = false;
		bool spikes = false;
		bool verticalSpelltraps = false;
	} customTraps;

	if (gameplayCustomManager.inUse() && gameplayCustomManager.mapGenerationExistsForMapName(map.name))
	{
		auto m = gameplayCustomManager.getMapGenerationForMapName(map.name);
		if (m && m->usingTrapTypes)
		{
			customTrapsForMapInUse = true;
			for (auto& traps : m->trapTypes)
			{
				if (traps.compare("boulders") == 0)
				{
					customTraps.boulders = true;
				}
				else if (traps.compare("arrows") == 0)
				{
					customTraps.arrows = true;
				}
				else if (traps.compare("spikes") == 0)
				{
					customTraps.spikes = true;
				}
				else if (traps.compare("spelltrap_vertical") == 0)
				{
					customTraps.verticalSpelltraps = true;
				}
			}
		}
	}

	// boulder and arrow traps
	//!! don't spawn traps on maze levels
	/*if ((svFlags & SV_FLAG_TRAPS) && map.flags[MAP_FLAG_DISABLETRAPS] == 0
		&& (!customTrapsForMapInUse || (customTrapsForMapInUse && (customTraps.boulders || customTraps.arrows)))
		)
	{
		numpossiblelocations = 0;
		for (c = 0; c < map.width * map.height; ++c)
		{
			possiblelocations[c] = false;
		}
		for (y = 1; y < map.height - 1; ++y)
		{
			for (x = 1; x < map.width - 1; ++x)
			{
				int sides = 0;
				if (firstroomtile[y + x * map.height])
				{
					continue;
				}
				if (!map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x + 1) * MAPLAYERS * map.height])
				{
					sides++;
				}
				if (!map.tiles[OBSTACLELAYER + (y + 1) * MAPLAYERS + x * MAPLAYERS * map.height])
				{
					sides++;
				}
				if (!map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x - 1) * MAPLAYERS * map.height])
				{
					sides++;
				}
				if (!map.tiles[OBSTACLELAYER + (y - 1) * MAPLAYERS + x * MAPLAYERS * map.height])
				{
					sides++;
				}
				if (sides == 1 && (trapexcludelocations[x + y * map.width] == false))
				{
					possiblelocations[y + x * map.height] = true;
					numpossiblelocations++;
				}
			}
		}

		// don't spawn traps in doors
		node_t* doorNode;
		for (doorNode = doorList.first; doorNode != nullptr; doorNode = doorNode->next)
		{
			door_t* door = (door_t*)doorNode->element;
			int x = std::min<unsigned int>(std::max(0, door->x), map.width); //TODO: Why are const int and unsigned int being compared?
			int y = std::min<unsigned int>(std::max(0, door->y), map.height); //TODO: Why are const int and unsigned int being compared?
			if (possiblelocations[y + x * map.height] == true)
			{
				possiblelocations[y + x * map.height] = false;
				--numpossiblelocations;
			}
		}

		// do a second pass to look for internal doorways
		for (node = map.entities->first; node != nullptr; node = node->next)
		{
			entity = (Entity*)node->element;
			int x = entity->x / 16;
			int y = entity->y / 16;
			if ((entity->sprite == 2 || entity->sprite == 3)
				&& (x >= 0 && x < map.width)
				&& (y >= 0 && y < map.height))
			{
				if (possiblelocations[y + x * map.height])
				{
					possiblelocations[y + x * map.height] = false;
					--numpossiblelocations;
				}
			}
		}

		int whatever = prng_get_uint() % 5;
		if (strncmp(map.name, "Hell", 4))
			j = std::min(
				std::min(
					std::max(1, currentlevel),
					5
				)
				+ whatever, numpossiblelocations
			)
			/ ((strcmp(map.name, "The Mines") == 0) + 1);
		else
		{
			j = std::min(15, numpossiblelocations);
		}
		//printlog("j: %d\n",j);
		//printlog("numpossiblelocations: %d\n",numpossiblelocations);
		for (c = 0; c < j; ++c)
		{
			// choose a random location from those available
			pickedlocation = prng_get_uint() % numpossiblelocations;
			i = -1;
			//printlog("pickedlocation: %d\n",pickedlocation);
			//printlog("numpossiblelocations: %d\n",numpossiblelocations);
			x = 0;
			y = 0;
			while (1)
			{
				if (possiblelocations[y + x * map.height] == true)
				{
					i++;
					if (i == pickedlocation)
					{
						break;
					}
				}
				x++;
				if (x >= map.width)
				{
					x = 0;
					y++;
					if (y >= map.height)
					{
						y = 0;
					}
				}
			}
			int side = 0;
			if (!map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x + 1) * MAPLAYERS * map.height])
			{
				side = 0;
			}
			else if (!map.tiles[OBSTACLELAYER + (y + 1) * MAPLAYERS + x * MAPLAYERS * map.height])
			{
				side = 1;
			}
			else if (!map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x - 1) * MAPLAYERS * map.height])
			{
				side = 2;
			}
			else if (!map.tiles[OBSTACLELAYER + (y - 1) * MAPLAYERS + x * MAPLAYERS * map.height])
			{
				side = 3;
			}
			bool arrowtrap = false;
			bool noceiling = false;
			bool arrowtrapspawn = false;
			if (!strncmp(map.name, "Hell", 4))
			{
				if (side == 0 && !map.tiles[(MAPLAYERS - 1) + y * MAPLAYERS + (x + 1) * MAPLAYERS * map.height])
				{
					noceiling = true;
				}
				if (side == 1 && !map.tiles[(MAPLAYERS - 1) + (y + 1) * MAPLAYERS + x * MAPLAYERS * map.height])
				{
					noceiling = true;
				}
				if (side == 2 && !map.tiles[(MAPLAYERS - 1) + y * MAPLAYERS + (x - 1) * MAPLAYERS * map.height])
				{
					noceiling = true;
				}
				if (side == 3 && !map.tiles[(MAPLAYERS - 1) + (y - 1) * MAPLAYERS + x * MAPLAYERS * map.height])
				{
					noceiling = true;
				}
				if (noceiling)
				{
					arrowtrapspawn = true;
				}
			}
			else
			{
				if (prng_get_uint() % 2 && (currentlevel > 5 && currentlevel <= 25))
				{
					arrowtrapspawn = true;
				}
			}

			if (customTrapsForMapInUse)
			{
				arrowtrapspawn = customTraps.arrows;
				if (customTraps.boulders && prng_get_uint() % 2)
				{
					arrowtrapspawn = false;
				}
			}

			if (arrowtrapspawn || noceiling)
			{
				arrowtrap = true;
				entity = newEntity(32, 1, map.entities, nullptr); // arrow trap
				entity->behavior = &actArrowTrap;
				map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] = 53; // trap wall
			}
			else
			{
				//messagePlayer(0, "Included at x: %d, y: %d", x, y);
				entity = newEntity(38, 1, map.entities, nullptr); // boulder trap
				entity->behavior = &actBoulderTrap;
			}
			entity->x = x * 16;
			entity->y = y * 16;
			//printlog("2 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
			entity = newEntity(18, 1, map.entities, nullptr); // electricity node
			entity->x = x * 16 - (side == 3) * 16 + (side == 1) * 16;
			entity->y = y * 16 - (side == 0) * 16 + (side == 2) * 16;
			//printlog("4 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
			// make torches
			if (arrowtrap)
			{
				entity = newEntity(4 + side, 1, map.entities, nullptr);
				Entity* entity2 = newEntity(4 + side, 1, map.entities, nullptr);
				switch (side)
				{
				case 0:
					entity->x = x * 16 + 16;
					entity->y = y * 16 + 4;
					entity2->x = x * 16 + 16;
					entity2->y = y * 16 - 4;
					break;
				case 1:
					entity->x = x * 16 + 4;
					entity->y = y * 16 + 16;
					entity2->x = x * 16 - 4;
					entity2->y = y * 16 + 16;
					break;
				case 2:
					entity->x = x * 16 - 16;
					entity->y = y * 16 + 4;
					entity2->x = x * 16 - 16;
					entity2->y = y * 16 - 4;
					break;
				case 3:
					entity->x = x * 16 + 4;
					entity->y = y * 16 - 16;
					entity2->x = x * 16 - 4;
					entity2->y = y * 16 - 16;
					break;
				}
				//printlog("5 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
				//printlog("6 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity2->sprite,entity2->getUID(),entity2->x,entity2->y);
			}
			i = 0;
			int testx = 0, testy = 0;
			do
			{
				if (i == 0)
				{
					// get rid of extraneous torch
					node_t* tempNode;
					node_t* nextTempNode;
					for (tempNode = map.entities->first; tempNode != nullptr; tempNode = nextTempNode)
					{
						nextTempNode = tempNode->next;
						Entity* tempEntity = (Entity*)tempNode->element;
						if (tempEntity->sprite >= 4 && tempEntity->sprite <= 7)
						{
							if (((int)floor(tempEntity->x + 8)) / 16 == x && ((int)floor(tempEntity->y + 8)) / 16 == y)
							{
								list_RemoveNode(tempNode);
							}
						}
					}
				}
				if (arrowtrap)
				{
					entity = newEntity(33, 1, map.entities, nullptr); // pressure plate
				}
				else
				{
					entity = newEntity(34, 1, map.entities, nullptr); // pressure plate
				}
				entity->x = x * 16;
				entity->y = y * 16;
				//printlog("7 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
				entity = newEntity(18, 1, map.entities, nullptr); // electricity node
				entity->x = x * 16 - (side == 3) * 16 + (side == 1) * 16;
				entity->y = y * 16 - (side == 0) * 16 + (side == 2) * 16;
				//printlog("8 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
				switch (side)
				{
				case 0:
					x++;
					break;
				case 1:
					y++;
					break;
				case 2:
					x--;
					break;
				case 3:
					y--;
					break;
				}
				i++;
				testx = std::min(std::max<unsigned int>(0, x), map.width - 1); //TODO: Why are const int and unsigned int being compared?
				testy = std::min(std::max<unsigned int>(0, y), map.height - 1); //TODO: Why are const int and unsigned int being compared?
			} while (!map.tiles[OBSTACLELAYER + testy * MAPLAYERS + testx * MAPLAYERS * map.height] && i <= 10);
		}
	}*/


	// monsters, decorations, and items
	numpossiblelocations = map.width * map.height;
	for (y = 0; y < map.height; y++)
	{
		for (x = 0; x < map.width; x++)
		{
			if (checkObstacle(x * 16 + 8, y * 16 + 8, NULL, NULL) || firstroomtile[y + x * map.height])
			{
				possiblelocations[y + x * map.height] = false;
				numpossiblelocations--;
			}
			else if (lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]])
			{
				possiblelocations[y + x * map.height] = false;
				numpossiblelocations--;
			}
			else if (swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]])
			{
				possiblelocations[y + x * map.height] = false;
				numpossiblelocations--;
			}
			else
			{
				possiblelocations[y + x * map.height] = true;
			}
		}
	}
	for (node = map.entities->first; node != nullptr; node = node->next)
	{
		entity = (Entity*)node->element;
		x = entity->x / 16;
		y = entity->y / 16;
		if (x >= 0 && x < map.width && y >= 0 && y < map.height)
		{
			if (possiblelocations[y + x * map.height])
			{
				possiblelocations[y + x * map.height] = false;
				--numpossiblelocations;
			}
		}
	}

	// read some editor map data if available:
	int genEntityMin = 0;
	int genEntityMax = 0;
	int genMonsterMin = 0;
	int genMonsterMax = 0;
	int genLootMin = 0;
	int genLootMax = 0;
	int genDecorationMin = 0;
	int genDecorationMax = 0;

	if (map.flags[MAP_FLAG_GENBYTES1] != 0 || map.flags[MAP_FLAG_GENBYTES2] != 0)
	{
		genEntityMin = (map.flags[MAP_FLAG_GENBYTES1] >> 24) & 0xFF; // first leftmost byte
		genEntityMax = (map.flags[MAP_FLAG_GENBYTES1] >> 16) & 0xFF; // second leftmost byte

		genMonsterMin = (map.flags[MAP_FLAG_GENBYTES1] >> 8) & 0xFF; // third leftmost byte
		genMonsterMax = (map.flags[MAP_FLAG_GENBYTES1] >> 0) & 0xFF; // fourth leftmost byte

		genLootMin = (map.flags[MAP_FLAG_GENBYTES2] >> 24) & 0xFF; // first leftmost byte
		genLootMax = (map.flags[MAP_FLAG_GENBYTES2] >> 16) & 0xFF; // second leftmost byte

		genDecorationMin = (map.flags[MAP_FLAG_GENBYTES2] >> 8) & 0xFF; // third leftmost byte
		genDecorationMax = (map.flags[MAP_FLAG_GENBYTES2] >> 0) & 0xFF; // fourth leftmost byte
	}

	int entitiesToGenerate = 30;
	int randomEntities = 10;

	//!! don't spawn a random amount of entities
	//!! on vanilla map, 33 enemies (13 trolls, 20 gnomes) spawn, so spawn 33 enemies
	/*if (genEntityMin > 0 || genEntityMax > 0)
	{
		genEntityMin = std::max(genEntityMin, 2); // make sure there's room for a ladder.
		entitiesToGenerate = genEntityMin;
		randomEntities = std::max(genEntityMax - genEntityMin, 1); // difference between min and max is the extra chances.
		//Needs to be 1 for prng_get_uint() % to not divide by 0.
		j = std::min<Uint32>(entitiesToGenerate + prng_get_uint() % randomEntities, numpossiblelocations); //TODO: Why are Uint32 and Sin32 being compared?
	}
	else
	{
		// revert to old mechanics.
		j = std::min<Uint32>(30 + prng_get_uint() % 10, numpossiblelocations); //TODO: Why are Uint32 and Sin32 being compared?
	}*/

	//!! hard code entity spawns
	int forcedMonsterSpawns = 33;
	int forcedLootSpawns = 0;
	int forcedDecorationSpawns = 0;

	/*if (genMonsterMin > 0 || genMonsterMax > 0)
	{
		forcedMonsterSpawns = genMonsterMin + prng_get_uint() % std::max(genMonsterMax - genMonsterMin, 1);
	}
	if (genLootMin > 0 || genLootMax > 0)
	{
		forcedLootSpawns = genLootMin + prng_get_uint() % std::max(genLootMax - genLootMin, 1);
	}
	if (genDecorationMin > 0 || genDecorationMax > 0)
	{
		forcedDecorationSpawns = genDecorationMin + prng_get_uint() % std::max(genDecorationMax - genDecorationMin, 1);
	}*/

	//messagePlayer(0, "Num locations: %d of %d possible, force monsters: %d, force loot: %d, force decorations: %d", j, numpossiblelocations, forcedMonsterSpawns, forcedLootSpawns, forcedDecorationSpawns);
	printlog("Num locations: %d of %d possible, force monsters: %d, force loot: %d, force decorations: %d", j, numpossiblelocations, forcedMonsterSpawns, forcedLootSpawns, forcedDecorationSpawns);
	int numGenItems = 0;
	int numGenGold = 0;
	int numGenDecorations = 0;

	//printlog("j: %d\n",j);
	//printlog("numpossiblelocations: %d\n",numpossiblelocations);
	
	//!! previously std::min(j, numpossiblelocations) in for loop below
	//!! (presumably j is number of rooms generated? double check this)
	//!! for modded gnomish mines gen we know how many entities we want to spawn
	//!! +1 is for the ladder
	int numSpawns = forcedMonsterSpawns + forcedLootSpawns + 1;
	for (c = 0; c < numSpawns; ++c)
	{
		// choose a random location from those available
		pickedlocation = prng_get_uint() % numpossiblelocations;
		i = -1;
		//printlog("pickedlocation: %d\n",pickedlocation);
		//printlog("numpossiblelocations: %d\n",numpossiblelocations);
		x = 0;
		y = 0;
		while (1)
		{
			if (possiblelocations[y + x * map.height] == true)
			{
				++i;
				if (i == pickedlocation)
				{
					break;
				}
			}
			++x;
			if (x >= map.width)
			{
				x = 0;
				++y;
				if (y >= map.height)
				{
					y = 0;
				}
			}
		}

		// create entity
		entity = nullptr;
		if ((c == 0 || (minotaurlevel && c < 2)) && (!secretlevel || currentlevel != 7) && (!secretlevel || currentlevel != 20)
			&& std::get<LEVELPARAM_DISABLE_NORMAL_EXIT>(mapParameters) == 0)
		{
			if (strcmp(map.name, "Hell"))
			{
				entity = newEntity(11, 1, map.entities, nullptr); // ladder
				entity->behavior = &actLadder;
			}
			else
			{
				entity = newEntity(45, 1, map.entities, nullptr); // hell uses portals instead
				entity->behavior = &actPortal;
				entity->skill[3] = 1; // not secret portals though
			}

			// determine if the ladder generated in a viable location
			if (strncmp(map.name, "Underworld", 10))
			{
				bool nopath = false;
				bool hellLadderFix = !strncmp(map.name, "Hell", 4);
				/*if ( !hellLadderFix )
				{
					hellLadderFix = !strncmp(map.name, "Caves", 4);
				}*/
				for (node = map.entities->first; node != NULL; node = node->next)
				{
					entity2 = (Entity*)node->element;
					if (entity2->sprite == 1)
					{
						list_t* path = generatePath(x, y, entity2->x / 16, entity2->y / 16, entity, entity2, hellLadderFix);
						if (path == NULL)
						{
							nopath = true;
						}
						else
						{
							list_FreeAll(path);
							free(path);
						}
						break;
					}
				}
				if (nopath)
				{
					// try again
					c--;
					list_RemoveNode(entity->mynode);
					entity = NULL;
				}
			}
		}
		else if (c == 1 && secretlevel && currentlevel == 7 && !strncmp(map.name, "Underworld", 10))
		{
			entity = newEntity(89, 1, map.entities, nullptr);
			entity->monsterStoreType = 1;
			entity->skill[5] = nummonsters;
			++nummonsters;
			//entity = newEntity(68, 1, map.entities, nullptr); // magic (artifact) bow
		}
		else
		{
			int x2, y2;
			bool nodecoration = false;
			int obstacles = 0;
			for (x2 = -1; x2 <= 1; x2++)
			{
				for (y2 = -1; y2 <= 1; y2++)
				{
					if (checkObstacle((x + x2) * 16, (y + y2) * 16, NULL, NULL))
					{
						obstacles++;
						if (obstacles > 1)
						{
							break;
						}
					}
				}
				if (obstacles > 1)
				{
					break;
				}
			}
			if (obstacles > 1)
			{
				nodecoration = true;
			}
			if (forcedMonsterSpawns > 0 || forcedLootSpawns > 0 || (forcedDecorationSpawns > 0 && !nodecoration))
			{
				// force monsters, then loot, then decorations.
				if (forcedMonsterSpawns > 0)
				{
					--forcedMonsterSpawns;
					if (monsterexcludelocations[x + y * map.width] == false)
					{
						bool doNPC = false;
						//!! don't spawn friendly NPCs on maze floors
						/*if (gameplayCustomManager.processedPropertyForFloor(currentlevel, secretlevel, map.name, GameplayCustomManager::PROPERTY_NPC, doNPC))
						{
							// doNPC processed by function
						}
						else if (prng_get_uint() % 10 == 0 && currentlevel > 1)
						{
							doNPC = true;
						}*/

						if (doNPC)
						{
							if (currentlevel > 15 && prng_get_uint() % 4 > 0)
							{
								entity = newEntity(93, 1, map.entities, map.creatures);  // automaton
								if (currentlevel < 25)
								{
									entity->monsterStoreType = 1; // weaker version
								}
							}
							else
							{
								entity = newEntity(27, 1, map.entities, map.creatures);  // human
								if (multiplayer != CLIENT && currentlevel > 5)
								{
									entity->monsterStoreType = (currentlevel / 5) * 3 + (rand() % 4); // scale humans with depth.  3 LVL each 5 floors, + 0-3.
								}
							}
						}
						else
						{
							entity = newEntity(10, 1, map.entities, map.creatures);  // monster
						}
						entity->skill[5] = nummonsters;
						++nummonsters;
					}
				}
				/*else if (forcedLootSpawns > 0)
				{
					--forcedLootSpawns;
					if (lootexcludelocations[x + y * map.width] == false)
					{
						if (prng_get_uint() % 10 == 0)   // 10% chance
						{
							entity = newEntity(9, 1, map.entities, nullptr);  // gold
							numGenGold++;
						}
						else
						{
							entity = newEntity(8, 1, map.entities, nullptr);  // item
							setSpriteAttributes(entity, nullptr, nullptr);
							numGenItems++;
						}
					}
				}
				else if (forcedDecorationSpawns > 0 && !nodecoration)
				{
					--forcedDecorationSpawns;
					// decorations
					if ((prng_get_uint() % 4 == 0 || currentlevel <= 10 && !customTrapsForMapInUse) && strcmp(map.name, "Hell"))
					{
						switch (prng_get_uint() % 7)
						{
						case 0:
							entity = newEntity(12, 1, map.entities, nullptr); //Firecamp.
							break; //Firecamp
						case 1:
							entity = newEntity(14, 1, map.entities, nullptr); //Fountain.
							break; //Fountain
						case 2:
							entity = newEntity(15, 1, map.entities, nullptr); //Sink.
							break; //Sink
						case 3:
							entity = newEntity(21, 1, map.entities, nullptr); //Chest.
							setSpriteAttributes(entity, nullptr, nullptr);
							entity->chestLocked = -1;
							break; //Chest
						case 4:
							entity = newEntity(39, 1, map.entities, nullptr); //Tomb.
							break; //Tomb
						case 5:
							entity = newEntity(59, 1, map.entities, nullptr); //Table.
							setSpriteAttributes(entity, nullptr, nullptr);
							break; //Table
						case 6:
							entity = newEntity(60, 1, map.entities, nullptr); //Chair.
							setSpriteAttributes(entity, nullptr, nullptr);
							break; //Chair
						}
					}
					else
					{
						if (customTrapsForMapInUse)
						{
							if (!customTraps.spikes && !customTraps.verticalSpelltraps)
							{
								continue;
							}
							else if (customTraps.verticalSpelltraps && prng_get_uint() % 2 == 0)
							{
								entity = newEntity(120, 1, map.entities, nullptr); // vertical spell trap.
								setSpriteAttributes(entity, nullptr, nullptr);
							}
							else if (customTraps.spikes)
							{
								entity = newEntity(64, 1, map.entities, nullptr); // spear trap
							}
						}
						else
						{
							if (currentlevel <= 25)
							{
								entity = newEntity(64, 1, map.entities, nullptr); // spear trap
							}
							else
							{
								if (prng_get_uint() % 2 == 0)
								{
									entity = newEntity(120, 1, map.entities, nullptr); // vertical spell trap.
									setSpriteAttributes(entity, nullptr, nullptr);
								}
								else
								{
									entity = newEntity(64, 1, map.entities, nullptr); // spear trap
								}
							}
						}
						Entity* also = newEntity(33, 1, map.entities, nullptr);
						also->x = x * 16;
						also->y = y * 16;
						//printlog("15 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",also->sprite,also->getUID(),also->x,also->y);
					}
					numGenDecorations++;
				}*/
			}
			else
			{
				printlog("we're doing normal generation! we should probably abort this!");
				printlog("c is ");
				printlog(std::to_string(c).c_str());
				// return to normal generation
				if (prng_get_uint() % 2 || nodecoration)
				{
					// balance for total number of players
					int balance = 0;
					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!client_disconnected[i])
						{
							balance++;
						}
					}
					switch (balance)
					{
					case 1:
						balance = 4;
						break;
					case 2:
						balance = 3;
						break;
					case 3:
						balance = 2;
						break;
					case 4:
						balance = 2;
						break;
					default:
						balance = 2;
						break;
					}

					// monsters/items
					if (balance)
					{
						if (prng_get_uint() % balance)
						{
							if (lootexcludelocations[x + y * map.width] == false)
							{
								if (prng_get_uint() % 10 == 0)   // 10% chance
								{
									entity = newEntity(9, 1, map.entities, nullptr);  // gold
									numGenGold++;
								}
								else
								{
									entity = newEntity(8, 1, map.entities, nullptr);  // item
									setSpriteAttributes(entity, nullptr, nullptr);
									numGenItems++;
								}
							}
						}
						else
						{
							if (monsterexcludelocations[x + y * map.width] == false)
							{
								bool doNPC = false;
								if (gameplayCustomManager.processedPropertyForFloor(currentlevel, secretlevel, map.name, GameplayCustomManager::PROPERTY_NPC, doNPC))
								{
									// doNPC processed by function
								}
								else if (prng_get_uint() % 10 == 0 && currentlevel > 1)
								{
									doNPC = true;
								}

								if (doNPC)
								{
									if (currentlevel > 15 && prng_get_uint() % 4 > 0)
									{
										entity = newEntity(93, 1, map.entities, map.creatures);  // automaton
										if (currentlevel < 25)
										{
											entity->monsterStoreType = 1; // weaker version
										}
									}
									else
									{
										entity = newEntity(27, 1, map.entities, map.creatures);  // human
										if (multiplayer != CLIENT && currentlevel > 5)
										{
											entity->monsterStoreType = (currentlevel / 5) * 3 + (rand() % 4); // scale humans with depth. 3 LVL each 5 floors, + 0-3.
										}
									}
								}
								else
								{
									entity = newEntity(10, 1, map.entities, map.creatures);  // monster
								}
								entity->skill[5] = nummonsters;
								nummonsters++;
							}
						}
					}
				}
				else
				{
					// decorations
					if ((prng_get_uint() % 4 == 0 || (currentlevel <= 10 && !customTrapsForMapInUse)) && strcmp(map.name, "Hell"))
					{
						switch (prng_get_uint() % 7)
						{
						case 0:
							entity = newEntity(12, 1, map.entities, nullptr); //Firecamp entity.
							break; //Firecamp
						case 1:
							entity = newEntity(14, 1, map.entities, nullptr); //Fountain entity.
							break; //Fountain
						case 2:
							entity = newEntity(15, 1, map.entities, nullptr); //Sink entity.
							break; //Sink
						case 3:
							entity = newEntity(21, 1, map.entities, nullptr); //Chest entity.
							setSpriteAttributes(entity, nullptr, nullptr);
							entity->chestLocked = -1;
							break; //Chest
						case 4:
							entity = newEntity(39, 1, map.entities, nullptr); //Tomb entity.
							break; //Tomb
						case 5:
							entity = newEntity(59, 1, map.entities, nullptr); //Table entity.
							setSpriteAttributes(entity, nullptr, nullptr);
							break; //Table
						case 6:
							entity = newEntity(60, 1, map.entities, nullptr); //Chair entity.
							setSpriteAttributes(entity, nullptr, nullptr);
							break; //Chair
						}
					}
					else
					{
						if (customTrapsForMapInUse)
						{
							if (!customTraps.spikes && !customTraps.verticalSpelltraps)
							{
								continue;
							}
							else if (customTraps.verticalSpelltraps && prng_get_uint() % 2 == 0)
							{
								entity = newEntity(120, 1, map.entities, nullptr); // vertical spell trap.
								setSpriteAttributes(entity, nullptr, nullptr);
							}
							else if (customTraps.spikes)
							{
								entity = newEntity(64, 1, map.entities, nullptr); // spear trap
							}
						}
						else
						{
							if (currentlevel <= 25)
							{
								entity = newEntity(64, 1, map.entities, nullptr); // spear trap
							}
							else
							{
								if (prng_get_uint() % 2 == 0)
								{
									entity = newEntity(120, 1, map.entities, nullptr); // vertical spell trap.
									setSpriteAttributes(entity, nullptr, nullptr);
								}
								else
								{
									entity = newEntity(64, 1, map.entities, nullptr); // spear trap
								}
							}
						}
						Entity* also = newEntity(33, 1, map.entities, nullptr);
						also->x = x * 16;
						also->y = y * 16;
						//printlog("15 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",also->sprite,also->getUID(),also->x,also->y);
					}
					numGenDecorations++;
				}
			}
		}
		if (entity != nullptr)
		{
			entity->x = x * 16;
			entity->y = y * 16;
			//printlog("9 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
		}
		// mark this location as inelligible for reselection
		possiblelocations[y + x * map.height] = false;
		numpossiblelocations--;
	}

	// on hell levels, lava doesn't bubble. helps performance
	/*if( !strcmp(map.name,"Hell") ) {
		for( node=map.entities->first; node!=NULL; node=node->next ) {
			Entity *entity = (Entity *)node->element;
			if( entity->sprite == 41 ) { // lava.png
				entity->skill[4] = 1; // LIQUID_LAVANOBUBBLE =
			}
		}
	}*/

	free(possiblelocations);
	free(trapexcludelocations);
	free(monsterexcludelocations);
	free(lootexcludelocations);
	free(firstroomtile);
	free(subRoomName);
	free(sublevelname);
	list_FreeAll(&subRoomMapList);
	list_FreeAll(&mapList);
	list_FreeAll(&doorList);
	printlog("successfully generated a dungeon with %d rooms, %d monsters, %d gold, %d items, %d decorations.\n", roomcount, nummonsters, numGenGold, numGenItems, numGenDecorations);
	//messagePlayer(0, "successfully generated a dungeon with %d rooms, %d monsters, %d gold, %d items, %d decorations.", roomcount, nummonsters, numGenGold, numGenItems, numGenDecorations);
	return secretlevelexit;
}