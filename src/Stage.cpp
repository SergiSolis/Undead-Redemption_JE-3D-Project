#include "Stage.h"
#include "game.h"
#include "utils.h"
#include "fbo.h"
#include <cmath>
#include "animation.h"
#include "input.h"
#include "Gamemap.h"

void playStage::render()
{
	Game* game = Game::instance;
	World world = game->world;
	EntityPlayer* player = world.player;
	// loadMesh();

	// set the clear color (the background color)
	glClearColor(0.0, 0.0, 0.0, 1.0);

	// set flags
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// Clear the window and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set the camera as default
	game->camera->enable();

	glDisable(GL_DEPTH_TEST);
	world.skyModel.translate(game->camera->eye.x, game->camera->eye.y - 10.0f, game->camera->eye.z);
	renderMesh(GL_TRIANGLES, world.skyModel, world.sky->mesh, world.sky->texture, world.sky->shader, game->camera);
	//world.sky->render();
	glEnable(GL_DEPTH_TEST);

	player->model = Matrix44();
	player->model.translate(player->mov.pos.x, player->mov.pos.y, player->mov.pos.z);
	player->model.rotate(player->mov.jaw * DEG2RAD, Vector3(0, 1, 0));

	setCamera(game->camera, player->model);

	world.ground->model = Matrix44();
	world.ground->tiling = 500.0f;
	renderMesh(GL_TRIANGLES, world.ground->model, world.ground->mesh, world.ground->texture, world.ground->shader, game->camera, world.ground->tiling);
	//world.ground->render();
	
	//player->render();
	renderMesh(GL_TRIANGLES, player->model, player->mesh->mesh, player->mesh->texture, player->mesh->shader, game->camera);

	for (size_t i = 0; i < world.static_entities.size(); i++)
	{
		EntityMesh* entity = world.static_entities[i];
		renderMesh(GL_TRIANGLES, entity->model, entity->mesh, entity->texture, entity->shader, game->camera);
		//entity->render();
	}
	/*
	for (size_t i = 0; i < world.gamemap->width; i++)
	{
		for (size_t j = 0; j < world.gamemap->height; j++)
		{
			sCell& cell = world.gamemap->getCell(i, j);
			int index = (int)cell.type;
			sPropViewData& prop = world.gamemap->viewData[index];


			Matrix44 cellModel;
			cellModel.translate(i * world.tileWidth, 0.0f, j * world.tileHeight);
			renderMesh(GL_TRIANGLES, cellModel, prop.mesh, prop.texture, world.shader, game->camera);
		}
	}
	*/
	//Draw the floor grid
	drawGrid();

	//render the FPS, Draw Calls, etc
	drawText(2, 2, getGPUStats(), Vector3(1, 1, 1), 2);

	//swap between front buffer and back buffer
	SDL_GL_SwapWindow(game->window);
}

