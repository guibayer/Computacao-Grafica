/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#include <algorithm>
#include <sstream>

#include "game.h"

#include <time.h>

#include "resource_manager.h"
#include "sprite_renderer.h"
#include "game_object.h"
#include "ball_object_collisions.h"
#include "text_renderer.h"

// Game-related State data
SpriteRenderer  *Renderer;
BallObject      *Ghost;
BallObject     *Ball;
TextRenderer  *Text;

Game::Game(GLuint width, GLuint height)
	: State(GAME_MENU), Keys(), Width(width), Height(height), Level(0), Score(0) {

}

Game::~Game()
{
	delete Renderer;
	//delete Player;
}
GLboolean CheckCollision(BallObject &one, GameObject &two);

void Game::Init()
{
	// Load shaders
	ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
	// Configure shaders
	glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(this->Width),
		static_cast<GLfloat>(this->Height), 0.0f, -1.0f, 1.0f);
	ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
	ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
	// Set render-specific controls
	Shader myShader = ResourceManager::GetShader("sprite");
	Renderer = new SpriteRenderer(myShader);
	// Load textures
	ResourceManager::LoadTexture("textures/background.jpg", GL_FALSE, "background");
	ResourceManager::LoadTexture("textures/awesomeface.png", GL_TRUE, "awesome");
	ResourceManager::LoadTexture("textures/block.png", GL_FALSE, "block");
	ResourceManager::LoadTexture("textures/block_solid.png", GL_FALSE, "block_solid");
	ResourceManager::LoadTexture("textures/dot.jpg", GL_FALSE, "dot");
	ResourceManager::LoadTexture("textures/pacman.png", GL_FALSE, "pacman");
	// Load levels
	GameLevel one; 
	one.Load("levels/one.lvl", this->Width, this->Height);
	GameLevel two; 
	two.Load("levels/two.lvl", this->Width, this->Height * 0.5);
	//GameLevel three; three.Load("levels/three.lvl", this->Width, this->Height * 0.5);
	//GameLevel four; four.Load("levels/four.lvl", this->Width, this->Height * 0.5);
	this->Levels.push_back(one);
	//this->Levels.push_back(two);
	//this->Levels.push_back(three);
	//this->Levels.push_back(four);
	this->Level = 0;
	this->Lives = 3;
	// Configure game objects
	glm::vec2 playerPos = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
	//Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("pacman"));
	// Ball configuration 
	glm::vec2 ballPos(this->Width / 2 - 15, this->Height - 112);
	Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY,
		ResourceManager::GetTexture("pacman"));
	glm::vec2 ghostPos(this->Width / 2, this->Height / 2 - 24);
	Ghost = new BallObject(ghostPos, BALL_RADIUS, INITIAL_BALL_VELOCITY,
		ResourceManager::GetTexture("awesome"));

	Text = new TextRenderer(this->Width, this->Height);
	Text->Load("fonts/ocraext.TTF", 24);

}

void Game::Update(GLfloat dt)
{
	// Update objects
	Ball->Move(dt, this->Width);
	// Check for collisions
	this->DoCollisions(dt);
}


void Game::ProcessInput(GLfloat dt)
{
	if (this->State == GAME_ACTIVE)
	{
		GLfloat velocity = PLAYER_VELOCITY * dt;
		// Move playerboard
		if (this->Keys[GLFW_KEY_A])
		{
			Ball->Position.x -= velocity;
			for (GameObject &box : this->Levels[this->Level].Bricks)
			{
				if (CheckCollision(*Ball, box))
				{
					if (box.IsSolid) {
						Ball->Position.x += velocity;
					}
				}
			}
		}
		if (this->Keys[GLFW_KEY_D])
		{
			Ball->Position.x += velocity;
			for (GameObject &box : this->Levels[this->Level].Bricks)
			{
				if (CheckCollision(*Ball, box))
				{
					if (box.IsSolid) {
						Ball->Position.x -= velocity;
					}
				}
			}
		}
		if (this->Keys[GLFW_KEY_W])
		{
			Ball->Position.y -= velocity;
			for (GameObject &box : this->Levels[this->Level].Bricks)
			{
				if (CheckCollision(*Ball, box))
				{
					if (box.IsSolid) {
						Ball->Position.y += velocity;
					}
				}
			}
		}
		if (this->Keys[GLFW_KEY_S])
		{
			Ball->Position.y += velocity;
			for (GameObject &box : this->Levels[this->Level].Bricks)
			{
				if (CheckCollision(*Ball, box))
				{
					if (box.IsSolid) {
						Ball->Position.y -= velocity;
					}
				}
			}
		}
	}
	else {
		// Move playerboard
		if (this->Keys[GLFW_KEY_ENTER])
		{
			this->State = GAME_ACTIVE;
		}
	}
}
/*
GLboolean CheckCollision(GameObject &one, GameObject &two) // AABB - AABB collision
{
	// Collision x-axis?
	bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
		two.Position.x + two.Size.x >= one.Position.x;
	// Collision y-axis?
	bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
		two.Position.y + two.Size.y >= one.Position.y;
	// Collision only if on both axes
	return collisionX && collisionY;
}
*/
GLboolean CheckCollision(BallObject &one, GameObject &two) // AABB - Circle collision
{
	// Get center point circle first 
	glm::vec2 center(one.Position + one.Radius);
	// Calculate AABB info (center, half-extents)
	glm::vec2 aabb_half_extents(two.Size.x / 2, two.Size.y / 2);
	glm::vec2 aabb_center(
		two.Position.x + aabb_half_extents.x,
		two.Position.y + aabb_half_extents.y
	);
	// Get difference vector between both centers
	glm::vec2 difference = center - aabb_center;
	glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
	// Add clamped value to AABB_center and we get the value of box closest to circle
	glm::vec2 closest = aabb_center + clamped;
	// Retrieve vector between center circle and closest point AABB and check if length <= radius
	difference = closest - center;
	return glm::length(difference) < one.Radius;
}


