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
		static RenderAPI GetAPI() { return s_Api; }

		// We do not want the renderer to be fully implemented per render API to avoid duplicate code, 
		// so we want as little pure virtual methods as needed (maybe just the constructor and destructor is already good enough).
		
		void StartRenderFrame();
		void FinishRenderFrame();

		//void RenderContexts(RenderEntry* entries);
		//void AddContext(RenderContext* context);
		//void RemoveContext(RenderContext* context);

		static Renderer* Create();

	protected:
		RenderInterface* GetRenderInterface();

		RenderInterface* m_RenderInterface;
		//RenderInterface* m_ComputeInterface;

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