void playStage::update(float seconds_elapsed)
{

	Game *game = Game::instance;
	World world = game->world;
	EntityPlayer* player = world.player;

	float speed = seconds_elapsed * 10; // the speed is defined by the seconds_elapsed so it goes constant

	//example
	world.angle += (float)seconds_elapsed * 10.0f;

	if (Input::wasKeyPressed(SDL_SCANCODE_TAB)) {
		player->cameraLocked = !player->cameraLocked;
		std::cout << "cameraLocked: " << player->cameraLocked << std::endl;
	}
	if (player->cameraLocked)
	{
		float playerSpeed = 20.0f * seconds_elapsed;
		float rotateSpeed = 120.0f * seconds_elapsed;
		float gravity = 9.807f;
		float jumpHeight = 5.0f;
		float verticalSpeed;

		if (player->firstPerson)
		{
			player->mov.pitch -= Input::mouse_delta.y * 5.0f * seconds_elapsed;
			player->mov.jaw -= Input::mouse_delta.x * 5.0f * seconds_elapsed;
			Input::centerMouse();
			SDL_ShowCursor(false);
		}
		else {
			if (Input::isKeyPressed(SDL_SCANCODE_D)) player->mov.jaw += rotateSpeed;
			if (Input::isKeyPressed(SDL_SCANCODE_A)) player->mov.jaw -= rotateSpeed;
		}
		Matrix44 playerRotation;
		playerRotation.rotate(player->mov.jaw * DEG2RAD, Vector3(0, 1, 0));
		Vector3 forward = playerRotation.rotateVector(Vector3(0, 0, -1));
		Vector3 right = playerRotation.rotateVector(Vector3(1, 0, 0));
		Vector3 jump = playerRotation.rotateVector(Vector3(0, 1, 0));
		Vector3 playerVel;

		if (Input::isKeyPressed(SDL_SCANCODE_W)) playerVel = playerVel + (forward * playerSpeed);
		if (Input::isKeyPressed(SDL_SCANCODE_S)) playerVel = playerVel - (forward * playerSpeed);
		if (Input::isKeyPressed(SDL_SCANCODE_Q)) playerVel = playerVel - (right * playerSpeed);
		if (Input::isKeyPressed(SDL_SCANCODE_E)) playerVel = playerVel + (right * playerSpeed);

		
		if (Input::isKeyPressed(SDL_SCANCODE_SPACE) && player->isGrounded) {
			player->isGrounded = false;
			playerVel = playerVel + (jump * sqrtf(2.0f * gravity * jumpHeight));
		}

		if (player->isGrounded == false)
		{
			playerVel = playerVel - (jump * gravity * seconds_elapsed);
		}
		
		
		std::cout << "position x: " << player->mov.pos.x << ", y: " << player->mov.pos.y << ", z: " << player->mov.pos.z << std::endl;

		Vector3 targetPos = player->mov.pos + playerVel;
		player->mov.pos = checkCollision(targetPos);
		
	}
	else
	{
		if ((Input::mouse_state & SDL_BUTTON_LEFT)) //is left button pressed?
		{
			game->camera->rotate(Input::mouse_delta.x * 0.005f, Vector3(0.0f, -1.0f, 0.0f));
			game->camera->rotate(Input::mouse_delta.y * 0.005f, game->camera->getLocalVector(Vector3(-1.0f, 0.0f, 0.0f)));
		}
		SDL_ShowCursor(true);
		if (Input::isKeyPressed(SDL_SCANCODE_LSHIFT)) speed *= 10; //move faster with left shift
		if (Input::isKeyPressed(SDL_SCANCODE_W)) game->camera->move(Vector3(0.0f, 0.0f, 1.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_S)) game->camera->move(Vector3(0.0f, 0.0f, -1.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_A)) game->camera->move(Vector3(1.0f, 0.0f, 0.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_D)) game->camera->move(Vector3(-1.0f, 0.0f, 0.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_UP)) game->camera->move(Vector3(0.0f, 1.0f, 0.0f) * speed);
		if (Input::isKeyPressed(SDL_SCANCODE_DOWN)) game->camera->move(Vector3(0.0f, -1.0f, 0.0f) * speed);
		
	}


}

void renderMesh(int primitive, Matrix44& model, Mesh* a_mesh, Texture* tex, Shader* a_shader, Camera* cam, float tiling) {
	Game* game = Game::instance;
	assert(a_mesh != NULL, "mesh in renderMesh was null");
	if (!a_shader) return;

	//enable shader
	a_shader->enable();

	//upload uniforms
	a_shader->setUniform("u_color", Vector4(1, 1, 1, 1));
	a_shader->setUniform("u_viewprojection", cam->viewprojection_matrix);
	if (tex != NULL)
	{
		a_shader->setUniform("u_texture", tex, 0);
	}
	a_shader->setUniform("u_time", time);
	a_shader->setUniform("u_text_tiling", tiling);
	a_shader->setUniform("u_model", model);
	//do the draw call
	a_mesh->render(primitive);

	//disable shader
	a_shader->disable();

	if (!game->world.cameraLocked) {
		a_mesh->renderBounding(model);
	}
}

void setCamera(Camera *cam, Matrix44 model)
{
	Game* game = Game::instance;
	World world = game->world;
	EntityPlayer* player = world.player;

	if (player->cameraLocked)
	{
		Vector3 eye = model * Vector3(0.0f, 3.0f, 4.0f);
		Vector3 center = model * Vector3(0.0f, 0.0f, -10.0f);
		Vector3 up = model.rotateVector(Vector3(0.0f, 1.0f, 0.0f));

		if (player->firstPerson) {
			Matrix44 camModel = player->model;
			camModel.rotate(player->mov.pitch * DEG2RAD, Vector3(1, 0, 0));

			eye = player->model * Vector3(0.0f, 1.0f, -0.5f);
			center = eye + camModel.rotateVector(Vector3(0.0f, 0.0f, -1.0f));
			up = camModel.rotateVector(Vector3(0.0f, 1.0f, 0.0f));
		}
		cam->lookAt(eye, center, up);
	}
	
	
}

