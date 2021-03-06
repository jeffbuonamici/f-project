// C/C++
#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>

// includes
#include "libraries/mat4.h"
#include "libraries/vec3.h"

#include "matty/application.h"
#include "matty/platform.h"
#include "matty/shader.h"
#include "matty/planebufferedgeometry.h"
#include "matty/terraingeometry.h"
#include "matty/camera.h"
#include "matty/texturemanager.h"
#include "libraries/biscuit.h"
#include "skybox.h"


namespace shaders {

std::unordered_map<std::string, std::shared_ptr<Shader>> shader;

void initialize() {

	auto terrain = biscuit::createShader(
		"uniform sampler2D uf_Ice;" \
		"uniform sampler2D uf_Stone;" \
		"uniform sampler2D uf_Rock;" \
		"uniform sampler2D uf_Grass;" \
		"uniform sampler2D uf_Sand;" \
		"vec4 effect(vec4 vcolor) {" \
		"vec4 terrainColor = vec4(0.0, 0.0, 0.0, 1.0);" \
		"float height = vcolor.a;" \
		"float regionMin = 0.0;" \
		"float regionMax = 0.0;" \
		"float regionRange = 0.0;" \
		"float regionWeight = 0.0;" \
		"/* ice */" \
		"regionMin = 0.75;" \
		"regionMax = 1.0;" \
		"regionRange = regionMax - regionMin;" \
		"regionWeight = (regionRange - abs(height - regionMax)) / regionRange;" \
		"regionWeight = max(0.0, regionWeight);" \
		"terrainColor += regionWeight * texture(uf_Ice, TexCoord * 16.0);" \
		"/* stone */" \
		"regionMin = 0.30;" \
		"regionMax = 0.8;" \
		"regionRange = regionMax - regionMin;" \
		"regionWeight = (regionRange - abs(height - regionMax)) / regionRange;" \
		"regionWeight = max(0.0, regionWeight);" \
		"terrainColor += regionWeight * texture(uf_Stone, TexCoord * 16.0);" \
		"/* grass */" \
		"regionMin = 0.0;" \
		"regionMax = 0.32;" \
		"regionRange = regionMax - regionMin;" \
		"regionWeight = (regionRange - abs(height - regionMax)) / regionRange;" \
		"regionWeight = max(0.0, regionWeight);" \
		"terrainColor += regionWeight * texture(uf_Grass, TexCoord * 16.0);" \
		"/* sand */" \
		"regionMin = 0.0;" \
		"regionMax = 0.15;" \
		"regionRange = regionMax - regionMin;" \
		"regionWeight = (regionRange - abs(height - regionMax)) / regionRange;" \
		"regionWeight = max(0.0, regionWeight);" \
		"terrainColor += regionWeight * texture(uf_Sand, TexCoord * 16.0);" \
		"return terrainColor;" \
		"}"
		);

	auto water = biscuit::createShader(
		"uniform float uf_Time;" \
		"out float uTime;" \
		"float calculateSurface(float x, float z) {" \
		"float y = 0.0;" \
		"y += (sin(x * 1.0 / 10 + uf_Time * 1.0) + sin(x * 2.3 / 10 + uf_Time * 1.5) + sin(x * 3.3 / 10 + uf_Time * 0.4)) / 3.0;" \
		"y += (sin(z * 0.2 / 10 + uf_Time * 1.8) + sin(z * 1.8 / 10 + uf_Time * 1.8) + sin(z * 2.8 / 10 + uf_Time * 0.8)) / 3.0;" \
		"return y;" \
		"}" \
		"vec4 position(mat4 transform_proj, vec4 vertpos) {" \
		"uTime = uf_Time;" \
		"vec4 pos = vertpos;" \
		"pos.y += 1.0 * calculateSurface(pos.x, pos.z);" \
		"pos.y -= 1.0 * calculateSurface(0.0, 0.0);" \
		"return (transform_proj * pos);" \
		"}",

		"in float uTime;" \
		"uniform sampler2D uf_Image;" \
		"vec4 effect(vec4 vcolor) {" \
		"vec2 uv = TexCoord * 16.0 + vec2(uTime * -0.05);" \
		"uv.y += 0.01 * (sin(uv.x * 3.5 + uTime * 0.35) + sin(uv.x * 4.8 + uTime * 1.05) + sin(uv.x * 7.3 + uTime * 0.45)) / 3.0;" \
		"uv.x += 0.12 * (sin(uv.y * 4.0 + uTime * 0.5) + sin(uv.y * 6.8 + uTime * 0.75) + sin(uv.y * 11.3 + uTime * 0.2)) / 3.0;" \
		"uv.y += 0.12 * (sin(uv.x * 4.2 + uTime * 0.64) + sin(uv.x * 6.3 + uTime * 1.65) + sin(uv.x * 8.2 + uTime * 0.45)) / 3.0;" \
		"vec4 tex1 = texture(uf_Image, uv * 1.0);" \
		"vec4 tex2 = texture(uf_Image, uv * 1.0 + vec2(0.2));" \
		"vec3 blue = vec3(0.20, 0.59, 0.85);" \
		"return vec4(blue + vec3(tex1.a * 0.9 - tex2.a * 0.02), 1.0);" \
		"}"
		);

	// the vertex shader uses a hack to removed translations
	auto skybox = biscuit::createShader(
		"vec4 position(mat4 transform_proj, vec4 vertpos) {" \
		"mat3 view3 = mat3(uView);" \
		"mat4 view4 = mat4(view3);" \
		"return uProjection * view4 * vertpos;" \
		"}",

		"uniform samplerCube skybox;"
		"vec4 effect(vec4 vcolor) {" \
		"return texture(skybox, VertPosition.xyz);" \
		"}"
	);

	Shader::ShaderSource terrain_shader{ terrain.first, terrain.second };
	Shader::ShaderSource water_shader{ water.first, water.second };
	Shader::ShaderSource skybox_shader{ skybox.first, skybox.second };

	shader["terrain"] = std::make_shared<Shader>(terrain_shader);
	shader["water"] = std::make_shared<Shader>(water_shader);
	shader["skybox"] = std::make_shared<Shader>(skybox_shader);
}
}

