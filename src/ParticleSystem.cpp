#include "ParticleSystem.hpp"

Particle::Particle(Vector2 &pos)
    : position{pos}
{
    acc = {0.0f, 0.05f};
    velocity = {(float)GetRandomValue(-5, 5) * .1f, (float)GetRandomValue(-5, 30) * .1f};
    size = {10.0f};
}

bool Particle::update()
{
    velocity = Vector2Add(velocity, acc);
    position = Vector2Add(position, velocity);
    if (GetRandomValue(0, 100) < 30)
        size -= 1;
    return size <= 0;
}

void Particle::draw()
{
    DrawRectangle(position.x, position.y, size, size, BLACK);
}

ParticleSystem::ParticleSystem(Vector2 &pos)
{
    for (int i = 0; i < 10; i++)
        particles.emplace_back(pos);
}

void ParticleSystem::update()
{
    particles.erase(
        std::remove_if(particles.begin(), particles.end(), [](auto &p)
                       { return p.update(); }),
        particles.end());
}

bool ParticleSystem::draw()
{
    for (std::vector<Particle>::iterator particle = particles.begin(); particle < particles.end(); particle++)
        (*particle).draw();
    return particles.size() <= 0;
}