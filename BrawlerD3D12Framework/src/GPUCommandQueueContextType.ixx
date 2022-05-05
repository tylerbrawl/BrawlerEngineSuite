module;

export module Brawler.D3D12.GPUCommandQueueContextType;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.DirectContext;
import Brawler.D3D12.ComputeContext;
import Brawler.D3D12.CopyContext;

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		struct QueueTypeInfo
		{
			static_assert(sizeof(QueueType) != sizeof(QueueType));
		};

		template <typename ContextType_>
		struct QueueTypeInfoInstantiation
		{
			using ContextType = ContextType_;
		};

		template <>
		struct QueueTypeInfo<GPUCommandQueueType::DIRECT> : public QueueTypeInfoInstantiation<DirectContext>
		{};

		template <>
		struct QueueTypeInfo<GPUCommandQueueType::COMPUTE> : public QueueTypeInfoInstantiation<ComputeContext>
		{};

		template <>
		struct QueueTypeInfo<GPUCommandQueueType::COPY> : public QueueTypeInfoInstantiation<CopyContext>
		{};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		using GPUCommandQueueContextType = QueueTypeInfo<QueueType>::ContextType;
	}
}