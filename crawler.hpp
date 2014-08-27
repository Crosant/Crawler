/* 
 * File:   crawler.hpp
 * Author: Florian
 *
 * Created on 26. August 2014, 12:48
 */

#ifndef CRAWLER_HPP
#define	CRAWLER_HPP

#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <sstream>
#include <regex>
#include <set>

#include <cstdio>
#include <ctime>
#include <unistd.h>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>

#include <boost/regex.hpp>

class crawler {
private:
    std::map<std::string, int> dict;
    std::multiset<std::string> visited;
    std::mutex visitedLock;
    std::queue<std::string> pageQueue;
    std::mutex queueLock;
    std::atomic_bool running;
    std::atomic<int> workingThreads, connErrors, pageCounter;
    std::vector<std::thread> workers;

    curlpp::Cleanup myCleanup;

    void parseNextPage() {
        queueLock.lock();
        if (pageQueue.empty()) {
            queueLock.unlock();
            if (workingThreads)
                usleep(60000000);
            else
                running = false;
            return;
        }
        std::string url = pageQueue.front();
        pageQueue.pop();
        queueLock.unlock();
        
        workingThreads++;
        parsePage(url);
        workingThreads--;
    }

    void parsePage(std::string url);

    bool readPage(std::string &url, std::string &pageData);
public:

    crawler() {
        running = true;
        workingThreads = 0;
        connErrors = 0;
        pageCounter = 0;
    }
    
    virtual ~crawler()
    {
        stopWorkerThreads();
    }

    int worker() {
        while (running) {
            parseNextPage();
        }

        return 0;
    }

    void addPageToQueue(std::string url) {
        queueLock.lock();
        pageQueue.push(url);
        queueLock.unlock();
    }

    void startWorkerThreads(int count) {
        running = true;

        for (int i = 0; i < count; i++) {
            workers.push_back(std::thread([this]() {
                worker(); }));
        }
    }

    void stopWorkerThreads() {
        running = false;

        for (auto &thread : workers)
            thread.join();

        workers.clear();
    }
    
    void waitForFinish()
    {
        while(running)
        {
            usleep(100000);
        }
    }
};

#endif	/* CRAWLER_HPP */

