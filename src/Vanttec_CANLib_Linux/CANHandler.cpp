//
// Created by abiel on 12/9/21.
//

#include "CANHandler.h"
#include <iostream>

namespace vanttec {
    CANHandler::CANHandler(const std::string &interfaceName) {
        epfd = epoll_create1(0);
        canfd = vanttec::socketcan_open(interfaceName);
        if (epfd == -1 || canfd == -1) {
            if (epfd) close(epfd);
            if (canfd) close(canfd);
            throw std::runtime_error("Could not initialise CANHandler fds");
        }

        int ret;
        epoll_event ev{};

        ev.data.fd = canfd;
        ev.events = EPOLLIN; //Poll for read and write

        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, canfd, &ev);
        if (ret == -1) {
            close(epfd);
            close(canfd);
            throw std::runtime_error("Could not add to epoll");
        }

//        register_parser([](uint8_t id, can_frame frame){
//            std::cout << std::to_string(id) << std::endl;
//        });
    }

    void CANHandler::update_write(){
        std::unique_lock<std::mutex> lk(cv_m);
        //Wait until write queue has something
        cv.wait(lk, [this]
                { return this->writeDataReady.load(); });

        while(!writeQueue.empty()){
            auto elem = writeQueue.front();

            if (elem.len != 0)
            {
                can_frame outFrame;
                outFrame.can_dlc = elem.len;
                memcpy(outFrame.data, elem.data, elem.len);
                outFrame.can_id = 0x123;

                while (::write(canfd, &outFrame, sizeof(outFrame)) != sizeof(outFrame))
                {
                    std::cerr << "Retrying CAN Write!" << std::endl;
                    ;
                }
            }

            writeQueue.pop();
        }

        writeDataReady = false;
        lk.unlock();
    }

    void CANHandler::write(const vanttec::CANMessage &msg) {
        //TODO Do something with the message
        std::lock_guard<std::mutex> lk(cv_m);
        writeQueue.push(msg);
        writeDataReady = true;

        cv.notify_all();
    }

    void CANHandler::register_parser(uint8_t filter, const std::function<void(can_frame)> &parser){
        filterMsgParsers[filter].emplace_back(parser);
    }

    void CANHandler::register_parser(const std::function<void(uint8_t, can_frame)> &parser) {
        msgParsers.emplace_back(parser);
    }

    void CANHandler::update_read() {
        int rdy = epoll_wait(epfd, evlist, MAX_EVENTS, -1);
        if (rdy == -1) {
            std::cerr << "Error waiting for epoll" << std::endl;
            return;
        }

        can_frame frame{};

        for (int i = 0; i < rdy; i++) {
            if(!(evlist[i].events & EPOLLIN)) continue;
            //Read available
            auto len = read(evlist[i].data.fd, &frame, sizeof(frame));
            if(len < 0) continue;

            auto id = can_parse_id(frame.data, frame.can_dlc);
            for(auto &parser : msgParsers) parser(id, frame);
            auto it = filterMsgParsers.find(id);
            if(it != filterMsgParsers.end())
                for(auto &parser : it->second) parser(frame);
        }
    }

    CANHandler::~CANHandler() {
        if (epfd)
            close(epfd);

        if (canfd)
            close(canfd);
    }
}