Vector3 checkCollision(Vector3 target)
{
	Game *game = Game::instance;
	World world = game->world;
	EntityPlayer* player = world.player;

	Vector3 coll;
	Vector3 collnorm;
	Vector3 pushAway;
	Vector3 returned;

	Vector3 centerCharacter = target + Vector3(0.0f, 1.0f, 0.0f);
	Vector3 bottomCharacter = target + Vector3(0.0f, 0.0f, 0.0f);


	if (game->world.static_entities.size() == 0) {
		if (game->world.ground->mesh->testSphereCollision(game->world.ground->model, bottomCharacter, 1, coll, collnorm))
		{
			std::cout << "COLLISION BOTTOM" << std::endl;
			player->isGrounded = true;
			target.y = coll.y;
		}
	}
	else {
		for (size_t i = 0; i < game->world.static_entities.size(); i++)
		{
			Vector3 posEnt = game->world.static_entities[i]->model.getTranslation();
			std::cout << "POSITION ENTITY: X:" << posEnt.x << ", Y: " << posEnt.y << ", Z: " << posEnt.z << std::endl;
			if (game->world.static_entities[i]->mesh->testSphereCollision(game->world.static_entities[i]->model, centerCharacter, 1, coll, collnorm))
			{
				if (player->mov.pos.y >= coll.y)
				{
					target.y = 2.25f;
					player->isGrounded = true;
				}
				else {
					pushAway = normalize(coll - centerCharacter) * game->elapsed_time;

					returned = player->mov.pos - pushAway;
					returned.y = player->mov.pos.y;
					return returned;
				}
			}
			else {
				if (game->world.ground->mesh->testSphereCollision(game->world.ground->model, bottomCharacter, 1, coll, collnorm))
				{
					std::cout << "COLLISION BOTTOM" << std::endl;
					player->isGrounded = true;
					target.y = coll.y;
				}
				else {
					player->isGrounded = false;
				}

			}
		}
	}

	return target;
}

Vector3 checkCollisionBottom(Vector3 target)
{
	Game* game = Game::instance;
	World world = game->world;
	EntityPlayer* player = world.player;

	Vector3 coll;
	Vector3 coll2;
	Vector3 collnorm;

	Vector3 bottomCharacter = target + Vector3(0.0f, 0.0f, 0.0f);



	return target;
}

void addEntityInFront(Camera* cam, const char* meshName, const char* textName) {

	Vector2 mouse = Input::mouse_position;

	Game* game = Game::instance;

	Vector3 dir = cam->getRayDirection(mouse.x, mouse.y, game->window_width, game->window_height);
	Vector3 rayOrigin = cam->eye;


	Vector3 spawnPos = RayPlaneCollision(Vector3(), Vector3(0, 1, 0), rayOrigin, dir);
	Matrix44 model;
	model.translate(spawnPos.x, spawnPos.y, spawnPos.z);
	model.scale(3.0f, 3.0f, 3.0f);
	EntityMesh* entity = new EntityMesh(GL_TRIANGLES, model, Mesh::Get(meshName), Texture::Get(textName), Game::instance->world.shader);
	Game::instance->world.static_entities.push_back(entity);
}

void rayPick(Camera* cam) {

	Vector2 mouse = Input::mouse_position;

	Game* game = Game::instance;
	World world = game->world;

	Vector3 dir = cam->getRayDirection(mouse.x, mouse.y, game->window_width, game->window_height);
	Vector3 rayOrigin = cam->eye;

	for (size_t i = 0; i < world.static_entities.size(); i++)
	{
		EntityMesh* entity = world.static_entities[i];
		Vector3 pos;
		Vector3 normal;
		if (entity->mesh->testRayCollision(entity->model, rayOrigin, dir, pos, normal))
		{
			std::cout << "RayPicked: " << entity->tiling << std::endl;
			world.selectedEntity = entity;
			break;
		}
	}
}


void rotateSelected(float angleDegrees) {
	Game* game = Game::instance;
	World world = game->world;
	EntityMesh* selectedEntity = world.selectedEntity;

	if (selectedEntity == NULL) {
		return;
	}
	else {
		std::cout << "aaaaaa: " << std::endl;
	}
	selectedEntity->model.rotate(angleDegrees * DEG2RAD, Vector3(0, 1, 0));
}