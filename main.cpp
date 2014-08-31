/* 
 * File:   main.cpp
 * Author: Florian
 *
 * Created on 26. August 2014, 12:17
 */

#include <signal.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>

#include "crawler.hpp"

using namespace std;

crawler crawl;

void myIntHandler(int)
{
    crawl.printInteruptStats();
    exit(1);
}

int main(int argc, char** argv) {
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = myIntHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);


    crawl.addPageToQueue(START_URL, true);
    crawl.startWorkerThreads(START_WORKER_LOADER, START_WORKER_ANALYZER);

    crawl.waitForFinish();

    return 0;
}

