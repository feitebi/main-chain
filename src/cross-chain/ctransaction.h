#ifndef CTRANSACTION_H
#define CTRANSACTION_H

#include "util/uint256.h"
#include "script.h"
#include "serialize.h"

#include <string>
#include <cstring>
#include <cstdio>

class COutPoint
{
public:
    uint256 hash;
    unsigned int n;

    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, unsigned int nIn) { hash = hashIn; n = nIn; }
    IMPLEMENT_SERIALIZE( READWRITE(FLATDATA(*this)); )
    void SetNull() { hash = 0; n = (unsigned int) -1; }
    bool IsNull() const { return (hash == 0 && n == (unsigned int) -1); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    std::string ToString() const
    {
        // return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10).c_str(), n);
    }

    void print() const
    {
        // printf("%s\n", ToString().c_str());
    }
};

class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    unsigned int nSequence;

    CTxIn()
    {
        nSequence = std::numeric_limits<unsigned int>::max();
    }

    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), unsigned int nSequenceIn=std::numeric_limits<unsigned int>::max())
    {
        prevout = prevoutIn;
        scriptSig = scriptSigIn;
        nSequence = nSequenceIn;
    }

    CTxIn(uint256 hashPrevTx, unsigned int nOut, CScript scriptSigIn=CScript(), unsigned int nSequenceIn=std::numeric_limits<unsigned int>::max())
    {
        prevout = COutPoint(hashPrevTx, nOut);
        scriptSig = scriptSigIn;
        nSequence = nSequenceIn;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(prevout);
        READWRITE(scriptSig);
        READWRITE(nSequence);
    )

    bool IsFinal() const
    {
        return (nSequence == std::numeric_limits<unsigned int>::max());
    }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToStringShort() const
    {
        return strprintf(" %s %d", prevout.hash.ToString().c_str(), prevout.n);
    }

    std::string ToString() const
    {
        std::string str;
        str += "CTxIn(";
        str += prevout.ToString();
        if (prevout.IsNull())
            str += strprintf(", coinbase %s", HexStr(scriptSig).c_str());
        else
            str += strprintf(", scriptSig=%s", scriptSig.ToString().substr(0,24).c_str());
        if (nSequence != std::numeric_limits<unsigned int>::max())
            str += strprintf(", nSequence=%u", nSequence);
        str += ")";
        return str;
    }

    void print() const
    {
        printf("%s\n", ToString().c_str());
    }
};

class CTxOut
{
public:
    int64 nValue;
    CScript scriptPubKey;

    CTxOut()
    {
        SetNull();
    }

    CTxOut(int64 nValueIn, CScript scriptPubKeyIn)
    {
        nValue = nValueIn;
        scriptPubKey = scriptPubKeyIn;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nValue);
        READWRITE(scriptPubKey);
    )

    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
    }

    bool IsNull()
    {
        return (nValue == -1);
    }

    void SetEmpty()
    {
        nValue = 0;
        scriptPubKey.clear();
    }

    bool IsEmpty() const
    {
        return (nValue == 0 && scriptPubKey.empty());
    }

    uint256 GetHash() const
    {
        return SerializeHash(*this);
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

//    std::string ToStringShort() const
//    {
//        return strprintf(" out %s %s", FormatMoney(nValue).c_str(), scriptPubKey.ToString(true).c_str());
//    }

//    std::string ToString() const
//    {
//        if (IsEmpty()) return "CTxOut(empty)";
//        if (scriptPubKey.size() < 6)
//            return "CTxOut(error)";
//        return strprintf("CTxOut(nValue=%s, scriptPubKey=%s)", FormatMoney(nValue).c_str(), scriptPubKey.ToString().c_str());
//    }

    void print() const
    {
        // printf("%s\n", ToString().c_str());
    }
};

class CBTCTransaction
{
public:
    static const int CURRENT_VERSION=1;

    int nVersion;
    unsigned int nTime;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    unsigned int nLockTime;

    // Denial-of-service detection:
    mutable int nDoS;
    bool DoS(int nDoSIn, bool fIn) const { nDoS += nDoSIn; return fIn; }

    CBTCTransaction()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        // READWRITE(nTime);
        READWRITE(vin);
        READWRITE(vout);
        READWRITE(nLockTime);
    )

    void SetNull()
    {
        nVersion = CBTCTransaction::CURRENT_VERSION;
        nTime = time(0);// GetAdjustedTime();
        vin.clear();
        vout.clear();
        nLockTime = 0;
        nDoS = 0;  // Denial-of-service prevention
    }

    bool IsNull() const
    {
        return (vin.empty() && vout.empty());
    }

    virtual uint256 GetHash() const
    {
        return SerializeHash(*this);
    }

