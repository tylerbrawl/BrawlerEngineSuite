module;

export module Util.Coroutine;

export namespace Util
{
	namespace Coroutine
	{
		void RethrowUnhandledException();
		
		/// <summary>
		/// Attempts to acquire a job from a worker thread queue. If a job is acquired, then it is
		/// immediately executed. Otherwise, the calling thread yields. If possible, use this
		/// function whenever you would normally do a busy wait.
		/// </summary>
		/// <returns>
		/// The function returns true if a job was executed during this call and false otherwise.
		/// </returns>
		bool TryExecuteJob();
	}
}