/****************************************************************************************\
 Filename    :  smBase.h
 Purpose     :  Basic Sate Machines engine
 Author      :  Krasnitsky Sergey
\****************************************************************************************/

#ifndef _SMBASE_H_
#define _SMBASE_H_

#include "def.h"
#include "smId.h"
#include "deblog.h"
#include "fifo_cse.h"
#include "thread.h"


#define SMID_ALL			((unsigned)(-1))	// Destination SM ID meaning "TO ALL"
#define NULLTRANS			((void*)1)			// Value for transitions that have not an action
#define STATE_CHOICE		((unsigned)(-1))	// special STATE value meaning a choice from several states

/*
   Size of global SM-Base buffer for allocating choices.
   Comment out this define to disable the choices at all.
*/
#define SMBASE_CHOICES		10


/* 
   SM Queues priorities
*/
enum SMQ
{
    SMQ_IMMEDIATE = -1,
    SMQ_HIGH      = 0,
    SMQ_LOW       = 1
};


struct SM;
struct SMEVENT;


/* 
   SM transition function types
*/
typedef bool  (SM::*FTRANSITION) (SMEVENT* ev, int param);


/* 
   SM choice function types
*/
typedef int  (SM::*FCHOICE) (SMEVENT* ev);



/* 
   SM event element for one state:
*/
struct SMEVSTATE
{
    SMEV            Event;
    int             State_end;      // End state for transition, if it's (-1) than StatParam points to array of SMCHOICE and Func returns index in this array
    union {
	  FTRANSITION   FuncTran;
	  FCHOICE		FuncChoice;
	  void*			FuncVoid;
	};
    int64			StatParam;		// it's int64 because of it contains SMCHOICE* for choices
};


struct SMCHOICE
{
    int             State_end;
    union {
	  FTRANSITION   FuncTran;
	  void*			FuncVoid;
	};
    int				StatParam;
};



struct SM : public DebLog
{
	SM(cchar *name) : DebLog(name) {};

  public:
    SMID         SmId;
    SMEVSTATE  (*aStates)[SMEV_NUMS];		// States-Events table for SM
    const char **aStateNames;				// States names
    int          naStates;					// Size of aStates array
    int          State;						// Current state
    int          State_prev;				// Previous state (for debug purpose only)
    int          State_next;				// Set when SM::Execute runs and may be used in the trunsactions (note: choice functions are run BEFORE this field is updated)

	bool Execute (SMEVENT *pEvent);			// One-cycle SM execute

	#ifdef SMBASE_CHOICES
	static SMCHOICE	ChoiceBuffer[SMBASE_CHOICES];
	static int		ChoiceNextIdx;
	static SMCHOICE* NewChoice(int n)
	{
		ASSERT_0 (ChoiceNextIdx + n <= SMBASE_CHOICES);
		SMCHOICE* ret = &ChoiceBuffer[ChoiceNextIdx];
		ChoiceNextIdx += n;
		return ret;
	}
	#endif
};