//    bool IsFinal(int nBlockHeight=0, int64 nBlockTime=0) const
//    {
//        // Time based nLockTime implemented in 0.1.6
//        if (nLockTime == 0)
//            return true;
//        if (nBlockHeight == 0)
//            nBlockHeight = nBestHeight;
//        if (nBlockTime == 0)
//            nBlockTime = GetAdjustedTime();
//        if ((int64)nLockTime < ((int64)nLockTime < LOCKTIME_THRESHOLD ? (int64)nBlockHeight : nBlockTime))
//            return true;
//        BOOST_FOREACH(const CTxIn& txin, vin)
//            if (!txin.IsFinal())
//                return false;
//        return true;
//    }

    bool IsNewerThan(const CBTCTransaction& old) const
    {
        if (vin.size() != old.vin.size())
            return false;
        for (unsigned int i = 0; i < vin.size(); i++)
            if (vin[i].prevout != old.vin[i].prevout)
                return false;

        bool fNewer = false;
        unsigned int nLowest = std::numeric_limits<unsigned int>::max();
        for (unsigned int i = 0; i < vin.size(); i++)
        {
            if (vin[i].nSequence != old.vin[i].nSequence)
            {
                if (vin[i].nSequence <= nLowest)
                {
                    fNewer = false;
                    nLowest = vin[i].nSequence;
                }
                if (old.vin[i].nSequence < nLowest)
                {
                    fNewer = true;
                    nLowest = old.vin[i].nSequence;
                }
            }
        }
        return fNewer;
    }

    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull() && vout.size() >= 1);
    }

    bool IsCoinStake() const
    {
        // ppcoin: the coin stake transaction is marked with the first output empty
        return (vin.size() > 0 && (!vin[0].prevout.IsNull()) && vout.size() >= 2 && vout[0].IsEmpty());
    }

    bool IsCoinBaseOrStake() const
    {
        return (IsCoinBase() || IsCoinStake());
    }


    /** Check for standard transaction types
        @return True if all outputs (scriptPubKeys) use only standard transaction forms
    */
    bool IsStandard() const;

    /** Check for standard transaction types
        @param[in] mapInputs	Map of previous transactions that have outputs we're spending
        @return True if all inputs (scriptSigs) use only standard transaction forms
        @see CTransaction::FetchInputs
    */
    // bool AreInputsStandard(const MapPrevTx& mapInputs) const;

    /** Count ECDSA signature operations the old-fashioned (pre-0.6) way
        @return number of sigops this transaction's outputs will produce when spent
        @see CTransaction::FetchInputs
    */
    unsigned int GetLegacySigOpCount() const;

    /** Count ECDSA signature operations in pay-to-script-hash inputs.

        @param[in] mapInputs	Map of previous transactions that have outputs we're spending
        @return maximum number of sigops required to validate this transaction's inputs
        @see CTransaction::FetchInputs
     */
    // unsigned int GetP2SHSigOpCount(const MapPrevTx& mapInputs) const;

    /** Amount of bitcoins spent by this transaction.
        @return sum of all outputs (note: does not include fees)
     */
//    int64 GetValueOut() const
//    {
//        int64 nValueOut = 0;
//        BOOST_FOREACH(const CTxOut& txout, vout)
//        {
//            nValueOut += txout.nValue;
//            if (!MoneyRange(txout.nValue) || !MoneyRange(nValueOut))
//                throw std::runtime_error("CTransaction::GetValueOut() : value out of range");
//        }
//        return nValueOut;
//    }

    /** Amount of bitcoins coming in to this transaction
        Note that lightweight clients may not know anything besides the hash of previous transactions,
        so may not be able to calculate this.

        @param[in] mapInputs	Map of previous transactions that have outputs we're spending
        @return	Sum of value of all inputs (scriptSigs)
        @see CTransaction::FetchInputs
     */
//    int64 GetValueIn(const MapPrevTx& mapInputs) const;

//    static bool AllowFree(double dPriority)
//    {
//        // Large (in bytes) low-priority (new, small-coin) transactions
//        // need a fee.
//        return dPriority > COIN * 960 / 250;
//    }

//    int64 GetMinFee(unsigned int nBlockSize=1, bool fAllowFree=false, enum GetMinFee_mode mode=GMF_BLOCK, unsigned int nBytes = 0) const;

//    bool ReadFromDisk(CDiskTxPos pos, FILE** pfileRet=NULL)
//    {
//        CAutoFile filein = CAutoFile(OpenBlockFile(pos.nFile, 0, pfileRet ? "rb+" : "rb"), SER_DISK, CLIENT_VERSION);
//        if (!filein)
//            return error("CTransaction::ReadFromDisk() : OpenBlockFile failed");

//        // Read transaction
//        if (fseek(filein, pos.nTxPos, SEEK_SET) != 0)
//            return error("CTransaction::ReadFromDisk() : fseek failed");

//        try {
//            filein >> *this;
//        }
//        catch (std::exception &e) {
//            return error("%s() : deserialize or I/O error", __PRETTY_FUNCTION__);
//        }