namespace game {

	Config* gameConfig;

	// Camera
  Camera  camera({ 0.0f, 15.0f, 3.0f });
	GLfloat lastX = 0.0f;
	GLfloat lastY = 0.0f;
	bool    keys[1024];

	// Light attributes
	vec3<float> lightPos(1.2f, 1.0f, 2.0f);

	// Deltatime
	GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
	GLfloat lastFrame = 0.0f;  	// Time of last frame

	/// Handle mouse input
	bool mouseButtonLeftDown = false;
	double mousePosY = 0;
	double mousePosY_down = 0;

	mat4f projection = mat4f::perspective(45.0f, ((float)600 / (float)533), 0.1f, 100.0f);
	mat4f transform = camera.GetViewMatrix();
	//mat4f transform = mat4f::lookAt(vec3f(4.0f, 25.0f, -100.0f), vec3f(0.0f, 0.0f, 0.0f), vec3f(0.0f, 1.0f, 0.0f));
	//prototype
	void initGameSettings();
	void do_movement();

	void init(Config* config) {
		config->window.width = 600;
		config->window.height = 533;
		config->window.decorated = true;
		config->window.floating = false;
		config->window.focused = false;
		config->window.resizable = false;
		config->window.vsyn = true;
		gameConfig = config;
	}

	void initGameSettings() {
		GLfloat lastX = gameConfig->window.width / 2.0;
		GLfloat lastY = gameConfig->window.height / 2.0;
	}

TextureManager* textureManager = new TextureManager;

PlaneBufferedGeometry water;
TerrainGeometry plane;
Skybox skybox;


Texture* ice_texture;
Texture* stone_texture;
Texture* rock_texture;
Texture* grass_texture;
Texture* sand_texture;
Texture* water_texture;
Texture skybox_cubemap;

void load() {
	printf("Load\n");
	shaders::initialize();

	water = PlaneBufferedGeometry(100, 100, 100, 100);
	plane = TerrainGeometry(100, 100, 100, 100);
    skybox.initialize();

	skybox_cubemap = textureManager->getCubeMap({
		"right.jpg", "left.jpg",
		"top.jpg", "bottom.jpg",
		"back.jpg", "front.jpg"
	});

	ice_texture = textureManager->get("ice.png");
	stone_texture = textureManager->get("stone.png");
	rock_texture = textureManager->get("rock.png");
	grass_texture = textureManager->get("grass.png");
	sand_texture = textureManager->get("sand.png");
	water_texture = textureManager->get("water.png");
}

void update() {
	// Calculate deltatime of current frame
	GLfloat currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	do_movement();
}

void do_movement()
{
	// Camera controls
	if (keys[GLFW_KEY_W])
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D])
		camera.ProcessKeyboard(RIGHT, deltaTime);
}


