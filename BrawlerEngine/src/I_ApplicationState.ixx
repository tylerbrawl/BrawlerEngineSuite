module;

export module Brawler.I_ApplicationState;

export namespace Brawler
{
	class I_ApplicationState
	{
	protected:
		I_ApplicationState() = default;

	public:
		virtual ~I_ApplicationState() = default;

		I_ApplicationState(const I_ApplicationState& rhs) = delete;
		I_ApplicationState& operator=(const I_ApplicationState& rhs) = delete;

		I_ApplicationState(I_ApplicationState&& rhs) noexcept = default;
		I_ApplicationState& operator=(I_ApplicationState&& rhs) noexcept = default;

		/// <summary>
		/// Derived classes should implement this function to perform any updates which should
		/// be performed during an update tick. The implemented function should return true if
		/// I_ApplicationState instances below this state in the owning ApplicationStateStack
		/// should also be updated and false otherwise.
		/// </summary>
		/// <param name="dt">
		/// - The time, in seconds, which is to be used as the timestep for updates.
		/// </param>
		/// <returns>
		/// Derived classes should implement this function to return true if I_ApplicationState
		/// instances below this state in the owning ApplicationStateStack should also be updated
		/// and false otherwise.
		/// </returns>
		virtual bool Update(const float dt) = 0;
	};
}