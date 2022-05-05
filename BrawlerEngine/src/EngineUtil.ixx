module;
#include <memory>
#include <array>
#include <thread>
#include "DxDef.h"

export module Util.Engine;
import Brawler.D3DVideoBudgetInfo;
import Brawler.SettingID;
import Brawler.SettingsManager;
import Brawler.CommandListType;

export namespace Brawler
{
	class Renderer;
	class DisplayAdapter;
	class CommandQueue;
	class RenderJobManager;
	class PSOManager;
}

export namespace Util
{
	namespace Engine
	{
		Brawler::Renderer& GetRenderer();
		Brawler::RenderJobManager& GetRenderJobManager();
		Brawler::DisplayAdapter& GetDisplayAdapter();
		Brawler::DXGIAdapter& GetDXGIAdapter();
		Brawler::DXGIFactory& GetDXGIFactory();
		Brawler::D3D12Device& GetD3D12Device();
		Brawler::CommandQueue& GetCommandQueue(const Brawler::CommandListType cmdListType);
		Brawler::PSOManager& GetPSOManager();

		Brawler::D3DVideoBudgetInfo GetVideoBudgetInfo();

		std::thread::id GetMasterRenderThreadID();

		std::uint64_t GetCurrentUpdateTick();
		std::uint64_t GetCurrentFrameNumber();

		std::uint32_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE heapType);
		
		/// <summary>
		/// Checks to see if the calling thread is the master render thread. This should *ONLY* be used
		/// in Debug builds to ensure that data is not subject to race conditions; it should *NOT* be
		/// used to branch code paths, since this limits parallelism and complicates code!
		/// </summary>
		/// <returns>
		/// The function returns true if the calling thread is the master render thread and false
		/// otherwise.
		/// </returns>
		bool IsMasterRenderThread();
		
		/// <summary>
		/// Checks to see if resourceStates is a valid combination of D3D12_RESOURCE_STATES.
		/// </summary>
		/// <param name="resourceStates">
		/// - The resource states which will be checked.
		/// </param>
		/// <returns>
		/// The function returns true if resourceStates is a valid combination of D3D12_RESOURCE_STATES
		/// and false otherwise.
		/// </returns>
		constexpr bool AreResourceStatesValid(const D3D12_RESOURCE_STATES resourceStates);

		template <Brawler::SettingID ID>
		Brawler::SettingsManager::SettingType<ID> GetOption();

		template <Brawler::SettingID ID>
		void SetOption(const Brawler::SettingsManager::SettingType<ID> value);
	}
}

// -----------------------------------------------------------------------------------------------------

namespace Util
{
	namespace Engine
	{
		constexpr bool AreResourceStatesValid(const D3D12_RESOURCE_STATES resourceStates)
		{
			constexpr std::array<D3D12_RESOURCE_STATES, 12> READ_STATES_ARRAY{
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
				D3D12_RESOURCE_STATE_INDEX_BUFFER,
				D3D12_RESOURCE_STATE_DEPTH_READ,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,  // This is equivalent to D3D12_RESOURCE_STATE_PREDICATION.
				D3D12_RESOURCE_STATE_COPY_SOURCE,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
				D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,
				D3D12_RESOURCE_STATE_VIDEO_DECODE_READ,
				D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ,
				D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ
			};
			
			constexpr std::array<D3D12_RESOURCE_STATES, 10> WRITE_STATES_ARRAY{
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				D3D12_RESOURCE_STATE_STREAM_OUT,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_RESOLVE_DEST,
				D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
				D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE,
				D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE,
				D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE
			};

			// Since D3D12_RESOURCE_STATE_COMMON == 0, we handle it separately here.
			if (resourceStates == D3D12_RESOURCE_STATE_COMMON)
				return true;

			// A resource can be in at most one write-only or read/write state.
			std::uint32_t writeStateCount = 0;
			for (const auto& writeState : WRITE_STATES_ARRAY)
			{
				if (resourceStates & writeState)
					++writeStateCount;
			}

			if (writeStateCount > 1)
				return false;

			// If there is a write-only or read/write state set, then no read
			// states may also be set.
			if (writeStateCount == 1)
			{
				for (const auto& readState : READ_STATES_ARRAY)
				{
					if (resourceStates & readState)
						return false;
				}
			}

			return true;
		}
		
		template <Brawler::SettingID ID>
		Brawler::SettingsManager::SettingType<ID> GetOption()
		{
			return Brawler::SettingsManager::GetInstance().GetOption<ID>();
		}

		template <Brawler::SettingID ID>
		void SetOption(const Brawler::SettingsManager::SettingType<ID> value)
		{
			Brawler::SettingsManager::GetInstance().SetOption<ID>(value);
		}
	}
}