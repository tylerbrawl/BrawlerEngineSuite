module;
#include <utility>
#include <iostream>
#include "DxDef.h"

module Tests.RenderJobSystem;
import Brawler.RenderJobSystem;
import Util.Engine;
import Brawler.Renderer;
import Util.Math;
import Brawler.I_GPUResource;

namespace
{
	static constexpr std::uint64_t BUFFER_SIZE = Util::Math::KilobytesToBytes(256);

	class GenericBuffer : public Brawler::I_GPUResource
	{
	public:
		GenericBuffer(const std::uint64_t bufferSize = BUFFER_SIZE) :
			I_GPUResource(),
			mBufferSize(bufferSize)
		{}

		Brawler::D3D12_RESOURCE_DESC GetResourceDescription() const override
		{
			Brawler::D3D12_RESOURCE_DESC resourceDesc{};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = mBufferSize;

			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			return resourceDesc;
		}

	private:
		const std::uint64_t mBufferSize;
	};
}

namespace Tests
{
	void RunRenderJobSystemTests()
	{
		// Test 1: Simple Job Submission
		//
		// This works as expected, but I am kind of worried about performance. As of writing
		// this, an internal MSVC compiler error prevents building in Release mode, so I
		// cannot determine actual performance numbers. In Debug builds, however, it seems
		// to take quite a lot of time before the RenderEventHandles are triggered.
		//
		// This might not be an actual issue, though. A real rendering job would do far
		// more than just printing out to the console, and chances are we don't really need to
		// wait for the render jobs to finish execution for the vast majority of cases. Also,
		// remember that "waiting" actually consists of stealing CPU jobs from the worker
		// thread queues.
		//
		// In the end, all we can do is benchmark and pray. After all, pre-mature
		// optimization is the root of all evil.
		{
			Brawler::RenderJobManager& renderJobManager{ Util::Engine::GetRenderJobManager() };

			Brawler::RenderJobBundle simpleBundle{};

			Brawler::ComputeJob randomComputeJob{};
			randomComputeJob.SetCallback([] (Brawler::ComputeContext& context)
			{
				std::cout << "Recording into a Brawler::ComputeContext...";
			});

			simpleBundle.AddComputeJob(std::move(randomComputeJob));

			Brawler::RenderEventHandle hCompletionEvent{ simpleBundle.SubmitRenderJobs() };
			hCompletionEvent.WaitForEventCompletion();

			std::cout << "done.\n" << std::endl;

			randomComputeJob = Brawler::ComputeJob{};
			randomComputeJob.SetCallback([] (Brawler::ComputeContext& context)
			{
				std::cout << "Re-using the previous Brawler::ComputeContext...";
			});
			simpleBundle.AddComputeJob(std::move(randomComputeJob));

			hCompletionEvent = std::move(simpleBundle.SubmitRenderJobs());
			hCompletionEvent.WaitForEventCompletion();

			std::cout << "done." << std::endl;
		}

		// Test 2: Resource Transitions
		{
			Brawler::RenderJobBundle transitionBundle{};

			std::unique_ptr<GenericBuffer> buffer{ Util::Engine::GetRenderer().CreateDefaultResource<GenericBuffer>(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) };

			Brawler::GraphicsJob transitionJob{};

			Brawler::GPUResourceHandle hBuffer{ transitionJob.AddResourceDependency(*buffer, Brawler::ResourceAccessMode::WRITE) };
			Brawler::ResourceTransitionToken bufferTransitionToken{ transitionJob.CreateResourceTransitionToken(hBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS) };

			transitionJob.SetCallback([hBuffer, bufferTransitionToken] (Brawler::GraphicsContext& context) mutable
			{
				context.TransitionResource(std::move(bufferTransitionToken));
			});
			transitionBundle.AddGraphicsJob(std::move(transitionJob));

			Brawler::RenderEventHandle hTransitionCompleted{ transitionBundle.SubmitRenderJobs() };
			hTransitionCompleted.WaitForEventCompletion();

			transitionJob = Brawler::GraphicsJob{};
			hBuffer = transitionJob.AddResourceDependency(*buffer, Brawler::ResourceAccessMode::WRITE);
			bufferTransitionToken = transitionJob.CreateResourceTransitionToken(hBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			transitionJob.SetCallback([hBuffer, bufferTransitionToken] (Brawler::GraphicsContext& context) mutable
			{
				// Due to implicit state promotion, this resource barrier should be dropped by the Brawler Engine.
				context.TransitionResource(std::move(bufferTransitionToken));
			});
			transitionBundle.AddGraphicsJob(std::move(transitionJob));

			hTransitionCompleted = std::move(transitionBundle.SubmitRenderJobs());
			hTransitionCompleted.WaitForEventCompletion();

			int breakHere = 0;
		}

		int breakHere = 0;
	}
}