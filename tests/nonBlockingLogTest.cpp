#include "NonBlockingLog.hpp"
#include <thread>
#include <iostream>
#include <vector>

using std::cout;
using std::endl;

NonBlockingLog *logT;

void singleThreadTest()
{
    // cout << "----------stressing test single thread BEGIN-----------" << endl;
    for (int i = 1; i <= 100000; ++i)
    {
        string msg = std::to_string(i);
        msg += ": test";
        msg += string(500, '!');
        msg += '\n';
        // cout << msg << endl;
        logT->append(std::move(msg));
    }
    // cout << "----------stressing test single thread END-----------" << endl;
}

void threadFunc(int threadId)
{
    string idStr = std::to_string(threadId);
    for(int i = 1; i <= 100000; ++i)
    {
        string msg = idStr;
        msg += ": hello!\n";
        logT->append(std::move(msg));
    }
}

int main()
{
    logT = new NonBlockingLog("./NonBlockingLogTest.log", 2);
    // singleThreadTest();
    vector<std::thread> threadVec;
    for(int i = 0; i < 4; ++i)
    {
        threadVec.emplace_back(std::thread(threadFunc, i));
    }

    for(auto &t: threadVec)
        t.join();

    delete logT;
}