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

	static const wstring NAMES[];;

	// for [direction] it gives [tile-face]
	static int DIRECTION_FACING[];

	// for [facing] it gives [direction]
	static int FACING_DIRECTION[];

	// for [direction] it gives [opposite direction]
	static int DIRECTION_OPPOSITE[];

	// for [direction] it gives [90 degrees clockwise direction]
	static int DIRECTION_CLOCKWISE[];

	// for [direction] it gives [90 degrees counter-clockwise direction]
	static int DIRECTION_COUNTER_CLOCKWISE[];

	// for [direction][world-facing] it gives [tile-facing]
	static int RELATIVE_DIRECTION_FACING[4][6];

    static int getOpposite(int dir);
    static int from2DDataValue(int param);

	static int getDirection(double xd, double zd);
	static int getDirection(int x0, int z0, int x1, int z1);

	    static int getStepX(int direction) {
        switch (direction) {
            case WEST:  return -1;
            case EAST:  return 1;
            default:    return 0;
        }
    }
    
    static int getStepZ(int direction) {
        switch (direction) {
            case NORTH: return -1;
            case SOUTH: return 1;
            default:    return 0;
        }
    }
	    static int getStepY(int direction) {
        
        return 0;  
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
