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
    std::queue<std::string> pageQueue;
    std::queue<std::pair<std::string, std::string>> contentQueue;
    std::mutex visitedLock, pageQueueLock, contentQueueLock;
    std::atomic_bool running;
    std::atomic<int> runningLoaders, runningAnalyzers, connErrors, pageCounter;
    std::vector<std::thread> loaderWorkers, analyzeWorkers;

    curlpp::Cleanup myCleanup;

    void loadNextPage() {
        pageQueueLock.lock();
        if (pageQueue.empty() || (contentQueue.size() > 100 * analyzeWorkers.size())) {
            pageQueueLock.unlock();
            usleep(1000000);
            return;
        }
        std::string url = pageQueue.front();
        pageQueue.pop();

        runningLoaders++;
        pageQueueLock.unlock();

        loadPage(url);
        runningLoaders--;
    }

    void analyzeNextPage() {
        contentQueueLock.lock();
        if (contentQueue.empty()) {
            pageQueueLock.lock();
            if (runningLoaders || runningAnalyzers || pageQueue.size()) {
                contentQueueLock.unlock();
                pageQueueLock.unlock();
                usleep(1000000);
            } else {
                contentQueueLock.unlock();
                pageQueueLock.unlock();
                running = false;
            }
            return;
        }
        std::pair<std::string, std::string> ctn = contentQueue.front();
        contentQueue.pop();

        runningAnalyzers++;
        contentQueueLock.unlock();

        analyzePage(ctn.first, ctn.second);
        runningAnalyzers--;
    }

    void loadPage(std::string url);
    void analyzePage(std::string url, std::string content);

    bool readPage(std::string &url, std::string &pageData);

public:

    crawler() {
        running = true;
        connErrors = 0;
        pageCounter = 0;
        runningLoaders = 0;
        runningAnalyzers = 0;
    }

    virtual ~crawler() {
        stopWorkerThreads();
    }

    void pageLoader() {
        while (running) {
            loadNextPage();
        }
    }

    void pageAnalyzer() {
        while (running) {
            analyzeNextPage();
        }
    }

    void addPageToQueue(std::string url, bool force = false) {
        pageQueueLock.lock();
        if (force || (pageQueue.size() < 1000 * loaderWorkers.size()))
            pageQueue.push(url);
        pageQueueLock.unlock();
    }

    void addContentToQueue(std::string url, std::string content) {
        contentQueueLock.lock();
        contentQueue.push(std::make_pair(url, content));
        contentQueueLock.unlock();
    }

    void startWorkerThreads(int loader, int analyzer) {
        running = true;

        for (int i = 0; i < loader; i++) {
            loaderWorkers.push_back(std::thread([this]() {
                pageLoader(); }));
        }

        for (int i = 0; i < analyzer; i++) {
            analyzeWorkers.push_back(std::thread([this]() {
                pageAnalyzer(); }));
        }
    }

    void stopWorkerThreads() {
        running = false;

        for (auto &thread : loaderWorkers)
            thread.join();

        for (auto &thread : analyzeWorkers)
            thread.join();

        loaderWorkers.clear();
        analyzeWorkers.clear();
    }

    void waitForFinish() {
        while (running) {
            usleep(100000);
        }
    }
};

#endif	/* CRAWLER_HPP */

