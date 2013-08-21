/*************************************************************************************************\
 Filename    :  smBase.cpp
 Purpose     :  Basic Sate Machines engine
 Author      :  Krasnitsky Sergey
\*************************************************************************************************/

#include "def.h"
#include "smBase.h"
#include "enums_impl.h"


IMPL_ENUM (SMID, SM_LIST)
IMPL_ENUM (SMEV, SMEV_LIST)


/***********************************************************************************************\
									SmBase static data
\***********************************************************************************************/

SmBase	SmBase::This;
SM*		SmBase::SmGlobalArray [SMID_NUMS];

Semaph	SmBase::QueueSemaphor;

FIFO_ALLOC <SMEVENT,SmBase::SM_HQUEUE_SIZE>  SmBase::QueueHigh;
FIFO_ALLOC <SMEVENT,SmBase::SM_LQUEUE_SIZE>  SmBase::QueueLow;

#ifdef SMBASE_CHOICES
SMCHOICE  SM::ChoiceBuffer[SMBASE_CHOICES];
int		  SM::ChoiceNextIdx;
#endif

/***********************************************************************************************\
										SM functions
\***********************************************************************************************/

bool SM::Execute (SMEVENT *pEv)
{
    SMEVSTATE   *pEvState;
    int			 StatParam;
	// Workaround for C2440 error:
	union {
		FTRANSITION  FuncTran;
		FCHOICE		 FuncChoice;
		void*		 NullTrans;
	} F;

    ASSERT_f (this == SmBase::SmGlobalArray[SmId]);

    LogMsg ("[ %6s:%-14s ] < - - - - - - - - '%s'\n", enumTable_SMID[SmId],  aStateNames[State],  enumTable_SMEV[pEv->Ev]);
    //prnEventPrint (pEv);

    pEvState = & aStates[State][pEv->Ev];
    F.FuncTran   = pEvState->FuncTran;

    if (F.FuncTran) 
    {
        if (pEvState->State_end == STATE_CHOICE)
        {
            int ind = (this->*F.FuncChoice)(pEv);

            SMCHOICE *aChoice = (SMCHOICE*) pEvState->StatParam;

            F.FuncTran = aChoice[ind].FuncTran;
            State_next = aChoice[ind].State_end;
            StatParam  = aChoice[ind].StatParam;
        }
        else {
            State_next = pEvState->State_end;
            StatParam  = (int) pEvState->StatParam;
        }

        if (F.NullTrans != NULLTRANS) {
            if (!(this->*F.FuncTran)(pEv,StatParam))
                LogMsg ("ERROR: SM::Execute: pSm = %X, TRANSITION FAILED\n", this);
        }
        State_prev = State;
        State = State_next;
        LogMsg ("[ %6s:%-14s ]\n\n", enumTable_SMID[SmId],  aStateNames[State]);
        return true;
    }        

    LogMsg ("WARNING: Unprocessed event!\n");
    return false;
}



/***********************************************************************************************\
									Public Static functions
\***********************************************************************************************/

void SmBase::Init()
{ 
    QueueLow.Construct();
    QueueHigh.Construct();
	This.Construct();
}


void SmBase::End()
{ 
	This.Destruct();
}


bool SmBase::PutEvent (SMEVENT *pEv, SMQ level)
{	
    bool res;

    switch (level)
    {
        case SMQ_IMMEDIATE:
            ASSERT_f (pEv->SmId && pEv->SmId!=SMID_ALL);
            SmGlobalArray[pEv->SmId]->Execute (pEv);
            res = true;
            goto exit;

        case SMQ_HIGH:
            res = QueueHigh.PutElement (*pEv);
            break;

        case SMQ_LOW:
            res = QueueLow.PutElement (*pEv);
            break;
    }
    
    QueueSemaphor.Signal();

	exit:
	return res;
}


/***********************************************************************************************\
										Public functions
\***********************************************************************************************/

void SmBase::Construct()
{
	Thread::Construct();
	Thread::Execute();
	LogMsg("Thread ID = %d", Thread::GetThreadId());
}

void SmBase::Destruct()
{
	Thread::Terminate();
	//Thread::WaitEnding(); TODO graceful finish
	LogMsg("Destructed");
}

void SmBase::Run ()
{
	LogMsg("Task started...");

    SMEVENT *ev;
    FIFO<SMEVENT> * fifos[] = { &QueueHigh, &QueueLow };
    
    // Do endless loop SMQ_HIGH..SMQ_LOW
    for (int i = SMQ_HIGH; ; i = (i+1) & 1)
    {
		if (i==SMQ_HIGH) // take it ones for all queues (because it's unknown to which queue PutEvent was)
			QueueSemaphor.Take();
        while (!fifos[i]->IsEmpty()) {
            ev = fifos[i]->GetFirst();
            SmGlobalArray[ev->SmId]->Execute (ev);
            fifos[i]->ReleaseFirst();
            if (i == SMQ_LOW)
                break;
        }
    }
}