//        // Return file pointer
//        if (pfileRet)
//        {
//            if (fseek(filein, pos.nTxPos, SEEK_SET) != 0)
//                return error("CTransaction::ReadFromDisk() : second fseek failed");
//            *pfileRet = filein.release();
//        }
//        return true;
//    }

    friend bool operator==(const CBTCTransaction& a, const CBTCTransaction& b)
    {
        return (a.nVersion  == b.nVersion &&
                // a.nTime     == b.nTime &&
                a.vin       == b.vin &&
                a.vout      == b.vout &&
                a.nLockTime == b.nLockTime);
    }

    friend bool operator!=(const CBTCTransaction& a, const CBTCTransaction& b)
    {
        return !(a == b);
    }

    std::string ToStringShort() const
    {
        std::string str;
        str += strprintf("%s %s", GetHash().ToString().c_str(), IsCoinBase()? "base" : (IsCoinStake()? "stake" : "user"));
        return str;
    }

//    std::string ToString() const
//    {
//        std::string str;
//        str += IsCoinBase()? "Coinbase" : (IsCoinStake()? "Coinstake" : "CTransaction");
//        str += strprintf("(hash=%s, nTime=%d, ver=%d, vin.size=%"PRIszu", vout.size=%"PRIszu", nLockTime=%d)\n",
//            GetHash().ToString().substr(0,10).c_str(),
//            nTime,
//            nVersion,
//            vin.size(),
//            vout.size(),
//            nLockTime
//            );

//        for (unsigned int i = 0; i < vin.size(); i++)
//            str += "    " + vin[i].ToString() + "\n";
//        for (unsigned int i = 0; i < vout.size(); i++)
//            str += "    " + vout[i].ToString() + "\n";
//        return str;
//    }

    void print() const
    {
        // printf("%s", ToString().c_str());
    }


    // bool ReadFromDisk(CTxDB& txdb, COutPoint prevout, CTxIndex& txindexRet);
    // bool ReadFromDisk(CTxDB& txdb, COutPoint prevout);
    // bool ReadFromDisk(COutPoint prevout);
    // bool DisconnectInputs(CTxDB& txdb);

    /** Fetch from memory and/or disk. inputsRet keys are transaction hashes.

     @param[in] txdb	Transaction database
     @param[in] mapTestPool	List of pending changes to the transaction index database
     @param[in] fBlock	True if being called to add a new best-block to the chain
     @param[in] fMiner	True if being called by CreateNewBlock
     @param[out] inputsRet	Pointers to this transaction's inputs
     @param[out] fInvalid	returns true if transaction is invalid
     @return	Returns true if all inputs are in txdb or mapTestPool
     */
//    bool FetchInputs(CTxDB& txdb, const std::map<uint256, CTxIndex>& mapTestPool,
//                     bool fBlock, bool fMiner, MapPrevTx& inputsRet, bool& fInvalid);

    /** Sanity check previous transactions, then, if all checks succeed,
        mark them as spent by this transaction.

        @param[in] inputs	Previous transactions (from FetchInputs)
        @param[out] mapTestPool	Keeps track of inputs that need to be updated on disk
        @param[in] posThisTx	Position of this transaction on disk
        @param[in] pindexBlock
        @param[in] fBlock	true if called from ConnectBlock
        @param[in] fMiner	true if called from CreateNewBlock
        @param[in] fStrictPayToScriptHash	true if fully validating p2sh transactions
        @return Returns true if all checks succeed
     */
//    bool ConnectInputs(CTxDB& txdb, MapPrevTx inputs,
//                       std::map<uint256, CTxIndex>& mapTestPool, const CDiskTxPos& posThisTx,
//                       const CBlockIndex* pindexBlock, bool fBlock, bool fMiner, bool fStrictPayToScriptHash=true);
//    bool ClientConnectInputs();
//    bool CheckTransaction() const;
//    bool AcceptToMemoryPool(CTxDB& txdb, bool fCheckInputs=true, bool* pfMissingInputs=NULL);
//    bool GetCoinAge(CTxDB& txdb, uint64& nCoinAge) const;  // ppcoin: get transaction coin age

protected:
//    const CTxOut& GetOutputFor(const CTxIn& input, const MapPrevTx& inputs) const;
};


class CTransaction : public CBTCTransaction
{
public:
    CTransaction() : CBTCTransaction()
    {

    }

//    CTransaction(CBTCTransaction & o)
//    {
//        // OMG :(
//        // CTransaction diff from CBTCTransaction only methods of serialisation
//        *this = reinterpret_cast<CTransaction &>(o);
//    }

//    CTransaction & operator = (const CBTCTransaction & o)
//    {
//        *this = reinterpret_cast<const CTransaction &>(o);
//        return *this;
//    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nTime);
        READWRITE(vin);
        READWRITE(vout);
        READWRITE(nLockTime);
    )

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return (a.nVersion  == b.nVersion &&
                a.nTime     == b.nTime &&
                a.vin       == b.vin &&
                a.vout      == b.vout &&
                a.nLockTime == b.nLockTime);
    }

    virtual uint256 GetHash() const
    {
        return SerializeHash(*this);
    }
};

#endif // CTRANSACTION_H
