#ifndef XBRIDGETRANSACTIONDESCR
#define XBRIDGETRANSACTIONDESCR

#include "util/uint256.h"
#include "xbridgepacket.h"

#include <string>
#include <boost/cstdint.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

//*****************************************************************************
//*****************************************************************************
struct XBridgeTransactionDescr;
typedef boost::shared_ptr<XBridgeTransactionDescr> XBridgeTransactionDescrPtr;

//******************************************************************************
//******************************************************************************
struct XBridgeTransactionDescr
{
    enum
    {
        MIN_TX_FEE = 100,
        COIN = 1000000
    };

    enum State
    {
        trNew = 0,
        trPending,
        trAccepting,
        trHold,
        trCreated,
        trSigned,
        trCommited,
        trFinished,
        trRollback,
        trDropped,
        trCancelled,
        trInvalid,
        trExpired
    };

    uint256                    id;

    std::vector<unsigned char> hubAddress;
    std::vector<unsigned char> myAddress;

    std::vector<unsigned char> from;
    std::string                fromCurrency;
    boost::uint64_t            fromAmount;
    std::vector<unsigned char> to;
    std::string                toCurrency;
    boost::uint64_t            toAmount;

    State                      state;

    boost::posix_time::ptime   created;
    boost::posix_time::ptime   txtime;

    uint256                    payTxId;
    std::string                payTx;
    std::string                revTx;

    XBridgePacketPtr           packet;

    XBridgeTransactionDescr()
        : state(trNew)
        , created(boost::posix_time::second_clock::universal_time())
        , txtime(boost::posix_time::second_clock::universal_time())
    {}

//    bool operator == (const XBridgeTransactionDescr & d) const
//    {
//        return id == d.id;
//    }

    bool operator < (const XBridgeTransactionDescr & d) const
    {
        return created < d.created;
    }

    bool operator > (const XBridgeTransactionDescr & d) const
    {
        return created > d.created;
    }

    XBridgeTransactionDescr & operator = (const XBridgeTransactionDescr & d)
    {
        if (this == &d)
        {
            return *this;
        }

        copyFrom(d);

        return *this;
    }

    XBridgeTransactionDescr(const XBridgeTransactionDescr & d)
    {
        state   = trNew;
        created = boost::posix_time::second_clock::universal_time();
        txtime  = boost::posix_time::second_clock::universal_time();

        copyFrom(d);
    }

private:
    void copyFrom(const XBridgeTransactionDescr & d)
    {
        id           = d.id;
        from         = d.from;
        fromCurrency = d.fromCurrency;
        fromAmount   = d.fromAmount;
        to           = d.to;
        toCurrency   = d.toCurrency;
        toAmount     = d.toAmount;
        state        = d.state;
        payTx        = d.payTx;
        revTx        = d.revTx;
        txtime       = boost::posix_time::second_clock::universal_time();
        if (created > d.created)
        {
            created = d.created;
        }

        payTxId     = d.payTxId;
        hubAddress  = d.hubAddress;
        myAddress   = d.myAddress;
    }
};

#endif // XBRIDGETRANSACTIONDESCR

