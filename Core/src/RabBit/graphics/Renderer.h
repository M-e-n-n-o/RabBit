#pragma once

namespace RB::Graphics
{
	enum class RenderAPI
	{
		None,
		D3D12
	};

	class RenderInterface;

	enum class RenderThreadState
	{
		Idle,
		Running,
		Waking,
		Shutdown,
		Terminated
	};

	// Fully static class
	class Renderer
	{
	public:
		Renderer();
		virtual ~Renderer();

		static void SetAPI(RenderAPI api) { s_Api = api; }
		inline static RenderAPI GetAPI() { return s_Api; }

		// We do not want the renderer to be fully implemented per render API to avoid duplicate code, 
		// so we want as little pure virtual methods as needed (maybe just the constructor and destructor is already good enough).
		
		//void SubmitViewContext();

		void StartRenderFrame();
		void FinishRenderFrame();


		static Renderer* Create();

	protected:
		virtual RenderInterface* GetRenderInterface() = 0;
		virtual uint32_t ExecuteRenderInterface() = 0;
		virtual void SyncCpuWithRenderInterface(uint32_t id) = 0;
		virtual void SyncGpuWithRenderInterface(uint32_t id) = 0;

	private:
		inline static RenderAPI s_Api = RenderAPI::None;

		extern friend DWORD WINAPI RenderLoop(PVOID param);

		HANDLE				m_RenderThread;
		CONDITION_VARIABLE	m_KickCV;
		CRITICAL_SECTION	m_KickCS;
		CONDITION_VARIABLE	m_SyncCV;
		CRITICAL_SECTION	m_SyncCS;

		RenderThreadState	m_RenderState;
	};
}