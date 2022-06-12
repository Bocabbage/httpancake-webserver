#include "Logger.hpp"
#include <thread>

void singleThreadTest()
{
    // cout << "----------stressing test single thread BEGIN-----------" << endl;
    for (int i = 1; i <= 100000; ++i)
    {
        LOG << "Aowu!!~~~";
    }
    // cout << "----------stressing test single thread END-----------" << endl;
}

void threadFunc(int threadId)
{
    string idStr = std::to_string(threadId);
    for(int i = 1; i <= 100000; ++i)
    {
        string msg(": hello!");
        LOG << threadId << msg;
    }
}

int main()
{
    Logger::setLogFileName("./LoggerTest.log");
    // singleThreadTest();
    vector<std::thread> threadVec;
    for(int i = 0; i < 4; ++i)
    {
        threadVec.emplace_back(std::thread(threadFunc, i));
    }

    for(auto &t: threadVec)
        t.join();
}