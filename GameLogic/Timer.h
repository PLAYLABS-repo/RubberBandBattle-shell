#ifndef TIME_H
#define TIME_H

#include <chrono>

class CountdownTimer
{
private:
    float duration;
    float timeLeft;
    std::chrono::steady_clock::time_point lastTime;

public:
    CountdownTimer(float seconds = 0.0f)
    {
        start(seconds);
    }

    void start(float seconds)
    {
        duration = seconds;
        timeLeft = seconds;
        lastTime = std::chrono::steady_clock::now();
    }

    void update()
    {
        auto now = std::chrono::steady_clock::now();

        float deltaTime = std::chrono::duration<float>(now - lastTime).count();

        lastTime = now;

        if (timeLeft > 0.0f)
        {
            timeLeft -= deltaTime;

            if (timeLeft < 0.0f)
                timeLeft = 0.0f;
        }
    }

    float getTimeLeft() const
    {
        return timeLeft;
    }

    int getSecondsLeft() const
    {
        return static_cast<int>(timeLeft + 0.999f);
    }

    bool finished() const
    {
        return timeLeft <= 0.0f;
    }

    void reset()
    {
        start(duration);
    }
};

#endif
