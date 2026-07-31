#pragma once

#include <vector>
#include <cstdint>
#include <iostream>
#include <cstdlib>

class Grid
{
public:
    // Editable settings
    int GRID_WIDTH;
    int GRID_HEIGHT;
    uint32_t RANGE;

    Grid(int width, int height, uint32_t range)
    {
        GRID_WIDTH = width;
        GRID_HEIGHT = height;
        RANGE = range;

        m_data.resize(GRID_WIDTH * GRID_HEIGHT);
    }

    void set(int x, int y, uint32_t value)
    {
        m_data[y * GRID_WIDTH + x] = value;
    }

    uint32_t get(int x, int y) const
    {
        return m_data[y * GRID_WIDTH + x];
    }

    void fill(uint32_t value)
    {
        for (uint32_t& cell : m_data)
            cell = value;
    }

    void randomize()
    {
        for (uint32_t& cell : m_data)
            cell = rand() % RANGE;
    }

    void print() const
    {
        for (int y = 0; y < GRID_HEIGHT; y++)
        {
            for (int x = 0; x < GRID_WIDTH; x++)
            {
                std::cout << get(x, y) << " ";
            }
            std::cout << '\n';
        }
    }

private:
    std::vector<uint32_t> m_data;
};