void draw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	transform = camera.GetViewMatrix();

	// skybox
	glDepthMask(GL_FALSE);
	auto skybox_shader = shaders::shader["skybox"];
	skybox_shader->attach();
	skybox_shader->setMatrix4("uProjection", projection);
	skybox_shader->setMatrix4("uView", transform);
	skybox_shader->setMatrix4("uModel", mat4f::identity());
	skybox_shader->setInteger("skybox", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_cubemap.handle);

    skybox.render();
	glDepthMask(GL_TRUE);

	// water
	auto water_shader = shaders::shader["water"];
	water_shader->attach();
	water_shader->setMatrix4("uProjection", projection);
	water_shader->setMatrix4("uView", transform);
	water_shader->setMatrix4("uModel", mat4f::identity());
	water_shader->setInteger("uf_Image", 0);
	water_shader->setFloat("uf_Time", (float)glfwGetTime());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, water_texture->handle);

	water.render();

	// terrain
	auto terrain_shader = shaders::shader["terrain"];
	terrain_shader->attach();
	terrain_shader->setMatrix4("uProjection", projection);
	terrain_shader->setMatrix4("uView", transform);
	terrain_shader->setMatrix4("uModel", mat4f::identity());
	terrain_shader->setInteger("uf_Ice", 0);
	terrain_shader->setInteger("uf_Stone", 1);
	terrain_shader->setInteger("uf_Rock", 2);
	terrain_shader->setInteger("uf_Grass", 3);
	terrain_shader->setInteger("uf_Sand", 4);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ice_texture->handle);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, stone_texture->handle);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, rock_texture->handle);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, grass_texture->handle);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, sand_texture->handle);

	plane.render();
}

	void quit() {
		printf("Quit\n");
	}

	void mousePressed(int x, int y, int button) {
		printf("Mouse Pressed\n");
		mouseButtonLeftDown = true;
		mousePosY_down = y;
	}

	void mouseReleased(int x, int y, int button) {
		printf("Mouse Released\n");
		mouseButtonLeftDown = false;
	}

	void keyPressed(int key, int scancode) {
		printf("Key Pressed\n");
		keys[key] = true;
		std::cout << "key: " << key << std::endl;
		switch (key)
		{
			//the user can change the rendering mode i.e. points, lines, triangles based on keyboard input
			case GLFW_KEY_P: {
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			}break;
			case GLFW_KEY_O: {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}break;
			case GLFW_KEY_T: {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}break;
		}
	}

	void keyReleased(int key, int scancode) {
		printf("Key Released\n");
		keys[key] = false;
	}

	bool firstMouse = true;
	void cursor_callback( double xpos, double ypos)
	{
		if (mouseButtonLeftDown) {

			if (firstMouse)
			{
				lastX = xpos;
				lastY = ypos;
				firstMouse = false;
			}

			GLfloat xoffset = xpos - lastX;
			GLfloat yoffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to left

			camera.ProcessMouseMovement(xoffset, yoffset);
		}
		lastX = xpos;
		lastY = ypos;
	}

}	// game


	int main(int argc, const char** argv) {
		Application* app = new Application();

#if defined(DEBUG)
		printf("Debug Build\n");
#endif

		app->init = &game::init;
		app->load = &game::load;
		app->update = &game::update;
		app->draw = &game::draw;
		app->quit = &game::quit;
		app->mousePressed = &game::mousePressed;
		app->mouseReleased = &game::mouseReleased;
		app->keyPressed = &game::keyPressed;
		app->keyReleased = &game::keyReleased;
		app->mouseMotion = &game::cursor_callback;
		boot(app);
		delete app;
		return 0;
	}
