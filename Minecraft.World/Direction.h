#pragma once
class Direction
{
public:
	static const int UNDEFINED = -1;
	static const int SOUTH = 0;
	static const int WEST = 1;
	static const int NORTH = 2;
	static const int EAST = 3;
	static const int STEP_X[];
	static const int STEP_Z[];
	static const wstring NAMES[];
	static int DIRECTION_FACING[];
	static int FACING_DIRECTION[];
	static int DIRECTION_OPPOSITE[];
	static int DIRECTION_CLOCKWISE[];
	static int DIRECTION_COUNTER_CLOCKWISE[];
	static int RELATIVE_DIRECTION_FACING[4][6];
	static int getDirection(double xd, double zd);
	static int getDirection(int x0, int z0, int x1, int z1);

	static int getStepX(int direction)
	{
		switch (direction) {
		case WEST:  return -1;
		case EAST:  return 1;
		default:    return 0;
		}
	}

	static int getStepZ(int direction)
	{
		switch (direction) {
		case NORTH: return -1;
		case SOUTH: return 1;
		default:    return 0;
		}
	}

	static int getStepY(int direction)
	{
		return 0;
	}

	static int get3DDataValue(int dir6)
	{
		return dir6;
	}

	static int from3DDataValue(int val)
	{
		return val;
	}

	static int getOpposite(int dir6)
	{
		static const int opp[6] = { 1, 0, 3, 2, 5, 4 };
		if (dir6 >= 0 && dir6 < 6)
			return opp[dir6];
		return dir6;
	}

	class Plane
	{
	public:
		static int getRandomFace(Random* random)
		{
			static const int horizontal[4] = {
				Direction::SOUTH,
				Direction::WEST,
				Direction::NORTH,
				Direction::EAST
			};
			return horizontal[random->nextInt(4)];
		}
	};
};