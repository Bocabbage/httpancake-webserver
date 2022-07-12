#include "EventLoop.hpp"
#include <thread>
#include <functional>
#include <iostream>
using std::cout;
using std::endl;

void inLoopCall()
{
    cout << "inLoopCall: " << gettid() << endl;
}

void subThread(EventLoop *mlp)
{
    EventLoop sublp;
    // Direct tid check
    cout << "sublp tid: " << sublp.tid() << endl;
    cout << "sublp looks up mainlp: " << mlp->tid() << endl;
    cout << "Is in main-thread? (should be false): " << mlp->isInLoopThread() << endl;

    // inLoopCall check
    mlp->runInLoop(inLoopCall);
}

int main()
{
    EventLoop lp;
    cout << "mainlp tid: " << lp.tid() << endl;
    cout << "Is in main-thread? (should be true): " << lp.isInLoopThread() << endl;
    std::thread subthread(subThread, &lp);
    subthread.join();
    lp.loop();
}