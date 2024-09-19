#pragma once
#include "main.hpp"

class Particle
{
private:
    Vector2 acc;
    Vector2 position;
    Vector2 velocity;
    float size;

public:
    Particle(Vector2 &pos);

    bool update();
    void draw();
};

class ParticleSystem
{
private:
    std::vector<Particle> particles;

public:
    ParticleSystem(Vector2 &pos);

    void update();
    bool draw();
};