GLboolean CheckCollisionGhost(BallObject &one, BallObject &two) // AABB - Circle collision
{
	// Get center point circle first 
	glm::vec2 center(one.Position + one.Radius);
	// Calculate AABB info (center, half-extents)
	glm::vec2 aabb_half_extents(two.Size.x / 2, two.Size.y / 2);
	glm::vec2 aabb_center(
		two.Position.x + aabb_half_extents.x,
		two.Position.y + aabb_half_extents.y
	);
	// Get difference vector between both centers
	glm::vec2 difference = center - aabb_center;
	glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
	// Add clamped value to AABB_center and we get the value of box closest to circle
	glm::vec2 closest = aabb_center + clamped;
	// Retrieve vector between center circle and closest point AABB and check if length <= radius
	difference = closest - center;
	return glm::length(difference) < one.Radius;
}

void Game::DoCollisions(GLfloat dt)
{
	GLfloat velocity = PLAYER_VELOCITY * dt;
	for (GameObject &box : this->Levels[this->Level].Bricks)
	{
		if (!box.Destroyed)
		{
			if (CheckCollision(*Ball, box))
			{ 
				if (!box.IsSolid) {
					box.Destroyed = GL_TRUE;
					Score++;
					printf("Score: %d \n", Score);
					if (Score == 73) {
						this->State = GAME_WIN;
					}
				}
			}
		}
	}
	srand(time(NULL));
	int v1 = rand() % 600;
	if (v1 % 2 == 1) {
		Ghost->Position.x -= 1;
		for (GameObject &box : this->Levels[this->Level].Bricks)
		{
			if (CheckCollision(*Ghost, box))
			{
				if (box.IsSolid) {
					Ghost->Position.x += 1;
				}
			}
		}
	}
	else if(v1 % 2 == 0)
	{
		Ghost->Position.x += 1;
		for (GameObject &box : this->Levels[this->Level].Bricks)
		{
			if (CheckCollision(*Ghost, box))
			{
				if (box.IsSolid) {
					Ghost->Position.x -= 1;
				}
			}
		}
	}

	if (CheckCollisionGhost(*Ghost, *Ball))
	{
		if (this->Lives <= 1) {
			this->State = GAME_LOOSE;
		}
		else {
			this->Lives--;
			this->State = GAME_ACTIVE;
			glm::vec2 ballPos(this->Width / 2 - 15, this->Height - 112);
			Ball->Position = ballPos;
		}
	}
	
}

void Game::Render()
{
	if (this->State == GAME_ACTIVE)
	{
		// Draw background
		Texture2D myTexture2D = ResourceManager::GetTexture("background");
		Renderer->DrawSprite(myTexture2D,
			glm::vec2(0, 0), glm::vec2(this->Width, this->Height), 0.0f
		);

		// Draw level
		this->Levels[this->Level].Draw(*Renderer);
		// Draw player
		Ghost->Draw(*Renderer);
		// Draw Ball
		Ball->Draw(*Renderer);
		std::stringstream ss, ssLives;
		ss << this->Lives;
		Text->RenderText("Lives:" + ss.str(), 5.0f, 5.0f, 1.0f);
		ssLives << this->Score;
		Text->RenderText("Score:" + ssLives.str(), this->Width - 120.0f, 5.0f, 1.0f);
	}
	if (this->State == GAME_MENU)
	{
		Text->RenderText("Press ENTER to start", 250.0f, Height / 2, 1.0f);
		Text->RenderText("Use AWDS to move the Pacman", 245.0f, Height / 2 + 20.0f, 0.75f);
	}
	if (this->State == GAME_WIN)
	{
		Text->RenderText("YOU WIN TUTS TUTS TUTS!!!!!", 250.0f, Height / 2, 1.0f);
		Text->RenderText("Press ESC to quit", 250.0f, Height / 2 + 20.0f, 0.75f);
	}
	if (this->State == GAME_LOOSE)
	{
		Text->RenderText("DAMN MAN, YOU LOOSE!!!", 220.0f, Height / 2, 1.0f);
		Text->RenderText("Press ESC to quit", 250.0f, Height / 2 + 20.0f, 0.75f);
	}
}