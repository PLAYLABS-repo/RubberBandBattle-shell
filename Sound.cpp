#include "Sound.h"
#include <mmsystem.h>
#include <string>

#pragma comment(lib, "winmm.lib")

int Sound::counter = 0;

Sound::Sound()
{
    alias = "snd_" + std::to_string(counter++);
    opened = false;
}

Sound::~Sound()
{
    stop();
}

bool Sound::load(const char* path)
{
    if (!path) return false;

    file = path;

    // Close if already open
    if (opened)
    {
        std::string cmd = "close " + alias;
        mciSendStringA(cmd.c_str(), NULL, 0, NULL);
        opened = false;
    }

    // Open once
    std::string openCmd =
        "open \"" + file + "\" type mpegvideo alias " + alias;

    if (mciSendStringA(openCmd.c_str(), NULL, 0, NULL) != 0)
        return false;

    opened = true;
    return true;
}

void Sound::play(bool loop)
{
    if (!opened) return;

    // Always seek to start before playing
    std::string seekCmd = "seek " + alias + " to start";
    mciSendStringA(seekCmd.c_str(), NULL, 0, NULL);

    std::string playCmd = "play " + alias;
    if (loop)
        playCmd += " repeat";

    mciSendStringA(playCmd.c_str(), NULL, 0, NULL);
}

void Sound::stop()
{
    if (!opened) return;

    std::string cmd = "stop " + alias;
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);

    cmd = "close " + alias;
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);

    opened = false;
}