template <class T> struct SMT : public SM
{
	typedef bool (T::*FTRANS)  (SMEVENT* ev, int param);
	typedef int  (T::*FCHOICE) (SMEVENT* ev);

	SMT(cchar *name) : SM(name) {};

	/* Construct & Register a new SM. The SMT object must be preallocated by a user	*/
	void Construct (SMID SmId)
	{
		SmId		= SmId;
		aStates		= T::StateTable;
		aStateNames = T::StateNames;
		naStates	= T::NSTATES;
		State = State_prev = 0;
		SmBase::SmGlobalArray[SmId] = this;
	}

	/* Initialization of one SMEVSTATE structure */
	static void InitStateNode (int state, SMEV ev, int state_end, FTRANS func, int64 param = 0)
	{
		union {
			FTRANS  FuncTran;
			void*   FuncVoid;
		} F = { func };
		T::StateTable[state][ev].Event     = ev;
		T::StateTable[state][ev].State_end = state_end;
		T::StateTable[state][ev].FuncVoid  = F.FuncVoid;
		T::StateTable[state][ev].StatParam = param;
	}

	/* Initialization to support NULLTRANS */
	static void InitStateNode (int state, SMEV ev, int state_end, void* func, int64 param = 0)
	{
		union {
			void*   FuncVoid;
			FTRANS  FuncTran;
		} F = { func };
		InitStateNode (state, ev, state_end, F.FuncTran, param);
	}

	/* Initialization to support NULLTRANS */
	static void InitStateNode (int state, SMEV ev, FCHOICE func, int nchoices)
	{
		union {
			FCHOICE  FuncChoice;
			FTRANS   FuncTran;
		} F = { func };
		InitStateNode (state, ev, STATE_CHOICE, F.FuncTran, (int64)NewChoice(nchoices));
	}

	static void InitChoice (int idx, int state, SMEV ev, int state_end, FTRANS func, int param = 0)
	{
		union {
			FTRANS  FuncTran;
			void*   FuncVoid;
		} F = { func };
		SMCHOICE* choice = ((SMCHOICE*) T::StateTable[state][ev].StatParam) + idx;
		
		choice->State_end = state_end;
		choice->FuncVoid  = F.FuncVoid;
		choice->StatParam = param;
	}

	static void InitChoice (int idx, int state, SMEV ev, int state_end, void* func, int param = 0)
	{
		union {
			void*   FuncVoid;
			FTRANS  FuncTran;
		} F = { func };
		InitChoice (idx, state, ev, state_end, F.FuncTran, param);
	}
};


#define STATE_ENUM(stname)  STATE_##stname
#define STATE_STR(stname)   #stname


/*
  Declare a SM's state list in SM C++ class. 
  The same parameter (statelist) should be passed to IMPL_STATES() macro.
  Parameters:
    statelist - should be defined earlier as macro with using 
                STATE() as follows:
                #define <statelist>  STATE(State1), STATE(State2),...., STATE(StateN)
  Note: Must be present in any SM declaration body.
*/
#define DECL_STATES(statelist)									\
    public:														\
        enum STATE {											\
            statelist,											\
            NSTATES												\
        };														\
		static SMEVSTATE	StateTable[NSTATES][SMEV_NUMS];		\
        static const char*  StateNames[NSTATES];


/*
  Declare a SM's state list in SM's cpp file. 
  The parameter <statelist> should be the same as passed to DECL_STATES().
  Parameters:
    smname    - SM C++ class name
    statelist - see the DECL_STATES()
  Note: Must be present in any SM's cpp file. The cpp file must include at the top the ExecBody.h!
*/
#define IMPL_STATES(smname,statelist)                       \
    SMEVSTATE     smname::StateTable[NSTATES][SMEV_NUMS];	\
    const char*   smname::StateNames[] = { statelist };


/*
  Creates one element of SM's array of states.
  Usage: STATE(<state_name>)
*/
#define STATE   STATE_ENUM


/* 
   Event element structure (16 bytes).
*/
struct SMEVENT
{
    SMID        SmId;           // Destination SM ID (may be SMID_ALL)
    SMEV        Ev;		        // Event number (of TEVENTNUM type)
    SMEV_PAR    Param;		    // Event parameters
};




class SmBase : public DebLog, public Thread
{
	friend struct SM;
	template <class T> friend struct SMT;

	enum {
		SM_HQUEUE_SIZE =  5,	// High-priority event queue size
		SM_LQUEUE_SIZE = 10		// Low-priority event queue size
	};

  public:
	static void Init();
	static void End();

	/* Send event to a specific SM via prioritized queue */
	static bool PutEvent (SMEVENT *pEv, SMQ level);

  public:
	SmBase() : DebLog("SmBase "), Thread("SmBase") {};

	void Construct();
	void Destruct();

  protected:
    virtual void Run();

  protected:
	static FIFO_ALLOC <SMEVENT,SM_HQUEUE_SIZE>  QueueHigh;
	static FIFO_ALLOC <SMEVENT,SM_LQUEUE_SIZE>  QueueLow;

	/* Array of the all State machines */
	static SM* SmGlobalArray [SMID_NUMS];

	/* SmBase object */
	static SmBase This;
};



#endif // _SMBASE_H_
