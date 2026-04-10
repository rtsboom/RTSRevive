#pragma once
#include "Window.h"
#include "Timer.h"
#include "Camera.h"
#include "Game.h"
#include <Renderer/Renderer.h>
#include <AssetManager/AssetManager.h>
namespace rrv
{

	class App
	{
	public:
		bool Init();
		void Run();
		void Shutdown();
	private:
		void Update(float dt);
		void Render();

		Window   m_window;
		Timer    m_timer;
		std::unique_ptr<Game> m_game;

		Renderer m_renderer;
		AssetManager m_asset_manager;
		Camera   m_camera;
	};
}
