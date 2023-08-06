#include "main.hpp"

enum dirs {
	UP, RIGHT, DOWN, LEFT, NONE = -1
};

enum tiles {
	FLOOR, WALL, HARDFLOOR, HARDWALL, MAZEFLOOR
};

void fillRectangleInPreLevel(int* level, int rows, int cols, int iStart, int jStart, int iEnd, int jEnd, int fillNum);
void createRoomInPreLevel(int* level, int rows, int cols, int iStart, int jStart, int iEnd, int jEnd, int wallNum, int fillNum);
void createRoomWithDoorsInPreLevel(int* level, int rows, int cols, int iStart, int jStart, int iEnd, int jEnd, int wallNum, int fillNum, int doorNum, int entranceNum, int topDoor = -1, int rightDoor = -1, int bottomDoor = -1, int leftDoor = -1);
void genMazeInPreLevel(int* level, int rows, int cols, int starti = 0, int startj = 0);

int generateCustomMazeLevel(char* levelset, Uint32 seed, std::tuple<int, int, int, int> mapParameters);