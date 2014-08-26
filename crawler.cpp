/* 
 * File:   crawler.cpp
 * Author: Florian
 * 
 * Created on 26. August 2014, 12:48
 */

#include "crawler.hpp"

#define TIMEOUT 1
#define BASE_MAX 2

void crawler::parsePage(std::string url) {
    std::string content;


    boost::match_results<std::string::const_iterator> res;
    boost::regex_search(url, res, boost::regex("(https?://[^/]*)/.*", boost::regex::icase));
    std::string baseUrl = res[1];

    visitedLock.lock();
    std::cout << "\r                                                           \r" << pageQueue.size() << "  " << connErrors << "e   " << std::flush;
#ifdef BASE_MAX
    int count = visited.count(baseUrl);
    if (count >= BASE_MAX) {
#else
    int count = visited.count(url);
    if (count) {
#endif
        visitedLock.unlock();
        return;
    }
    visitedLock.unlock();

    if (!readPage(url, content))
        return;

    if (!boost::regex_match(content, boost::regex(".*DOCTYPE.*", boost::regex::icase)))
        return;

    boost::regex_search(url, res, boost::regex("(https?://[^/]*)/.*", boost::regex::icase));
    baseUrl = res[1];
    boost::regex_search(url, res, boost::regex("(https?://[^\\?]*)(\\?{0,1}).*", boost::regex::icase));
    std::string fileUrl = res[1];
    boost::regex_search(url, res, boost::regex("(https?://.*/)[^/]*", boost::regex::icase));
    std::string folderUrl = res[1];

    visitedLock.lock();
#ifdef BASE_MAX
    count = visited.count(baseUrl);
    if (count >= BASE_MAX) {
#else
    count = visited.count(url);
    if (count) {
#endif
        visitedLock.unlock();
        return;
    } else if (!count) {
        std::cout << "\r" << ++pageCounter << "   " << baseUrl << std::endl;
    }
    visited.insert(baseUrl);

    visitedLock.unlock();

    boost::sregex_iterator hrefIt(content.begin(), content.end(), boost::regex("(?:href|src|link)[[:space:]]*=[[:space:]]*(?:\\\"(.*?)\\\"|\\'(.*?)\\')", boost::regex::icase));

    std::for_each(hrefIt, boost::sregex_iterator(), [this, baseUrl, fileUrl, folderUrl](const boost::match_results<std::string::const_iterator>& what) {
        std::string link = what[1];
        if (link.substr(0, 7) == "http://" || link.substr(0, 8) == "https://") {
        } else if (link.substr(0, 2) == "//") {
            link = "http:" + link;
        } else if (link.substr(0, 1) == "/") {
            link = baseUrl + link;
        } else if (link.substr(0, 1) == "?") {
            link = fileUrl + link;
        } else {
            link = folderUrl + link;
        }

        if (boost::regex_match(link, boost::regex(".*/javascript:.*", boost::regex::icase))) {
            return;
        }

        if (boost::regex_match(link, boost::regex(".*\\.(png|bmp|css|js|gif|jpg|jpeg|svg|zip|exe|gz|tar|bz2|rar|avi|mp4|mp3|wmv|wma|acc|flac|iso|7z|od.|pdf|docx?|pptx?|csvx?|vbs?)$", boost::regex::icase))) {
            return;
        }

        visitedLock.lock();
#ifdef BASE_MAX
                if (visited.count(baseUrl) < BASE_MAX) {
#else
                if (!visited.count(url)) {
#endif
            addPageToQueue(link);
        }
        visitedLock.unlock();
    });
}

bool crawler::readPage(std::string &url, std::string &pageData) {
    try {
        curlpp::Easy myRequest;

        myRequest.setOpt<curlpp::options::WriteFunction>(
                [&pageData](char* buf, size_t size, size_t nmemb) {
                    for (int c = 0; c < size * nmemb; c++) {
                        pageData.push_back(buf[c]);
                    }
                    return size*nmemb;
                });
        myRequest.setOpt<curlpp::options::FollowLocation>(1);
        myRequest.setOpt<curlpp::options::Url>(url);
        myRequest.setOpt<curlpp::options::Timeout>(TIMEOUT);

        myRequest.perform();

        url = curlpp::infos::EffectiveUrl().get(myRequest);
    } catch (curlpp::RuntimeError & e) {
        connErrors++;
        return false;
    } catch (curlpp::LogicError & e) {
        connErrors++;
        return false;
    }

    return true;
}