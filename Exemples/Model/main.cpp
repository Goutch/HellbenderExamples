#include "HBE.h"
#include "ModelScene.h"

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

bool foo(int a, int b) {
	return a > b;
}

class Test {
public:
	int add(int a, int b) {
		return a + b;
	}
};

int main() {

	ApplicationInfo app_info{};
	app_info.name = "Model";
	app_info.required_extension_flags = VULKAN_REQUIRED_EXTENSION_DESCRIPTOR_INDEXING;
	app_info.vulkan_version = VULKAN_VERSION_1_3;
	Application::init(app_info);
	//-----------------------SETUP--------------------
	{
		//-----------------------Games--------------------
		ModelScene model_scene = ModelScene();
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