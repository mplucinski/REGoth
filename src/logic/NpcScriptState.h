#pragma once
#include <string>
#include <handle/HandleDef.h>
#include <engine/World.h>

namespace Logic
{
    /**
     * Program based AI-states
     */
    enum class EPrgStates
    {
        NPC_PRGAISTATE_INVALID = -1,
        NPC_PRGAISTATE_ANSWER = 0,
        NPC_PRGAISTATE_DEAD,
        NPC_PRGAISTATE_UNCONSCIOUS,
        NPC_PRGAISTATE_FADEAWAY,
        NPC_PRGAISTATE_FOLLOW,
        NUM_PRGNPC_AISTATE
    };

	/**
	 * Structure describing an NPC-state
	 */
	struct NpcAIState 
	{
		enum class EPhase { Uninitialized = -1, Loop = 0, End = 1, Interrupt = 2 };

		NpcAIState()
		{
			symIndex = 0;
			symLoop = 0;
			symEnd = 0;
			phase = EPhase::Uninitialized;
			valid = false;
			stateTime = 0.0f;
            prgState = EPrgStates::NPC_PRGAISTATE_INVALID;
			isRoutineState = false;
		}

		// Index of the start-function symbol
		size_t			symIndex;

		// Index of the loop-function symbol
		size_t			symLoop;

		// Index of the end-function symbol
		size_t			symEnd;
		// -1 = not yet initialized, 0 = _LOOP, 1 = _END, 2 = _INTERRUPT
		EPhase			phase;

		// Whether the symbols are valid
		bool			valid;

		// Name of the state: ZS_name
		std::string		name;

		// How long this is already running
		float			stateTime;

		// User defined index to choose instead of calling a script-function
        EPrgStates		prgState;

		// Whether this is a state from a daily routine
		bool			isRoutineState;
	};

	class NpcScriptState
	{
	public:

		NpcScriptState(World::WorldInstance& world, Handle::EntityHandle hostVob);
		~NpcScriptState();

		/**
		 * @param symIdx Symbol-index of the state-main function
		 * @param endOldState Whether to end the currently active state as soon as possible
		 * @param timeBehaviour How this function should be executed
		 * @param isRoutineState Whether this belongs to the daily routine of the npc
		 * @return True, if the state was successfully activated. False if not possible or wanted for some reason.
		 */
		bool startAIState(size_t symIdx, bool endOldState, bool isRoutineState = false);
		bool startAIState(const std::string& name, bool endOldState, bool isRoutineState = false);

		/**
		 * @return Index of the last states main-function
		 */
		size_t getLastState() { return m_LastStateSymIndex; }

        /**
         * Checks whether the PLAYER going to the given state would be valid at this time
         * @param state State to check
         * @return Whether we could go to the input state
         */
        bool canPlayerUseAIState(const NpcAIState& state);

        /**
         * Performs the actions needed for the current frame and the current state. Advances to the next one, if needed
         * @param deltaTime time since last frame
         * @return Whether the state could be processed. This may be false when states are not activated for this NPC.
         */
        bool doAIState(float deltaTime);

        /**
         * Starts the routine-state set for this NPC
         * @return Whether the state could be started
         */
        bool startRoutineState(bool force = false);

		/**
		 * @return Whether this NPC currently is in a routine state or no state at all
		 */
		bool isInRoutine();

		/**
		 * Checks whether this NPC is currently in the state with stateMainSymbol as starting function
		 * @param stateMainSymbol Starting function of the state to check
		 * @return Whether the NPC is inside the given state
		 */
		bool isInState(size_t stateMainSymbol);

		/**
		 * Activates the currently set routine state
		 * @param force Do it, even though we are in an other state ATM
		 * @return Success
		 */
		bool activateRoutineState(bool force);

		/**
		 * Inserts a new routine target for this npc
		 * @param hoursStart Starting time (hours)
		 * @param minutesStart Starting time (minutes)
		 * @param hoursEnd Ending time (hours)
		 * @param minutesEnd Ending time (minutes)
		 * @param symFunc Symbol of the states init function
		 * @param waypoint Waypoint to go to
		 */
		void insertRoutine(int hoursStart, int minutesStart, int hoursEnd, int minutesEnd, size_t symFunc, const std::string& waypoint);

		/**
		 * @return Whether this NPC has a start-ai-state set inside the script object (monsters, mostly)
		 */
		bool isNpcStateDriven();

		/**
		 * @return Setup-function of the state we are currently in
		 */
		size_t getCurrentStateSym();

		/**
		 * Gets the current routine function from the script instance and replaces the routine stored with the new one
		 */
		void reinitRoutine();
	protected:

		/**
		 * Currently executed AI-state
		 */
		NpcAIState m_CurrentState;

		/**
		 * Next AI-state in queue
		 */
		NpcAIState m_NextState;

		/**
		 * Index of the last states main-function
		 */
		size_t m_LastStateSymIndex;

		/**
		 * World this resides in
		 */
		World::WorldInstance& m_World;

        /**
         * Vob this belongs to
         */
        Handle::EntityHandle m_HostVob;

		/**
		 * Other/victim/item set when we started the state
		 */
		Daedalus::GameState::NpcHandle m_StateOther;
		Daedalus::GameState::NpcHandle m_StateVictim;
		Daedalus::GameState::ItemHandle m_StateItem;


		/**
		 * One entry for the routines. See insertRoutine for more info
		 */
		struct RoutineEntry
		{
			/**
			 * @return Whether the given hours/minutes match to this state
			 */
			bool timeInRange(int hours, int minutes)
			{
				auto tbigger = [&](int h1, int m1, int h2, int m2){
					return h1 > h2 || (h1 == h2 && m1 > m2);
				};

				auto tsmaller = [&](int h1, int m1, int h2, int m2){
					return h1 < h2 || (h1 == h2 && m1 < m2);
				};

				auto trange = [&](int h1, int m1, int h, int m, int h2, int m2){
					return tbigger(h,m,h1,m1) && tsmaller(h,m,h2,m2);
				};

				bool crossesZero = hoursEnd < hoursStart;

				if(!crossesZero)
					return trange(hoursStart, minutesStart, hours, minutes, hoursEnd, minutesEnd);
				else
				{
					return trange(hoursStart, minutesStart, hours, minutes, 24, 0)
							&& trange(0, 0, hours, minutes, hoursEnd, minutesEnd);
				}
			}

			int hoursStart;
			int minutesStart;
			int hoursEnd;
			int minutesEnd;
			size_t symFunc;
			std::string waypoint;
			bool isOverlay;
		};


		struct
		{
			/**
			 * Currently active routine.
			 */
			std::vector<RoutineEntry> routine;
			size_t routineActiveIdx;

			/**
			 * Whether to start a new routine as soon as possible
			 */
			bool startNewRoutine;
			bool hasRoutine;
		}m_Routine;
	};
}