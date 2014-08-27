/* 
 * File:   crawler.cpp
 * Author: Florian
 * 
 * Created on 26. August 2014, 12:48
 */

#include "crawler.hpp"

#define TIMEOUT 60
#define BASE_MAX 1
#define USE_BASE

std::string lineOverride = "\r                                            \r";

boost::regex doctypePattern(".*DOCTYPE.*", boost::regex::icase);
boost::regex urlPattern("(((https?://[^/]*)/[^/]*)[^\\?]*)(?:\\?{0,1}).*", boost::regex::icase);
boost::regex baseUrlPattern("(https?://[^/]*)/.*", boost::regex::icase);
boost::regex filePattern("(https?://[^\\?]*)(?:\\?{0,1}).*", boost::regex::icase);
boost::regex folderPattern("(https?://.*/)[^/]*", boost::regex::icase);
boost::regex javascriptPattern("javascript:.*", boost::regex::icase);
boost::regex ignoreTypePattern(".*\\.(png|bmp|css|js|gif|jpg|jpeg|svg|zip|exe|gz|tar|bz2|rar|avi|mp4|mp3|wmv|wma|acc|flac|iso|7z|od.|pdf|docx?|pptx?|csvx?|vbs?)$", boost::regex::icase);
boost::regex linkPattern("(?:href|src|link)[[:space:]]*=[[:space:]]*(?:\\\"(.*?)\\\"|\\'(.*?)\\')", boost::regex::icase);
boost::regex tldPattern("https?://[^/]*\\.([^\\./]*)/.*", boost::regex::icase);
boost::regex ignoreTagsPattern("<(script|style).*?\\1>", boost::regex::icase);
boost::regex tagPattern("</?.*?>", boost::regex::icase);
boost::regex wordPattern("(?:(?!&?n?b?s?p?;|&?a?m?p?;)[^[:space:][:punct:]]|(?:&(?!nbsp;|amp;)\\w{1,5};))+", boost::regex::icase);

void crawler::loadPage(std::string url) {
    std::string content;


    boost::match_results<std::string::const_iterator> res;
    boost::regex_search(url, res, baseUrlPattern);
    std::string baseUrl = res[1];

    visitedLock.lock();
    std::cout << lineOverride << pageQueue.size() << "  " << connErrors << "e   " << pageQueue.size() / (float) pageCounter << std::flush;
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

    addContentToQueue(url, content);
}

void crawler::analyzePage(std::string url, std::string content) {
    if (!boost::regex_match(content, doctypePattern))
        return;

    boost::match_results<std::string::const_iterator> res;
    boost::regex_search(url, res, urlPattern);
    std::string baseUrl = res[3];
    std::string folderUrl = res[2];
    std::string fileUrl = res[1];

    visitedLock.lock();
#ifdef BASE_MAX
    int count = visited.count(baseUrl);
    if (count >= BASE_MAX) {
#else
    int count = visited.count(url);
    if (count) {
#endif
        visitedLock.unlock();
        return;
    } else if (!count) {
#ifdef USE_BASE
        std::cout << lineOverride << ++pageCounter << "   " << baseUrl << "   " << std::endl;
#else
        std::cout << lineOverride << ++pageCounter << "   " << url << "   " << std::endl;
#endif
    }

#ifdef BASE_MAX
    visited.insert(baseUrl);
#else
    visited.insert(url);
#endif

    visitedLock.unlock();

    boost::sregex_iterator hrefIt(content.begin(), content.end(), linkPattern);

    std::for_each(hrefIt, boost::sregex_iterator(), [this, baseUrl, fileUrl, folderUrl](const boost::match_results<std::string::const_iterator>& what) {
        std::string link = what[1];

        if (boost::regex_match(link, javascriptPattern)) {
            return;
        }

        if (link.substr(0, 1) == ".") {
            link = "/" + link;
        }

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


        if (boost::regex_match(link, ignoreTypePattern)) {
            return;
        }

        boost::match_results<std::string::const_iterator> res;
                boost::regex_search(link, res, tldPattern);
        if (res[1] != "de") {
            return;
        }


        boost::regex_search(link, res, baseUrlPattern);
                std::string baseLink = res[1];

                visitedLock.lock();
#ifdef BASE_MAX
                if (visited.count(baseLink) < BASE_MAX) {
#else
                if (!visited.count(link)) {
#endif
#ifdef USE_BASE
            addPageToQueue(baseLink);
#else
            addPageToQueue(link);
#endif
        }
        visitedLock.unlock();
    });


    content = boost::regex_replace(content, ignoreTagsPattern, "", boost::match_default | boost::format_all);
    content = boost::regex_replace(content, tagPattern, "", boost::match_default | boost::format_all);

    boost::sregex_token_iterator wordIt(content.begin(), content.end(), wordPattern, 0), wordItEnd;
    while (wordIt != wordItEnd) {
        std::string word = *wordIt++;

        auto it = dict.find(word);
        if (it == dict.end()) {
            dict.insert(std::make_pair(word, 1));
        } else {
            it->second++;
        }
    }
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
