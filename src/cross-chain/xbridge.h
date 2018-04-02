//*****************************************************************************
//*****************************************************************************

#ifndef XBRIDGE_H
#define XBRIDGE_H

#include <deque>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

//*****************************************************************************
//*****************************************************************************
class XBridge
{
    enum
    {
        THREAD_COUNT = 2,
        TIMER_INTERVAL = 20
    };

    typedef std::shared_ptr<boost::asio::io_service>      IoServicePtr;

public:
    typedef boost::asio::ip::tcp::socket                  Socket;
    typedef std::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;

public:
    XBridge();

    void run();
    void stop();

private:
    void onTimer();

private:
    std::deque<IoServicePtr>                        m_services;
    std::deque<boost::asio::io_service::work>       m_works;
    boost::thread_group                             m_threads;

    boost::asio::io_service                         m_timerIo;
    boost::asio::io_service::work                   m_timerIoWork;
    boost::thread                                   m_timerThread;
    boost::asio::deadline_timer                     m_timer;
};

typedef std::shared_ptr<XBridge> XBridgePtr;

#endif // XBRIDGE_H
