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
    
    crawl.addPageToQueue("google.com");
    crawl.startWorkerThreads(10);
    
    crawl.waitForFinish();
    
    return 0;
}

