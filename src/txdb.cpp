// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2016-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"

#include "main.h"
#include "pow.h"
#include "uint256.h"
#include "accumulators.h"
#include "spork.h"
#include <stdint.h>

#include <boost/thread.hpp>

using namespace std;
using namespace libzerocoin;

static const char DB_COIN = 'C';
static const char DB_COINS = 'c';
static const char DB_BLOCK_FILES = 'f';
static const char DB_TXINDEX = 't';
static const char DB_ADDRESSINDEX = 'a';
static const char DB_ADDRESSUNSPENTINDEX = 'u';
static const char DB_TIMESTAMPINDEX = 's';
static const char DB_SPENTINDEX = 'p';
static const char DB_BLOCK_INDEX = 'b';

static const char DB_BEST_BLOCK = 'B';
static const char DB_FLAG = 'F';
static const char DB_REINDEX_FLAG = 'R';
static const char DB_LAST_BLOCK = 'l';

CCoinsViewDB::CCoinsViewDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "chainstate", nCacheSize, fMemory, fWipe)
{
}

bool CCoinsViewDB::GetCoins(const uint256& txid, CCoins& coins) const
{
    return db.Read(make_pair(DB_COINS, txid), coins);
}

bool CCoinsViewDB::HaveCoins(const uint256& txid) const
{
    return db.Exists(make_pair(DB_COINS, txid));
}

uint256 CCoinsViewDB::GetBestBlock() const
{
    uint256 hashBestChain;
    if (!db.Read(DB_BEST_BLOCK, hashBestChain))
        return uint256(0);
    return hashBestChain;
}

bool CCoinsViewDB::BatchWrite(CCoinsMap& mapCoins, const uint256& hashBlock)
{
    CDBBatch batch(db);
    size_t count = 0;
    size_t changed = 0;
    for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end();) {
        if (it->second.flags & CCoinsCacheEntry::DIRTY) {
            if (it->second.coins.IsPruned())
                batch.Erase(make_pair(DB_COINS, it->first));
            else
                batch.Write(make_pair(DB_COINS, it->first), it->second.coins);
            changed++;
        }
        count++;
        CCoinsMap::iterator itOld = it++;
        mapCoins.erase(itOld);
    }
    if (hashBlock != uint256(0))
         batch.Write(DB_BEST_BLOCK, hashBlock);

    LogPrint("coindb", "Committing %u changed transactions (out of %u) to coin database...\n", (unsigned int)changed, (unsigned int)count);
    return db.WriteBatch(batch);
}

CBlockTreeDB::CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "blocks" / "index", nCacheSize, fMemory, fWipe)
{
}

bool CBlockTreeDB::WriteBlockIndex(const CDiskBlockIndex& blockindex)
{
    return Write(make_pair(DB_BLOCK_INDEX, blockindex.GetBlockHash()), blockindex);
}

bool CBlockTreeDB::WriteBlockFileInfo(int nFile, const CBlockFileInfo& info)
{
    return Write(make_pair(DB_BLOCK_FILES, nFile), info);
}

bool CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo& info)
{
    return Read(make_pair(DB_BLOCK_FILES, nFile), info);
}

bool CBlockTreeDB::WriteLastBlockFile(int nFile)
{
    return Write(DB_LAST_BLOCK, nFile);
}

bool CBlockTreeDB::WriteReindexing(bool fReindexing)
{
    if (fReindexing)
        return Write(DB_REINDEX_FLAG, '1');
    else
        return Erase(DB_REINDEX_FLAG);
}

bool CBlockTreeDB::ReadReindexing(bool& fReindexing)
{
    fReindexing = Exists(DB_REINDEX_FLAG);
    return true;
}

bool CBlockTreeDB::ReadLastBlockFile(int& nFile)
{
    return Read(DB_LAST_BLOCK, nFile);
}

