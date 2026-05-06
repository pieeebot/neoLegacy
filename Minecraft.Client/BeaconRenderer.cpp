#include "stdafx.h"
#include "../Minecraft.World/net.minecraft.world.level.tile.entity.h"
#include "../Minecraft.World/net.minecraft.world.level.h"
#include "BeaconRenderer.h"
#include "Tesselator.h"
#include <cmath> 
#include"..\StainedGlassBlock.h"
#include"..\StainedGlassPaneBlock.h"


ResourceLocation BeaconRenderer::BEAM_LOCATION = ResourceLocation(TN_MISC_BEACON_BEAM);


static float BEACON_COLORS[16][3] = {
    {0.074f, 0.074f, 0.074f}, // 0: Black
    {0.600f, 0.164f, 0.164f}, // 1: Red
    {0.337f, 0.423f, 0.184f}, // 2: Green
    {0.411f, 0.270f, 0.156f}, // 3: Brown
    {0.203f, 0.286f, 0.611f}, // 4: Blue
    {0.478f, 0.203f, 0.658f}, // 5: Purple
    {0.298f, 0.501f, 0.600f}, // 6: Cyan
    {0.623f, 0.623f, 0.623f}, // 7: Silver (Light Gray)
    {0.298f, 0.298f, 0.298f}, // 8: Gray
    {0.941f, 0.482f, 0.639f}, // 9: Pink
    {0.501f, 0.752f, 0.125f}, // 10: Lime
    {0.870f, 0.870f, 0.164f}, // 11: Yellow
    {0.400f, 0.623f, 0.811f}, // 12: Light Blue
    {0.701f, 0.325f, 0.823f}, // 13: Magenta
    {0.850f, 0.478f, 0.235f}, // 14: Orange
    {1.000f, 1.000f, 1.000f}  // 15: White
};




