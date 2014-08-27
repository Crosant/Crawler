/* 
 * File:   main.cpp
 * Author: Florian
 *
 * Created on 26. August 2014, 12:17
 */

#include <cstdlib>
#include <iostream>

#include "crawler.hpp"

using namespace std;

/*
 * 
 */
int main(int argc, char** argv) {
    crawler crawl;

    crawl.addPageToQueue("http://google.de/", true);
    crawl.startWorkerThreads(1, 1);

    crawl.waitForFinish();

    return 0;
}

