# Wolfenstein 3D Clone
I'm making a Wolfenstein 3D clone to get a better grip on 3D graphics programming. 
Whenever I attempted to write a 3D game in the past, I would get bogged down by the details about the rendering system; trying to make it as flexible as possible, easy to use, support multiple vertex formats, etc. Furthermore, I became pretty obsessed with making a flexible system for handling entities, levels, etc. I have, in the process of working on this clone, learned to let the structure emerge from how the code is used, not the other way around. This is also why most of the code is written in C-style; I usually spend a lot of time thinking about the dependencies between objects and how I can make things as decoupled as possible. Writing in this style makes this much easier to do than committing to a more object-oriented structure since the cross-cutting concerns are isolated to functions/procedures as opposed to percolating through a class hirearchy. That being said, I'll be refactoring things as I go along to try and compress down this code into something a little more reusable. Anyways, here are some screenshots:

## Week 1
![Alt text](screens/week1.png?raw=true "Week 1")
### Features
* Sprite animations
* Level + Bounding Box collision
* Dynamic doors (can open and close them)
* Level and entities being generated/loaded from text format
* Looking around + Moving forward/backward + Strafing

## Week 1 + Weekend
![Alt text](screens/week1weekend.png?raw=true "Week 1 + Weekend")
### Features
* Level + Bounding Box raycasting
* Bullet impacts
* Painting entities which shake and change texture upon being shot
* Enemies which follow you around and can be shot + killed

