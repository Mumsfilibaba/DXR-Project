#pragma once
#include "Platform/Mutex.h"

#include "Generic/Thread.h"

class TaskManager
{
public:
    bool Init();

    void Release();

    static TaskManager& Get();

private:
    TaskManager();
    ~TaskManager();

    Mutex JobMutex;
};