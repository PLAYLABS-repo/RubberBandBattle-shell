#include "AbsolutEngine.h"
#include "Src/UI.h"

#include <cstdio>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stdio.h>
#include <algorithm>
#include "Src/Timer.h"

#include "DrawHelpers.h"
#include "Player.h"


using namespace std;


#pragma once


inline void SetupWindow(Window& window, const string& WindowName, int WindowWidth, int WindowHeight){

    window.create(WindowName.c_str(), WindowWidth, WindowHeight);

}
inline void PlayerInit(Player& player, const string& PlayerImageDir , const string& PlayerAtlasDir , const string& PlayerAnimDir, Vec2 PlayerPosition){
   Image* playersheet = PL_LoadImage(PlayerImageDir.c_str());
    Atlas* playeratlas = LoadAtlas(PlayerAtlasDir.c_str());
    player.anim = CreateAnimator();
    player.anim->Load(PlayerAnimDir.c_str());


     player.sprite                 = CreateSprite();
    player.sprite->image          = playersheet;
    player.sprite->atlas          = playeratlas;
    player.sprite->frameName      = "0001";
    player.sprite->position       = {player.x, player.y};
    player.sprite->targetPosition = {player.x, player.y};
    player.sprite->scale          = {PlayerPosition};

player.playersheet = PL_LoadImage(PlayerImageDir.c_str());
player.playeratlas = LoadAtlas(PlayerAtlasDir.c_str());


if(player.playersheet == nullptr)
{
    printf("FAILED TO LOAD PLAYER IMAGE\n");
}
else
{
    printf("PLAYER IMAGE LOADED\n");
}

if(player.playeratlas == nullptr)
{
    printf("FAILED TO LOAD PLAYER ATLAS\n");
}
else
{
    printf("BULLSHIT\n");
}



}
/*
   CountdownTimer lasted(5.0f);
      CountdownTimer serverEnd(5.0f + lasted.getSecondsLeft());

    Window window;
    window.create("RubberBandBattle-Shell", 1280, 720);

    std::string username = "usernameEmpty";

    Camera main_cam;
    main_cam.position = {0.0f, 0.0f};
    main_cam.zoom     = 1.723f;

    Timer timer;
    //FPS debug thing
#ifdef _DEBUG
    float fpsTimer = 0.0f, currentFPS = 0.0f;
    int   fpsFrames = 0;
#endif
    lasted.update();
    Player player;
    // Init assets
     UI::_font::load("Resources/Font/Confale.ttf");
    Image* playersheet = PL_LoadImage("Resources/Skins/spritemap.png");
    Atlas* playeratlas = LoadAtlas("Resources/Skins/spritemap.json");
    player.anim = CreateAnimator();
    player.anim->Load("Resources/Skins/Animation.json");
    Image* ground =PL_LoadImage("Resources/Texture/BGDebug.png");

    player.sprite                 = CreateSprite();
    player.sprite->image          = playersheet;
    player.sprite->atlas          = playeratlas;
    player.sprite->frameName      = "0001";
    player.sprite->position       = {player.x, player.y};
    player.sprite->targetPosition = {player.x, player.y};
    player.sprite->scale          = {1.0f, 1.0f};


    Anim(player.anim, PLAYER, IDLE);
    Sound* bgm =  CreateSound();
    bgm->load("bgm.wav");
    Sprite* bg         = CreateSprite();
    bg->image          = ground;
    bg->position       = {0.0f,0.0f};
   bg->skewX          = 0.0f;
*/