void BeaconRenderer::render(shared_ptr<TileEntity> _beacon, double x, double y, double z, float a, bool setColor, float alpha, bool useCompiled)
{
    shared_ptr<BeaconTileEntity> beacon = dynamic_pointer_cast<BeaconTileEntity>(_beacon);
    if (!beacon) return;

    float scale = beacon->getAndUpdateClientSideScale();
    if (scale <= 0) return;

    Tesselator *t = Tesselator::getInstance();
    Level* level = beacon->getLevel();
    
    
    bindTexture(&BEAM_LOCATION);

    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    float tt = (float)level->getGameTime() + a;
    float texVOff = -tt * .20f - floor(-tt * .10f);

    
    struct BeamSegment {
        float r, g, b;
        int height;
    };
    std::vector<BeamSegment> segments;

    float curR = 1.0f, curG = 1.0f, curB = 1.0f;
    int bx = beacon->x;
    int by = beacon->y;
    int bz = beacon->z;
    bool firstGlass = true;

    for (int i = by + 1; i < 256; i++) {
        int tileID = level->getTile(bx, i, bz);
        
        if (tileID == Tile::stained_glass_Id || tileID == Tile::stained_glass_pane_Id) {
            int meta = level->getData(bx, i, bz);
            int colorIdx = StainedGlassBlock::getItemAuxValueForBlockData(meta);
            
            
            if (firstGlass) {
                curR = BEACON_COLORS[colorIdx][0];
                curG = BEACON_COLORS[colorIdx][1];
                curB = BEACON_COLORS[colorIdx][2];
                firstGlass = false;
            } else {
                curR = (curR + BEACON_COLORS[colorIdx][0]) * 0.5f;
                curG = (curG + BEACON_COLORS[colorIdx][1]) * 0.5f;
                curB = (curB + BEACON_COLORS[colorIdx][2]) * 0.5f;
            }
            
            segments.push_back({curR, curG, curB, 1});
        } 
        else if (tileID == 0 || tileID == Tile::glass_Id || tileID == Tile::glass_pane_Id) {
            if (segments.empty()) {
                segments.push_back({1.0f, 1.0f, 1.0f, 1});
            } else {
                segments.back().height++;
            }
        } 
        else {
            if (Tile::tiles[tileID] && Tile::tiles[tileID]->blocksLight()) {
                break; 
            }
            else if (!segments.empty()) {
                segments.back().height++;
            }
        }
    }

    if (segments.empty()) return;

    
    glEnable(GL_BLEND);
   
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    glDepthMask(true);

    double currentYBase = 0;
    for (const auto& seg : segments) {
        int r = (int)(seg.r * 255);
        int g = (int)(seg.g * 255);
        int b = (int)(seg.b * 255);

        double rot = tt * .025 * (1 - (1 & 1) * 2.5);
        double rr1 = 0.2; 

        double wnx = .5 + cos(rot + PI * .75) * rr1;
        double wnz = .5 + sin(rot + PI * .75) * rr1;
        double enx = .5 + cos(rot + PI * .25) * rr1;
        double enz = .5 + sin(rot + PI * .25) * rr1;
        double wsx = .5 + cos(rot + PI * 1.25) * rr1;
        double wsz = .5 + sin(rot + PI * 1.25) * rr1;
        double esx = .5 + cos(rot + PI * 1.75) * rr1;
        double esz = .5 + sin(rot + PI * 1.75) * rr1;

        double yMin = currentYBase * scale;
        double yMax = (currentYBase + seg.height) * scale;
        double vv2 = currentYBase * scale * (0.5 / rr1) + texVOff;
        double vv1 = (currentYBase + seg.height) * scale * (0.5 / rr1) + texVOff;

        t->begin();
        t->color(r, g, b, 255); 

        t->vertexUV(x + wnx, y + yMax, z + wnz, 1.0, vv1);
        t->vertexUV(x + wnx, y + yMin, z + wnz, 1.0, vv2);
        t->vertexUV(x + enx, y + yMin, z + enz, 0.0, vv2);
        t->vertexUV(x + enx, y + yMax, z + enz, 0.0, vv1);
        t->vertexUV(x + esx, y + yMax, z + esz, 1.0, vv1);
        t->vertexUV(x + esx, y + yMin, z + esz, 1.0, vv2);
        t->vertexUV(x + wsx, y + yMin, z + wsz, 0.0, vv2);
        t->vertexUV(x + wsx, y + yMax, z + wsz, 0.0, vv1);
        t->vertexUV(x + enx, y + yMax, z + enz, 1.0, vv1);
        t->vertexUV(x + enx, y + yMin, z + enz, 1.0, vv2);
        t->vertexUV(x + esx, y + yMin, z + esz, 0.0, vv2);
        t->vertexUV(x + esx, y + yMax, z + esz, 0.0, vv1);
        t->vertexUV(x + wsx, y + yMax, z + wsz, 1.0, vv1);
        t->vertexUV(x + wsx, y + yMin, z + wsz, 1.0, vv2);
        t->vertexUV(x + wnx, y + yMin, z + wnz, 0.0, vv2);
        t->vertexUV(x + wnx, y + yMax, z + wnz, 0.0, vv1);
        t->end();

        currentYBase += seg.height;
    }

    
    glDepthMask(false);

    currentYBase = 0;
    for (const auto& seg : segments) {
        int r = (int)(seg.r * 255);
        int g = (int)(seg.g * 255);
        int b = (int)(seg.b * 255);

        double yMin = currentYBase * scale;
        double yMax = (currentYBase + seg.height) * scale;
        double vv2 = currentYBase * scale + texVOff;
        double vv1 = (currentYBase + seg.height) * scale + texVOff;

        t->begin();
        t->color(r, g, b, 32); 

        t->vertexUV(x + 0.2, y + yMax, z + 0.2, 1.0, vv1);
        t->vertexUV(x + 0.2, y + yMin, z + 0.2, 1.0, vv2);
        t->vertexUV(x + 0.8, y + yMin, z + 0.2, 0.0, vv2);
        t->vertexUV(x + 0.8, y + yMax, z + 0.2, 0.0, vv1);
        t->vertexUV(x + 0.8, y + yMax, z + 0.8, 1.0, vv1);
        t->vertexUV(x + 0.8, y + yMin, z + 0.8, 1.0, vv2);
        t->vertexUV(x + 0.2, y + yMin, z + 0.8, 0.0, vv2);
        t->vertexUV(x + 0.2, y + yMax, z + 0.8, 0.0, vv1);
        t->vertexUV(x + 0.8, y + yMax, z + 0.2, 1.0, vv1);
        t->vertexUV(x + 0.8, y + yMin, z + 0.2, 1.0, vv2);
        t->vertexUV(x + 0.8, y + yMin, z + 0.8, 0.0, vv2);
        t->vertexUV(x + 0.8, y + yMax, z + 0.8, 0.0, vv1);
        t->vertexUV(x + 0.2, y + yMax, z + 0.8, 1.0, vv1);
        t->vertexUV(x + 0.2, y + yMin, z + 0.8, 1.0, vv2);
        t->vertexUV(x + 0.2, y + yMin, z + 0.2, 0.0, vv2);
        t->vertexUV(x + 0.2, y + yMax, z + 0.2, 0.0, vv1);
        t->end();

        currentYBase += seg.height;
    }

    glDepthMask(true);
    glDisable(GL_BLEND);
}