bool CCoinsViewDB::GetStats(CCoinsStats &stats) const {
    /* It seems that there are no "const iterators" for LevelDB.  Since we
       only need read operations on it, use a const-cast to get around
       that restriction.  */
    boost::scoped_ptr<CDBIterator> pcursor(const_cast<CDBWrapper*>(&db)->NewIterator());
    pcursor->Seek(DB_COINS);

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    stats.hashBlock = GetBestBlock();
    ss << stats.hashBlock;
    CAmount nTotalAmount = 0;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;
        CCoins coins;
        if (pcursor->GetKey(key) && key.first == DB_COINS) {
            if (pcursor->GetValue(coins)) {
                stats.nTransactions++;
                for (unsigned int i=0; i<coins.vout.size(); i++) {
                    const CTxOut &out = coins.vout[i];
                    if (!out.IsNull()) {
                        stats.nTransactionOutputs++;
                        ss << VARINT(i+1);
                        ss << out;
                        nTotalAmount += out.nValue;
                    }
                }
                stats.nSerializedSize += 32 + pcursor->GetValueSize();
                ss << VARINT(0);
            } else {
                return error("CCoinsViewDB::GetStats() : unable to read value");
            }
        } else {
            break;
        }
        pcursor->Next();
    }
    {
        LOCK(cs_main);
        stats.nHeight = mapBlockIndex.find(stats.hashBlock)->second->nHeight;
    }
    stats.hashSerialized = ss.GetHash();
    stats.nTotalAmount = nTotalAmount;
    return true;
}

bool CBlockTreeDB::ReadTxIndex(const uint256& txid, CDiskTxPos& pos)
{
    return Read(make_pair(DB_TXINDEX, txid), pos);
}

bool CBlockTreeDB::WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> >& vect)
{
    CDBBatch batch(*this);
    for (std::vector<std::pair<uint256, CDiskTxPos> >::const_iterator it = vect.begin(); it != vect.end(); it++)
        batch.Write(make_pair(DB_TXINDEX, it->first), it->second);
    return WriteBatch(batch);
}


bool CBlockTreeDB::ReadSpentIndex(CSpentIndexKey& key, CSpentIndexValue& value)
{
    return Read(make_pair(DB_SPENTINDEX, key), value);
}

bool CBlockTreeDB::UpdateSpentIndex(const std::vector<std::pair<CSpentIndexKey, CSpentIndexValue>>& vect)
{
    CDBBatch batch(*this);
    for (std::vector<std::pair<CSpentIndexKey, CSpentIndexValue>>::const_iterator it = vect.begin(); it != vect.end(); it++) {
        if (it->second.IsNull()) {
            batch.Erase(make_pair(DB_SPENTINDEX, it->first));
        } else {
            batch.Write(make_pair(DB_SPENTINDEX, it->first), it->second);
        }
    }
    return WriteBatch(batch);
}

bool CBlockTreeDB::UpdateAddressUnspentIndex(const std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue>>& vect)
{
    CDBBatch batch(*this);
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue>>::const_iterator it = vect.begin(); it != vect.end(); it++) {
        if (it->second.IsNull()) {
            batch.Erase(make_pair(DB_ADDRESSUNSPENTINDEX, it->first));
        } else {
            batch.Write(make_pair(DB_ADDRESSUNSPENTINDEX, it->first), it->second);
        }
    }
    return WriteBatch(batch);
}

bool CBlockTreeDB::ReadAddressUnspentIndex(uint160 addressHash, int type, std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue>>& unspentOutputs)
{
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(make_pair(DB_ADDRESSUNSPENTINDEX, CAddressIndexIteratorKey(type, addressHash)));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, CAddressUnspentKey> key;
        if (pcursor->GetKey(key) && key.first == DB_ADDRESSUNSPENTINDEX && key.second.hashBytes == addressHash) {
            CAddressUnspentValue nValue;
            if (pcursor->GetValue(nValue)) {
                unspentOutputs.push_back(make_pair(key.second, nValue));
                pcursor->Next();
            } else {
                return error("failed to get address unspent value");
            }
        } else {
            break;
        }
    }

    return true;
}

bool CBlockTreeDB::WriteAddressIndex(const std::vector<std::pair<CAddressIndexKey, CAmount>>& vect)
{
    CDBBatch batch(*this);
    for (std::vector<std::pair<CAddressIndexKey, CAmount>>::const_iterator it = vect.begin(); it != vect.end(); it++)
        batch.Write(make_pair(DB_ADDRESSINDEX, it->first), it->second);
    return WriteBatch(batch);
}

bool CBlockTreeDB::EraseAddressIndex(const std::vector<std::pair<CAddressIndexKey, CAmount>>& vect)
{
    CDBBatch batch(*this);
    for (std::vector<std::pair<CAddressIndexKey, CAmount>>::const_iterator it = vect.begin(); it != vect.end(); it++)
        batch.Erase(make_pair(DB_ADDRESSINDEX, it->first));
    return WriteBatch(batch);
}

