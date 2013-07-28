/****************************************************************************************\
 Filename    :  smBase.h
 Purpose     :  Basic Sate Machines engine
 Author      :  Krasnitsky Sergey
\****************************************************************************************/

#ifndef _SMBASE_H_
#define _SMBASE_H_

#include "smId.h"
#include "deblog.h"
#include "fifo_cse.h"
#include "thread.h"


#define SMID_ALL            ((unsigned)(-1))
#define CHOICE              ((unsigned)(-1))
#define NULLTRANS           ((void*)1)			// Value for transitions that have not an action


enum SMQ
{
    SMQ_IMMEDIATE = -1,
    SMQ_HIGH      = 0,
    SMQ_LOW       = 1
};


struct SM;
struct SMEVENT;


/* 
    SM transition types
*/
typedef int  (SM::*FTRANSITION) (SMEVENT* ev, int param);

/*
    Get the state name for SM function. Only for debug purpose.
*/
typedef char* (*FSTATENAME) (int state);


/* 
    SM event element for one state:
*/
struct TEVSTATE
{
    SMEV            Event;
    int             State_end;      // End state for transition, if it's (-1) than StatParam points to array of TCHOICE and Func returns index in this array
    union {
		FTRANSITION     Func;
		void*			Func_;
	};
    UINT32          StatParam;
};


struct TCHOICE
{
    int             State_end;
    FTRANSITION     Func;
    UINT32          StatParam;
};



struct SM : public DebLog
{
	SM(cchar *name) : DebLog(name) {};

  public:
    SMID         SmId;
    TEVSTATE     (*aStates)[SMEV_NUMS];		// States-Events table for SM
    const char **aStateNames;				// States names
    int          naStates;					// Size of aStates array
    int          State;						// Current state
    int          State_prev;				// Previous state

	bool Execute (SMEVENT *pEvent);			// One-cycle SM execute
};


template <class T> struct SMT : public SM
{
	typedef bool (T::*FTRANS) (SMEVENT* ev, int param);

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

	/* Initialization of one TEVSTATE structure */
	static void InitStateNode (int state, SMEV ev, int state_end, FTRANS func, UINT32 param = 0)
	{
		union {
			FTRANS  Func;
			void*   Func_;
		} F = { func };
		T::StateTable[state][ev].Event     = ev;
		T::StateTable[state][ev].State_end = state_end;
		T::StateTable[state][ev].Func_     = F.Func_;
		T::StateTable[state][ev].StatParam = param;
	}

	/* Initialization to support NULLTRANS */
	static void InitStateNode (int state, SMEV ev, int state_end, void* func, UINT32 param = 0)
	{
		union {
			void*   Func_;
			FTRANS  Func;
		} F = { func };
		InitStateNode (state, ev, state_end, F.Func, param);
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
		static TEVSTATE		StateTable[NSTATES][SMEV_NUMS];		\
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
    TEVSTATE      smname::StateTable[NSTATES][SMEV_NUMS];	\
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



/*
    Initialization the TCHOICE structure macro (TODO - this is old style, to improve):
    smbInitChoiceElem (TCHOICE[] aDynState, int Num, int StateEnd, FTRANSITION Func, int param);
*/
#define smbInitChoiceElem(_aDynState_,_Num_,_StateEnd_,_Func_,_StatParam_)  \
{                                                                           \
    _aDynState_[_Num_].State_end = _StateEnd_;                              \
    _aDynState_[_Num_].Func      = _Func_;                                  \
    _aDynState_[_Num_].StatParam = (UINT32)_StatParam_;                     \
}



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
