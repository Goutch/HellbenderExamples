#include "HBE.h"
#include "CubeScene.h"

using namespace HBE;
bool fullscreen = false;

void onAppUpdate(float delta) {
	if (Input::getKeyDown(KEY_ESCAPE)) {
		Application::quit();
	}
	if (Input::getKeyDown(KEY_F11)) {
		fullscreen = !fullscreen;
		Graphics::getWindow()->setFullscreen(fullscreen);
	}
	if (Input::getKeyDown(KEY_V)) {
		Configs::setVerticalSync(!Configs::getVerticalSync());
	}
}


int main() {
	ApplicationInfo app_info{};
	Application::init(app_info);
	//-----------------------SETUP--------------------
	Configs::setWindowTitle("Pong");
	{
		//-----------------------Games--------------------
		CubeScene cubeScene = CubeScene();
		//-----------------------EVENTS------------------
		Application::onUpdate.subscribe(&onAppUpdate);
		//-----------------------LOOP--------------------
		Application::run();
		//-----------------------CLEANUP------------------
		Application::onUpdate.unsubscribe(&onAppUpdate);
		//-----------------------TERMINATE------------------
	}
	Application::terminate();
}