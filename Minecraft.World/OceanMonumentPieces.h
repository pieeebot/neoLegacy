#pragma once
#include "StructurePiece.h"
#include <vector>
#include <algorithm>

class OceanMonumentPieces
{
public:
    static void loadStatic();


    // field_175967_a = index
    // field_175965_b = neighbors[6]
    // field_175966_c = hasOpening[6]
    // field_175963_d = claimed
    // field_175964_e = isEntrance
    // field_175962_f = visitedTag
   
    struct RoomDefinition
    {
        int   index;           // field_175967_a
        RoomDefinition* neighbors[6];  // field_175965_b
        bool  hasOpening[6];           // field_175966_c
        bool  claimed;                 // field_175963_d
        bool  isEntrance;              // field_175964_e
        int   visitedTag;              // field_175962_f

        RoomDefinition(int idx) : index(idx), claimed(false), isEntrance(false), visitedTag(0)
        {
            memset(hasOpening, 0, sizeof(hasOpening));
            memset(neighbors,  0, sizeof(neighbors));
        }

        // func_175957_a 
        void connectTo(int facing, RoomDefinition* other)
        {
             this->neighbors[facing] = other;
        
        int opposite = (facing % 2 == 0) ? facing + 1 : facing - 1;
        other->neighbors[opposite] = this;
        }

        // func_175958_a 
        void initOpenings()
        {
            for (int i = 0; i < 6; ++i)
                hasOpening[i] = (neighbors[i] != nullptr);
        }

        // func_175959_a 
        bool isReachable(int tag)
        {
            if (isEntrance) return true;
            visitedTag = tag;
            for (int i = 0; i < 6; ++i)
            {
                if (neighbors[i] != nullptr && hasOpening[i] &&
                    neighbors[i]->visitedTag != tag &&
                    neighbors[i]->isReachable(tag))
                    return true;
            }
            return false;
        }

        // func_175961_b 
        bool isSpecial() const { return index >= 75; }

        // func_175960_c 
        int countOpenings() const
        {
            int c = 0;
            for (int i = 0; i < 6; ++i)
                if (hasOpening[i]) ++c;
            return c;
        }

    private:
        static int oppositeFacing(int f)
        {
            // DOWN=0 UP=1 NORTH=2 SOUTH=3 WEST=4 EAST=5
            static const int opp[6] = { 1, 0, 3, 2, 5, 4 };
            return opp[f];
        }
    };


    static int blockPrismarine();
    static int blockPrismarineBricks();
    static int blockDarkPrismarine();
    static int blockWater();
    static int blockSeaLantern();
    static int blockGoldBlock();
    static int blockSponge();
    static int blockAir();

 
    class Piece : public StructurePiece
    {
    protected:
        RoomDefinition* roomDef; // field_175830_k

        // func_175820_a(x, y, z) = y*25 + z*5 + x
        static int roomIndex(int x, int y, int z) { return y * 25 + z * 5 + x; }

        static const int ENTRY_INDEX;   // field_175823_g = roomIndex(2,0,0)
        static const int CORE_INDEX;    // field_175831_h = roomIndex(2,2,0)
        static const int WING1_INDEX;   // field_175832_i = roomIndex(0,1,0)
        static const int WING2_INDEX;   // field_175829_j = roomIndex(4,1,0)

        Piece();
        Piece(int type);
        Piece(int facing, BoundingBox* box); 
        Piece(int type, int facing, RoomDefinition* room, int sizeX, int sizeY, int sizeZ);

        // func_181655_a 
        void fillWithAirOrWater(Level* level, BoundingBox* bb, int x0, int y0, int z0, int x1, int y1, int z1, bool onlySolid);

        // func_175821_a
        void generateFloor(Level* level, BoundingBox* bb, int offX, int offZ, bool hasOpening);

        // func_175819_a 
        void fillWaterWith(Level* level, BoundingBox* bb, int x0, int y0, int z0, int x1, int y1, int z1, int block);

        // func_175818_a
        bool intersectsXZ(BoundingBox* chunkBB, int x0, int z0, int x1, int z1);

        // func_175817_a 
        bool spawnElderGuardian(Level* level, BoundingBox* bb, int x, int y, int z);

    public:
        virtual void addAdditonalSaveData(CompoundTag* tag) override;
        virtual void readAdditonalSaveData(CompoundTag* tag) override;
        virtual EStructurePiece GetType() = 0;
    };


