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
    
    crawl.addPageToQueue("http://www.wikipedia.org");
    crawl.startWorkerThreads(25);
    
    crawl.waitForFinish();
    
    return 0;
}

