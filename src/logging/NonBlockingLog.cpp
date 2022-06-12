#include "NonBlockingLog.hpp"
#include <functional>
#include <chrono>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

using namespace std::chrono_literals;

NonBlockingLog::NonBlockingLog(const std::string& logFileName, int flushInterval):
    logFileName_(logFileName),
    logFileFd_(::open(logFileName_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)),
    logThread_(std::bind(&NonBlockingLog::threadFunc, this)),
    mtx_(),
    cond_(),
    flushInterval_(flushInterval),
    running_(true),  // start the logThread when created
    buffers_(),
    nowBuffer_(new LargeFixBuffer),
    nextBuffer_(new LargeFixBuffer)
    // nowBuffer_(&buffers_[0]),
    // writeBackBuffer_(&buffers_[1])
{

}

NonBlockingLog::~NonBlockingLog()
{
    if(running_)
        stop();
    ::close(logFileFd_);
}

void NonBlockingLog::stop()
{
    if(running_)
        running_ = false;
    cond_.notify_one(); // only logThread(reader) waits for this cond
    logThread_.join();
}

void NonBlockingLog::append(string &&msg)
{
    std::lock_guard<std::mutex> lk(mtx_);
    if(msg.length() < nowBuffer_->free())
    {
        nowBuffer_->append(std::forward<string>(msg));
    }
    else
    {
        // if(msg.length() >= nowBuffer_->free())
        // {
        //     // exchange the buffers
        //     std::swap(nowBuffer_, writeBackBuffer_);
        //     cond_.notify_one();
        // }
        
        // remained problem:
        // if(msg.length() > nowBuffer_->free())
        // {
        //     printf("not enough!\n");
        //     exit(-1);
        // }
        buffers_.push_back(std::move(nowBuffer_));

        if(nextBuffer_)
        {
            nowBuffer_.reset(nextBuffer_.release());
        }
        else
            nowBuffer_ .reset(new LargeFixBuffer);
            
        nowBuffer_->append(std::forward<string>(msg));
        cond_.notify_one(); // buffers_ is not empty now, notify the consumer(threadFunc)
    }     
}

void NonBlockingLog::writeBack(const string& msg)
{
    if(logFileFd_ > 0)
        ::write(logFileFd_, msg.c_str(), msg.length());
    else
    {
        // logFile failed
        printf("NonBlockingLog Error: logFileFd < 0.\n");
        exit(-1);
    }

}

void NonBlockingLog::threadFunc()
{
    std::chrono::seconds timeout = 1s;
    timeout *= flushInterval_;

    std::unique_ptr<LargeFixBuffer> newBuffer1(new LargeFixBuffer);
    std::unique_ptr<LargeFixBuffer> newBuffer2(new LargeFixBuffer);
    std::vector<std::unique_ptr<LargeFixBuffer> > writeBackBuffers;

    while(running_)
    {
        // string writeBackContent;
        // string nowContent;

        {
            std::unique_lock<std::mutex> lk(mtx_);  // lock
            // if(writeBackBuffer_->empty())
            // {
            //     cond_.wait_for(
            //         lk,
            //         timeout,
            //         [this]{return !this->writeBackBuffer_->empty();}
            //     );
            // }

            // if(!writeBackBuffer_->empty())
            // {
            //     // exchange out the buffer-content
            //     string content(writeBackBuffer_->getContent());
            //     writeBackContent.swap(content);
            // }

            // if(!nowBuffer_->empty())
            // {
            //     // exchange out the buffer-content
            //     string content(nowBuffer_->getContent());
            //     nowContent.swap(content);
            // }

            if(buffers_.empty())
            {
                cond_.wait_for(
                    lk,
                    timeout,
                    [this]{ return !this->buffers_.empty(); }
                );
            }
            buffers_.emplace_back(nowBuffer_.release());
            nowBuffer_.reset(newBuffer1.release());
            writeBackBuffers.swap(buffers_); // exchange out the buffer-content

            if(!nextBuffer_)
                nextBuffer_.reset(newBuffer2.release());
        }

        // writeBack(writeBackContent);
        // writeBack(nowContent);

        // write-back
        for(auto &buff: writeBackBuffers)
        {
            writeBack(buff->getContent());
        }

        if (writeBackBuffers.size() > 2) 
            writeBackBuffers.resize(2);

        if(!newBuffer1)
        {
            assert(!writeBackBuffers.empty());
            newBuffer1.reset(writeBackBuffers.back().release());
        }

        if(!newBuffer2)
        {
            assert(!writeBackBuffers.empty());
            newBuffer2.reset(writeBackBuffers.back().release());
        }

        writeBackBuffers.clear();

    }

    {
        std::unique_lock<std::mutex> lk(mtx_);
        buffers_.emplace_back(nowBuffer_.release());
        writeBackBuffers.swap(buffers_);
    }

    if(!writeBackBuffers.empty())
        for(auto &buff: writeBackBuffers)
        {
            writeBack(buff->getContent());
        }


}