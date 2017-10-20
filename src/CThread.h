// **************************************************************************
//
// Modul-Name        : CThread.h
// Author            : Hans-Juergen Lange <hjl@simulated-universe.de>
// Creation-Date     : 16.01.2011
// Modification-Date : 29.04.2011 07:08:56
//
//  Copyrights by Hans-Juergen Lange. All rights reserved.
//
// **************************************************************************
#ifndef __EXTENSIONLIBS_THREADS_LINUXTHREADS_CTHREAD_INC
#define __EXTENSIONLIBS_THREADS_LINUXTHREADS_CTHREAD_INC
// **************************************************************************
//                             F o r w a r d s
// **************************************************************************
#ifdef _WIN32
#endif // _WIN32
extern "C" {
void* startfnc(void* ptr);
} // extern C 
// **************************************************************************
//                  C l a s s    d e c l a r a t i o n
// **************************************************************************

class CThread {
public:
    CThread();
    CThread(const std::string& aName);
    virtual ~CThread();
    virtual bool InitInstance();
    void Create();
    virtual int Run();
    virtual void ExitInstance();
    void SetName(const std::string& aName);
    std::string& GetName();
public:
    static int Count;
protected:
    std::string Name;
    pthread_t ThreadID;
};
#endif // __EXTENSIONLIBS_THREADS_LINUXTHREADS_CTHREAD_INC
