/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the GPL).
 *
 * Software distributed under the License is distributed
 * on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

#ifndef ng_ReentrantMonitor_h
#define ng_ReentrantMonitor_h

#include <prmon.h>
#include <mozilla/BlockingResourceBase.h>
#include <mozilla/ReentrantMonitor.h>


class ngReentrantMonitor : mozilla::BlockingResourceBase
{
public:
    ngReentrantMonitor(const char* aName) :
        BlockingResourceBase(aName, eReentrantMonitor)
#ifdef DEBUG
        , mEntryCount(0)
#endif
    {
        MOZ_COUNT_CTOR(ReentrantMonitor);
        mReentrantMonitor = PR_NewMonitor();
        if (!mReentrantMonitor)
            NS_RUNTIMEABORT("Can't allocate ngReentrantMonitor");
    }

    ~ngReentrantMonitor()
    {
        NS_ASSERTION(mReentrantMonitor,
           "improperly constructed ngReentrantMonitor or double free");
        PR_DestroyMonitor(mReentrantMonitor);
        mReentrantMonitor = 0;
        MOZ_COUNT_DTOR(ReentrantMonitor);
    }

#ifndef DEBUG
    void Enter()
    {
        PR_EnterMonitor(mReentrantMonitor);
    }

    void Exit()
    {
        PR_ExitMonitor(mReentrantMonitor);
    }

    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT)
    {
        return PR_Wait(mReentrantMonitor, interval) == PR_SUCCESS ?
            NS_OK : NS_ERROR_FAILURE;
    }

#else
    void Enter();
    void Exit();
    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT);

#endif  // ifndef DEBUG

    nsresult Notify()
    {
        return PR_Notify(mReentrantMonitor) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

    nsresult NotifyAll()
    {
        return PR_NotifyAll(mReentrantMonitor) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

#ifdef DEBUG
    void AssertCurrentThreadIn()
    {
        PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mReentrantMonitor);
    }

    void AssertNotCurrentThreadIn()
    {
        // FIXME bug 476536
    }

#else
    void AssertCurrentThreadIn()
    {
    }
    void AssertNotCurrentThreadIn()
    {
    }

#endif  // ifdef DEBUG

    friend class ngReentrantMonitorAutoEnter;
private:
    ngReentrantMonitor();
    ngReentrantMonitor(const mozilla::ReentrantMonitor&);
    ngReentrantMonitor& operator =(const mozilla::ReentrantMonitor&);

    PRMonitor* mReentrantMonitor;
#ifdef DEBUG
    PRInt32 mEntryCount;
#endif
};


/**
 * ngReentrantMonitorAutoEnter
 * Enters the ReentrantMonitor when it enters scope, and exits it when
 * it leaves scope. This adds Enter() and Exit() functions.
 */
class ngReentrantMonitorAutoEnter
{
public:
    ngReentrantMonitorAutoEnter(ngReentrantMonitor &aReentrantMonitor) :
        mReentrantMonitor(&aReentrantMonitor)
    {
        NS_ASSERTION(mReentrantMonitor, "null monitor");
        mReentrantMonitor->Enter();
    }
    
    ~ngReentrantMonitorAutoEnter(void)
    {
        mReentrantMonitor->Exit();
    }
 
    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT)
    {
       return mReentrantMonitor->Wait(interval);
    }

    nsresult Notify()
    {
        return mReentrantMonitor->Notify();
    }

    nsresult NotifyAll()
    {
        return mReentrantMonitor->NotifyAll();
    }

    void Enter()
    {
        NS_ASSERTION(mReentrantMonitor->mReentrantMonitor,
            "improperly constructed ngReentrantMonitorAutoEnter");
        mReentrantMonitor->Enter();
    }

    void Exit()
    {
        NS_ASSERTION(mReentrantMonitor->mReentrantMonitor,
            "ngReentrantMonitorAutoEnter never entered!");
        mReentrantMonitor->Exit();
    }

private:
    ngReentrantMonitorAutoEnter();
    ngReentrantMonitorAutoEnter(const ngReentrantMonitorAutoEnter&);
    ngReentrantMonitorAutoEnter& operator =(const ngReentrantMonitorAutoEnter&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);
    ngReentrantMonitor* mReentrantMonitor;

};

#endif // ifndef ng_ReentrantMonitor_h
