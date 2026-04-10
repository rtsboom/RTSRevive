#pragma once
#include <memory>

namespace rrv
{

	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		bool Init(void* hwnd, int width, int height);
		void Shutdown();
		void Render();

		void SetCamera(rrv::Vec3 eyePos, rrv::Vec3 focusPos);

		// just for testing, need to remove later
		void Test_UpdateInstances(float deltaTime);
		void Test_UploadDDSTexture();

	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}