    struct RoomFitHelper
    {
        virtual bool canFit(RoomDefinition* room) = 0;
        virtual Piece* createPiece(int facing, RoomDefinition* room, Random* random) = 0;
        virtual ~RoomFitHelper() {}
    };


    class MonumentBuilding : public Piece
    {
    private:
        static const int ARRAY_SIZE = 75; // 5*5*3
        RoomDefinition* entryRoom;  // field_175845_o
        RoomDefinition* coreRoom;   // field_175844_p
        std::vector<Piece*> subPieces; // field_175843_q

        std::vector<RoomDefinition*> buildRoomGrid(Random* random);

        
        void buildWing(bool isRight, int offsetX, Level* level, Random* random, BoundingBox* chunkBB);
        void buildEntrance(Level* level, Random* random, BoundingBox* chunkBB);
        void buildTopSection(Level* level, Random* random, BoundingBox* chunkBB);
        void buildCorridorCenter(Level* level, Random* random, BoundingBox* chunkBB);
        void buildSideCorridor(Level* level, Random* random, BoundingBox* chunkBB);
        void buildInnerCorridor(Level* level, Random* random, BoundingBox* chunkBB);
        void buildUpperSection(Level* level, Random* random, BoundingBox* chunkBB);

    public:
        static StructurePiece* Create() { return new MonumentBuilding(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentBuilding; }

        MonumentBuilding();
        MonumentBuilding(Random* random, int x, int z, int facing);
        ~MonumentBuilding();

        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };

    class CoreRoom : public Piece {
    public:
        static StructurePiece* Create() { return new CoreRoom(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentCore; }
        CoreRoom();
        CoreRoom(int facing, RoomDefinition* room, Random* random);
        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };


    class SimpleRoom : public Piece {
    private:
        int variant; // field_175833_o
    public:
        static StructurePiece* Create() { return new SimpleRoom(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentSimple; }
        SimpleRoom();
        SimpleRoom(int facing, RoomDefinition* room, Random* random);
        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };

    class SimpleTopRoom : public Piece {
    public:
        static StructurePiece* Create() { return new SimpleTopRoom(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentSimpleTop; }
        SimpleTopRoom();
        SimpleTopRoom(int facing, RoomDefinition* room, Random* random);
        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };


    class DoubleXRoom : public Piece {
    public:
        static StructurePiece* Create() { return new DoubleXRoom(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentDoubleX; }
        DoubleXRoom();
        DoubleXRoom(int facing, RoomDefinition* room, Random* random);
        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };


    class DoubleXYRoom : public Piece {
    public:
        static StructurePiece* Create() { return new DoubleXYRoom(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentDoubleXY; }
        DoubleXYRoom();
        DoubleXYRoom(int facing, RoomDefinition* room, Random* random);
        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };


    class DoubleYRoom : public Piece {
    public:
        static StructurePiece* Create() { return new DoubleYRoom(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentDoubleY; }
        DoubleYRoom();
        DoubleYRoom(int facing, RoomDefinition* room, Random* random);
        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };


    class DoubleYZRoom : public Piece {
    public:
        static StructurePiece* Create() { return new DoubleYZRoom(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentDoubleYZ; }
        DoubleYZRoom();
        DoubleYZRoom(int facing, RoomDefinition* room, Random* random);
        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };


    class DoubleZRoom : public Piece {
    public:
        static StructurePiece* Create() { return new DoubleZRoom(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentDoubleZ; }
        DoubleZRoom();
        DoubleZRoom(int facing, RoomDefinition* room, Random* random);
        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };



    class EntryRoom : public Piece {
    public:
        static StructurePiece* Create() { return new EntryRoom(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentEntry; }
        EntryRoom();
        EntryRoom(int facing, RoomDefinition* room);
        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };


    class Penthouse : public Piece {
    public:
        static StructurePiece* Create() { return new Penthouse(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentPenthouse; }
        Penthouse();
        Penthouse(int facing, BoundingBox* box);
        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };


    class WingRoom : public Piece {
    private:
        int wingType; // field_175834_o = p_i45585_3_ & 1
    public:
        static StructurePiece* Create() { return new WingRoom(); }
        virtual EStructurePiece GetType() override { return eStructurePiece_OceanMonumentWing; }
        WingRoom();
        WingRoom(int facing, BoundingBox* box, int seed);
        virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) override;
    };
};
