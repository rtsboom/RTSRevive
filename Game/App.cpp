#include "pch.h"
#include "App.h"

namespace rrv
{

	bool App::Init()
	{
		if (!m_window.Init(1280, 720, L"RTSEngine"))
			return false;

		if (!m_renderer.Init(m_window.GetHandle(), 1280, 720))
			return false;

		m_asset_manager = AssetManager(&m_renderer);
		m_game = std::make_unique<Game>();

		// Asset Load
		m_asset_manager.LoadModelFromGLTF("Assets/Models/Duck/Duck.gltf");






		return true;
	}

	void App::Run()
	{
		MSG msg = {};
		m_timer.Reset();

		while (true)
		{
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT) return;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if (m_window.ShouldClose()) return;

			// frame capping to 480 FPS
			const float targetFrameTime = 1.0f / 480.f;

			m_timer.Tick();
			float deltaTime = m_timer.GetDeltaTime();

			// spin wait until we reach the target frame time
			while (deltaTime < targetFrameTime)
			{
				m_timer.Tick();
				deltaTime += m_timer.GetDeltaTime();
			}

			// get FPS and set it to window title
			static float timeElapsed = 0.f;
			static int frameCount = 0;

			timeElapsed += deltaTime;
			frameCount++;

			if (timeElapsed > 1.f)
			{
				float fps = frameCount / timeElapsed;
				wchar_t buffer[64];
				swprintf(buffer, 64, L"RTSRevive   FPS: %.0f", fps);
				SetWindowText(m_window.GetHandle(), buffer);

				frameCount = 0;
				timeElapsed = 0.f;
			}

			Update(deltaTime);
			m_renderer.Test_UpdateInstances(deltaTime); // just for testing, need to remove later
			Render();
		}
	}

	void App::Shutdown()
	{
		m_window.Shutdown();
		m_renderer.Shutdown();
	}

	void App::Update(float dt)
	{
		auto& input = m_window.GetInput();
		float speed = 200.f * dt;
		if (input.IsKeyDown('W')) m_camera.MoveCamera(0.0f, speed);
		if (input.IsKeyDown('S')) m_camera.MoveCamera(0.0f, -speed);
		if (input.IsKeyDown('A')) m_camera.MoveCamera(-speed, 0.0f);
		if (input.IsKeyDown('D')) m_camera.MoveCamera(speed, 0.0f);

		// Rotation
		if (input.IsMouseButtonDown(2))
		{
			m_camera.RotateCamera(
				input.GetMouseDeltaX() * dt * 5.f,
				input.GetMouseDeltaY() * dt * 5.f);
		}

		// Zoom
		if (input.GetMouseWheelDelta() != 0)
			m_camera.ZoomCamera(input.GetMouseWheelDelta() * dt * 20.f);

		input.ResetMouseDeltas();

	}

	void App::Render()
	{
		m_renderer.SetCamera(m_camera.GetEyePosition(), m_camera.GetFocusPosition());
		m_renderer.Render();
	}
}