bool CBlockTreeDB::ReadAddressIndex(uint160 addressHash, int type, std::vector<std::pair<CAddressIndexKey, CAmount>>& addressIndex, int start, int end)
{
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    if (start > 0 && end > 0) {
        pcursor->Seek(make_pair(DB_ADDRESSINDEX, CAddressIndexIteratorHeightKey(type, addressHash, start)));
    } else {
        pcursor->Seek(make_pair(DB_ADDRESSINDEX, CAddressIndexIteratorKey(type, addressHash)));
    }

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, CAddressIndexKey> key;
        if (pcursor->GetKey(key) && key.first == DB_ADDRESSINDEX && key.second.hashBytes == addressHash) {
            if (end > 0 && key.second.blockHeight > end) {
                break;
            }
            CAmount nValue;
            if (pcursor->GetValue(nValue)) {
                addressIndex.push_back(make_pair(key.second, nValue));
                pcursor->Next();
            } else {
                return error("failed to get address index value");
            }
        } else {
            break;
        }
    }

    return true;
}

bool CBlockTreeDB::WriteTimestampIndex(const CTimestampIndexKey& timestampIndex)
{
    CDBBatch batch(*this);
    batch.Write(make_pair(DB_TIMESTAMPINDEX, timestampIndex), 0);
    return WriteBatch(batch);
}

bool CBlockTreeDB::ReadTimestampIndex(const unsigned int &high, const unsigned int &low, std::vector<uint256> &hashes) {

    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(make_pair(DB_TIMESTAMPINDEX, CTimestampIndexIteratorKey(low)));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, CTimestampIndexKey> key;
        if (pcursor->GetKey(key) && key.first == DB_TIMESTAMPINDEX && key.second.timestamp <= high) {
            hashes.push_back(key.second.blockHash);
            pcursor->Next();
        } else {
            break;
        }
    }

    return true;
}


bool CBlockTreeDB::WriteFlag(const std::string& name, bool fValue)
{
    return Write(std::make_pair(DB_FLAG, name), fValue ? '1' : '0');
}

bool CBlockTreeDB::ReadFlag(const std::string& name, bool& fValue)
{
    char ch;
    if (!Read(std::make_pair(DB_FLAG, name), ch))
        return false;
    fValue = ch == '1';
    return true;
}

bool CBlockTreeDB::WriteInt(const std::string& name, int nValue)
{
    return Write(std::make_pair('I', name), nValue);
}

bool CBlockTreeDB::ReadInt(const std::string& name, int& nValue)
{
    return Read(std::make_pair('I', name), nValue);
}

bool CBlockTreeDB::LoadBlockIndexGuts()
{
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(make_pair(DB_BLOCK_INDEX, uint256()));

    // Load mapBlockIndex
    uint256 nPreviousCheckpoint;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            std::pair<char, uint256> key;
            if (pcursor->GetKey(key) && key.first == DB_BLOCK_INDEX) {
                CDiskBlockIndex diskindex;
                if (pcursor->GetValue(diskindex)) {
                    // Construct block index object
                    CBlockIndex* pindexNew = InsertBlockIndex(diskindex.GetBlockHash());
                    pindexNew->pprev = InsertBlockIndex(diskindex.hashPrev);
                    pindexNew->pnext = InsertBlockIndex(diskindex.hashNext);
                    pindexNew->nHeight = diskindex.nHeight;
                    pindexNew->nFile = diskindex.nFile;
                    pindexNew->nDataPos = diskindex.nDataPos;
                    pindexNew->nUndoPos = diskindex.nUndoPos;
                    pindexNew->nVersion = diskindex.nVersion;
                    pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
                    pindexNew->nTime = diskindex.nTime;
                    pindexNew->nBits = diskindex.nBits;
                    pindexNew->nNonce = diskindex.nNonce;
                    pindexNew->nStatus = diskindex.nStatus;
                    pindexNew->nTx = diskindex.nTx;

                    //zerocoin
                    pindexNew->nAccumulatorCheckpoint = diskindex.nAccumulatorCheckpoint;
                    pindexNew->mapZerocoinSupply = diskindex.mapZerocoinSupply;
                    pindexNew->vMintDenominationsInBlock = diskindex.vMintDenominationsInBlock;

                    //Proof Of Stake
                    pindexNew->nMint = diskindex.nMint;
                    pindexNew->nMoneySupply = diskindex.nMoneySupply;
                    pindexNew->nFlags = diskindex.nFlags;
                    pindexNew->nStakeModifier = diskindex.nStakeModifier;
                    if (pindexNew->nHeight >= GetSporkValue(SPORK_13_STAKING_PROTOCOL_2)) pindexNew->nStakeModifierV2 = diskindex.nStakeModifierV2;
                    //pindexNew->prevoutStake = diskindex.prevoutStake;
                    //pindexNew->nStakeTime = diskindex.nStakeTime;
                    //pindexNew->hashProofOfStake = diskindex.hashProofOfStake;

                    if (pindexNew->nHeight <= Params().LAST_POW_BLOCK()) {
                        if (!CheckProofOfWork(pindexNew->GetBlockHash(), pindexNew->nBits))
                            return error("LoadBlockIndex() : CheckProofOfWork failed: %s", pindexNew->ToString());
                    }
                    // ppcoin: build setStakeSeen. unused
                    //if (pindexNew->IsProofOfStake()) setStakeSeen.insert(make_pair(pindexNew->prevoutStake, pindexNew->nStakeTime));

                    //populate accumulator checksum map in memory
                    if(pindexNew->nAccumulatorCheckpoint != 0 && pindexNew->nAccumulatorCheckpoint != nPreviousCheckpoint)
                    {
                        LoadAccumulatorValuesFromDB(pindexNew->nAccumulatorCheckpoint);

                        nPreviousCheckpoint = pindexNew->nAccumulatorCheckpoint;
                    }

                    pcursor->Next();
                    } else {
                    return error("LoadBlockIndex() : failed to read value");
                }
            } else {
                break; // if shutdown requested or finished loading block index
            }
        } catch (std::exception& e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }

    return true;
}

CZerocoinDB::CZerocoinDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "zerocoin", nCacheSize, fMemory, fWipe)
{
}

bool CZerocoinDB::WriteCoinMint(const PublicCoin& pubCoin, const uint256& hashTx)
{
    CBigNum bnValue = pubCoin.getValue();
    CDataStream ss(SER_GETHASH, 0);
    ss << pubCoin.getValue();
    uint256 hash = Hash(ss.begin(), ss.end());

    return Write(make_pair('m', hash), hashTx, true);
}

bool CZerocoinDB::ReadCoinMint(const CBigNum& bnPubcoin, uint256& hashTx)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << bnPubcoin;
    uint256 hash = Hash(ss.begin(), ss.end());

    return Read(make_pair('m', hash), hashTx);
}

bool CZerocoinDB::EraseCoinMint(const CBigNum& bnPubcoin)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << bnPubcoin;
    uint256 hash = Hash(ss.begin(), ss.end());

    return Erase(make_pair('m', hash));
}

bool CZerocoinDB::WriteCoinSpend(const CBigNum& bnSerial, const uint256& txHash)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << bnSerial;
    uint256 hash = Hash(ss.begin(), ss.end());

    return Write(make_pair('s', hash), txHash, true);
}

bool CZerocoinDB::ReadCoinSpend(const CBigNum& bnSerial, uint256& txHash)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << bnSerial;
    uint256 hash = Hash(ss.begin(), ss.end());

    return Read(make_pair('s', hash), txHash);
}

bool CZerocoinDB::EraseCoinSpend(const CBigNum& bnSerial)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << bnSerial;
    uint256 hash = Hash(ss.begin(), ss.end());

    return Erase(make_pair('s', hash));
}

bool CZerocoinDB::WriteAccumulatorValue(const uint32_t& nChecksum, const CBigNum& bnValue)
{
    LogPrint("zero","%s : checksum:%d val:%s\n", __func__, nChecksum, bnValue.GetHex());
    return Write(make_pair(DB_ADDRESSINDEX, nChecksum), bnValue);
}

bool CZerocoinDB::ReadAccumulatorValue(const uint32_t& nChecksum, CBigNum& bnValue)
{
    return Read(make_pair(DB_ADDRESSINDEX, nChecksum), bnValue);
}

bool CZerocoinDB::EraseAccumulatorValue(const uint32_t& nChecksum)
{
    LogPrint("zero", "%s : checksum:%d\n", __func__, nChecksum);
    return Erase(make_pair(DB_ADDRESSINDEX, nChecksum));
}
