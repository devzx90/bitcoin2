// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2017-2019 The Bitcoin 2 developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "amount.h"
#include "hash.h"
#include "main.h"
#include "masternode-sync.h"
#include "net.h"
#include "pow.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "timedata.h"
#include "util.h"
#include "utilmoneystr.h"
#ifdef ENABLE_WALLET
#include "wallet.h"
#endif
#include "validationinterface.h"
#include "masternode-payments.h"
#include "accumulators.h"
#include "spork.h"

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

using namespace std;
unsigned int LastHashedBlockHeight = 0, LastHashedBlockTime = 0;

//////////////////////////////////////////////////////////////////////////////
//
// Bitcoin2Miner
//

//
// Unconfirmed transactions in the memory pool often depend on other
// transactions in the memory pool. When we select transactions from the
// pool, we select by highest priority or fee rate, so we might consider
// transactions that depend on transactions that aren't yet in the block.
// The COrphan class keeps track of these 'temporary orphans' while
// CreateBlock is figuring out which transactions to include.
//
class COrphan
{
public:
    const CTransaction* ptx;
    set<uint256> setDependsOn;
    CFeeRate feeRate;
    double dPriority;

    COrphan(const CTransaction* ptxIn) : ptx(ptxIn), feeRate(0), dPriority(0)
    {
    }
};

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;

// We want to sort transactions by priority and fee rate, so:
typedef boost::tuple<double, CFeeRate, const CTransaction*> TxPriority;
class TxPriorityCompare
{
    bool byFee;

public:
    TxPriorityCompare(bool _byFee) : byFee(_byFee) {}

    bool operator()(const TxPriority& a, const TxPriority& b)
    {
        if (byFee) {
            if (a.get<1>() == b.get<1>())
                return a.get<0>() < b.get<0>();
            return a.get<1>() < b.get<1>();
        } else {
            if (a.get<0>() == b.get<0>())
                return a.get<1>() < b.get<1>();
            return a.get<0>() < b.get<0>();
        }
    }
};

void UpdateTime(CBlockHeader* pblock, const CBlockIndex* pindexPrev)
{
    pblock->nTime = std::max(pindexPrev->GetMedianTimePast() + 1, GetAdjustedTime());
}

CBlockTemplate* CreateNewBlock(const CScript& scriptPubKeyIn, CWallet* pwallet, bool fProofOfStake)
{
    // Create new block
    unique_ptr<CBlockTemplate> pblocktemplate(new CBlockTemplate());
    if (!pblocktemplate.get())
        return NULL;
    CBlock* pblock = &pblocktemplate->block; // pointer for convenience

    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (Params().MineBlocksOnDemand())
        pblock->nVersion = GetArg("-blockversion", pblock->nVersion);

    // Make sure to create the correct block version after zerocoin is enabled
    bool fZerocoinActive = GetAdjustedTime() >= Params().Zerocoin_StartTime();
    if (fZerocoinActive) pblock->nVersion = 4;
    else pblock->nVersion = 3;

    // Create coinbase tx
    CMutableTransaction txNew;
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vout.resize(1);
    txNew.vout[0].scriptPubKey = scriptPubKeyIn;

	if(fProofOfStake)  pblock->vtx.push_back(txNew);
	pblock->vtx.push_back(CTransaction());  // BTC2: Add dummy tx, will be the coinbase tx on PoW. or coinstake tx on PoS.

	pblocktemplate->vTxFees.push_back(-1);   // updated at end
	pblocktemplate->vTxSigOps.push_back(-1); // updated at end	

    CAmount nFees = 0;
	boost::this_thread::interruption_point();
	CBlockIndex* pindexPrev;
	int nHeight;
	{
		LOCK(cs_main);
		pindexPrev = chainActive.Tip();
		nHeight = pindexPrev->nHeight + 1;
		pblock->nBits = GetNextWorkRequired(pindexPrev);
	}

	CMutableTransaction txCoinStake;
	CAmount nCredit;
	if (fProofOfStake)
	{
		unsigned int nTxNewTime = 0;
		nCredit = pwallet->CreateCoinStake(*pwallet, pblock->nBits, txCoinStake, nTxNewTime);
		if (nCredit != 0)
		{
			pblock->nTime = nTxNewTime;
			pblock->vtx[0].vout[0].SetEmpty();
		}
		else
		{
			pblock->vtx.clear();
			pblocktemplate->vTxFees.clear();
			pblocktemplate->vTxSigOps.clear();
			return NULL;
		}
	}

	// Largest block you're willing to create:
	unsigned int nBlockMaxSize = GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
	// Limit to betweeen 1K and MAX_BLOCK_SIZE-1K for sanity:
	unsigned int nBlockMaxSizeNetwork = MAX_BLOCK_SIZE_CURRENT;
	nBlockMaxSize = std::max((unsigned int)1000, std::min((nBlockMaxSizeNetwork - 1000), nBlockMaxSize));

	// How much of the block should be dedicated to high-priority transactions,
	// included regardless of the fees they pay
	unsigned int nBlockPrioritySize = GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
	nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

    {
        LOCK2(cs_main, mempool.cs);
		if (chainActive.Tip() != pindexPrev)
		{
			LogPrintf("CreateNewBlock(): Chain changed meanwhile, best abandon this block.\n");
			return NULL;
		}

		// Collect memory pool transactions into the block
        CCoinsViewCache view(pcoinsTip);
        // Priority order to process transactions
        list<COrphan> vOrphan; // list memory doesn't move
        map<uint256, vector<COrphan*> > mapDependers;
        bool fPrintPriority = GetBoolArg("-printpriority", false);

        // This vector will be sorted into a priority queue:
        vector<TxPriority> vecPriority;
        vecPriority.reserve(mempool.mapTx.size());
		CFeeRate ActualMinRelayTxFee(MINRELAYFEE - 1); // -1 to help mitigate rounding errors.
        for (map<uint256, CTxMemPoolEntry>::iterator mi = mempool.mapTx.begin(); mi != mempool.mapTx.end(); ++mi)
		{
            const CTransaction& tx = mi->second.GetTx();
            if (tx.IsCoinBase() || tx.IsCoinStake() || !IsFinalTx(tx, nHeight)){
                continue;
            }
            if(GetAdjustedTime() > GetSporkValue(SPORK_16_ZEROCOIN_MAINTENANCE_MODE) && tx.ContainsZerocoins()){
                continue;
            }

            COrphan* porphan = NULL;
            double dPriority = 0;
            CAmount nTotalIn = 0;
            bool fMissingInputs = false;
            bool hasZerocoinSpends = tx.HasZerocoinSpendInputs();
            if (hasZerocoinSpends)
                nTotalIn = tx.GetZerocoinSpent();
            for (const CTxIn& txin : tx.vin) {
                //zerocoinspend has special vin
                if (hasZerocoinSpends) {
                    nTotalIn += tx.GetZerocoinSpent();
					break;
                }

                // Read prev transaction
                if (!view.HaveCoins(txin.prevout.hash)) {
                    // This should never happen; all transactions in the memory
                    // pool should connect to either transactions in the chain
                    // or other transactions in the memory pool.
                    if (!mempool.mapTx.count(txin.prevout.hash)) {
                        LogPrintf("ERROR: mempool transaction missing input\n");
                        if (fDebug) assert("mempool transaction missing input" == 0);
                        fMissingInputs = true;
                        if (porphan)
                            vOrphan.pop_back();
                        break;
                    }

                    // Has to wait for dependencies
                    if (!porphan) {
                        // Use list for automatic deletion
                        vOrphan.push_back(COrphan(&tx));
                        porphan = &vOrphan.back();
                    }
                    mapDependers[txin.prevout.hash].push_back(porphan);
                    porphan->setDependsOn.insert(txin.prevout.hash);
                    nTotalIn += mempool.mapTx[txin.prevout.hash].GetTx().vout[txin.prevout.n].nValue;
                    continue;
                }

                const CCoins* coins = view.AccessCoins(txin.prevout.hash);
                assert(coins);

                CAmount nValueIn = coins->vout[txin.prevout.n].nValue;
                nTotalIn += nValueIn;

                int nConf = nHeight - coins->nHeight;

				dPriority += (double)nValueIn * nConf;

            }
            if (fMissingInputs) continue;

            // Priority is sum(valuein * age) / modified_txsize
            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            dPriority = tx.ComputePriority(dPriority, nTxSize);

            uint256 hash = tx.GetHash();
            mempool.ApplyDeltas(hash, dPriority, nTotalIn);

            CFeeRate feeRate(nTotalIn - tx.GetValueOut(), nTxSize);

            if (porphan) {
                porphan->dPriority = dPriority;
                porphan->feeRate = feeRate;
            } else
                vecPriority.push_back(TxPriority(dPriority, feeRate, &mi->second.GetTx()));
        }

        // Collect transactions into block
        uint64_t nBlockSize = 1000;
        uint64_t nBlockTx = 0;
        int nBlockSigOps = 100;

		// TODO: Remove priority calculations completely and just sort by fee.

        TxPriorityCompare comparer(true); // fSortedByFee true
        std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);

        vector<CBigNum> vBlockSerials;
        vector<CBigNum> vTxSerials;
        while (!vecPriority.empty()) {
            // Take highest priority transaction off the priority queue:
            double dPriority = vecPriority.front().get<0>();
            CFeeRate feeRate = vecPriority.front().get<1>();
            const CTransaction& tx = *(vecPriority.front().get<2>());

            std::pop_heap(vecPriority.begin(), vecPriority.end(), comparer);
            vecPriority.pop_back();

            // Size limits
            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            if (nBlockSize + nTxSize >= nBlockMaxSize)
                continue;

            // Legacy limits on sigOps:
            unsigned int nMaxBlockSigOps = MAX_BLOCK_SIGOPS_CURRENT;
            unsigned int nTxSigOps = GetLegacySigOpCount(tx);
            if (nBlockSigOps + nTxSigOps >= nMaxBlockSigOps)
                continue;

            // Skip free and too low fee transactions
            if (!tx.HasZerocoinSpendInputs() && (feeRate < ActualMinRelayTxFee))
                continue;

            if (!view.HaveInputs(tx))
                continue;

            // double check that there are no double spent zBTC2 spends in this block or tx
            if (tx.HasZerocoinSpendInputs()) {
                int nHeightTx = 0;
                if (IsTransactionInChain(tx.GetHash(), nHeightTx))
                    continue;

                bool fDoubleSerial = false;
                for (const CTxIn txIn : tx.vin) {
                    if (txIn.IsZerocoinSpend()) {
                        libzerocoin::CoinSpend spend = TxInToZerocoinSpend(txIn);
                        if (!spend.HasValidSerial(Params().Zerocoin_Params()))
                            fDoubleSerial = true;
                        if (count(vBlockSerials.begin(), vBlockSerials.end(), spend.getCoinSerialNumber()))
                            fDoubleSerial = true;
                        if (count(vTxSerials.begin(), vTxSerials.end(), spend.getCoinSerialNumber()))
                            fDoubleSerial = true;
                        if (fDoubleSerial)
                            break;
                        vTxSerials.emplace_back(spend.getCoinSerialNumber());
                    }
                }
                //This zBTC2 serial has already been included in the block, do not add this tx.
                if (fDoubleSerial)
                    continue;
            }

            CAmount nTxFees = view.GetValueIn(tx) - tx.GetValueOut();

            nTxSigOps += GetP2SHSigOpCount(tx, view);
            if (nBlockSigOps + nTxSigOps >= nMaxBlockSigOps)
                continue;

            // Note that flags: we don't want to set mempool/IsStandard()
            // policy here, but we still have to ensure that the block we
            // create only contains transactions that are valid in new blocks.
            CValidationState state;
            if (!CheckInputs(tx, state, view, true, MANDATORY_SCRIPT_VERIFY_FLAGS, true, NULL, Params().Zerocoin_StartHeight()))
                continue;

            CTxUndo txundo;
            UpdateCoins(tx, state, view, txundo, nHeight);

            // Added
            pblock->vtx.push_back(tx);
            pblocktemplate->vTxFees.push_back(nTxFees);
            pblocktemplate->vTxSigOps.push_back(nTxSigOps);
            nBlockSize += nTxSize;
            ++nBlockTx;
            nBlockSigOps += nTxSigOps;
            nFees += nTxFees;

            for (const CBigNum bnSerial : vTxSerials)
                vBlockSerials.emplace_back(bnSerial);

            if (fPrintPriority) {
                LogPrintf("priority %.1f fee %s txid %s\n",
                    dPriority, feeRate.ToString(), tx.GetHash().ToString());
            }

            // Add transactions that depend on this one to the priority queue
			const uint256& hash = tx.GetHash();
            if (mapDependers.count(hash)) {
                BOOST_FOREACH (COrphan* porphan, mapDependers[hash]) {
                    if (!porphan->setDependsOn.empty()) {
                        porphan->setDependsOn.erase(hash);
                        if (porphan->setDependsOn.empty()) {
                            vecPriority.push_back(TxPriority(porphan->dPriority, porphan->feeRate, porphan->ptx));
                            std::push_heap(vecPriority.begin(), vecPriority.end(), comparer);
                        }
                    }
                }
            }
        }

        nLastBlockTx = nBlockTx;
		if(nLastBlockSize != nBlockSize) LogPrintf("CreateNewBlock(): total size %u\n", nBlockSize);
        nLastBlockSize = nBlockSize;

        // Compute final coinbase transaction.
        if (!fProofOfStake)
		{
			txNew.vout[0].nValue = GetBlockValue(nHeight) + nFees;
			pblock->vtx[0] = txNew;
			pblocktemplate->vTxFees[0] = -nFees; 
        }
		// Compute the reward and sign the coin stake. This part has to be done after adding transactions.
		else
		{
			if (!pwallet->FinishCoinStake(txCoinStake, nCredit + nFees))
			{
				mempool.clear();
				return NULL;
			}
			LogPrintf("CreateNewBlock(): FinishCoinStake() success.\n");
			pblock->vtx[1] = CTransaction(txCoinStake);
		}

		pblock->vtx[0].vin[0].scriptSig = CScript() << nHeight << OP_0;

        // Fill in header
		LogPrintf("CreateNewBlock(): pblock->hashPrevBlock = pindexPrev->GetBlockHash();.\n");
        pblock->hashPrevBlock = pindexPrev->GetBlockHash();

		if (!fProofOfStake)
		{
			UpdateTime(pblock, pindexPrev);

			if (!Params().AllowMinDifficultyBlocks()) pblock->nBits = GetNextWorkRequired(pindexPrev);
		}

		LogPrintf("CreateNewBlock(): pblock->nNonce = GetRand(1000000000) + 100;\n");
        pblock->nNonce = GetRand(1000000000) + 100; // + 100 to avoid < 100 being generated.
		LogPrintf("CreateNewBlock(): uint256 nCheckpoint = 0;\n");
        uint256 nCheckpoint = 0;

        if(fZerocoinActive)
		{
			AccumulatorMap mapAccumulators;
			if (GetTime() > GetSporkValue(SPORK_16_ZEROCOIN_MAINTENANCE_MODE))
			{
				LogPrintf("CreateNewBlock(): nCheckpoint = chainActive[nHeight - 1]->nAccumulatorCheckpoint;\n");
				nCheckpoint = chainActive[nHeight - 1]->nAccumulatorCheckpoint;
			}
			else if(!CalculateAccumulatorCheckpoint(nHeight, nCheckpoint, mapAccumulators))
			{
				LogPrintf("%s: failed to get accumulator checkpoint\n", __func__);
				nCheckpoint = chainActive[nHeight - 1]->nAccumulatorCheckpoint;
			}
        }
		LogPrintf("CreateNewBlock(): nAccumulatorCheckpoint calculated.\n");
		pblock->nAccumulatorCheckpoint = nCheckpoint;
        pblocktemplate->vTxSigOps[0] = GetLegacySigOpCount(pblock->vtx[0]);

		if (chainActive.Height() >= nHeight)
		{
			LogPrintf("CreateNewBlock(): Someone else created a block and height changed meanwhile, best abandon this block.\n");
			mempool.clear();
			return NULL; 
		}
		LogPrintf("CreateNewBlock(): About to TestBlockValidity.\n");
        CValidationState state;
        if (!TestBlockValidity(state, *pblock, pindexPrev, false, false)) {
            LogPrintf("CreateNewBlock() : TestBlockValidity failed\n");
            mempool.clear();
            return NULL;
        }
    }

    return pblocktemplate.release();
}

void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock) {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight + 1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}

#ifdef ENABLE_WALLET
//////////////////////////////////////////////////////////////////////////////
//
// Internal miner
//
double dHashesPerSec = 0.0;
int64_t nHPSTimerStart = 0;

CBlockTemplate* CreateNewBlockWithKey(CReserveKey& reservekey, CWallet* pwallet, bool fProofOfStake)
{
    CPubKey pubkey;
    if (!reservekey.GetReservedKey(pubkey))
        return NULL;
    CScript scriptPubKey = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;
    return CreateNewBlock(scriptPubKey, pwallet, fProofOfStake);
}

bool ProcessBlockFound(CBlock* pblock, CWallet& wallet)
{
    LogPrintf("%s\n", pblock->ToString());

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash())
            return error("Bitcoin 2 Miner : generated block is stale");
    }

	LogPrintf("generated %s\n", FormatMoney(pblock->vtx[1].vout[1].nValue));

    // Track how many getdata requests this block gets
    {
        LOCK(wallet.cs_wallet);
        wallet.mapRequestCount[pblock->GetHash()] = 0;
    }

    // Inform about the new block
    GetMainSignals().BlockFound(pblock->GetHash());

    // Process this block the same as if we had received it from another node
    CValidationState state;
	LogPrint("masternode", "%s: calling ProcessNewBlock\n", __func__);
    if (!ProcessNewBlock(state, NULL, pblock))
        return error("Bitcoin2Miner : ProcessNewBlock, block not accepted");
	LogPrint("masternode", "ProcessBlockFound - for (CNode* node : vNodes) {\n");

	// PoW: Remove key from key pool
	//reservekey.KeepKey();

	LOCK(cs_vNodes);
	for (CNode* node : vNodes) {
		node->PushInventory(CInv(MSG_BLOCK, pblock->GetHash()));
	}
	LogPrint("masternode", "ProcessBlockFound - done. return true\n");
    return true;
}

bool fGenerateBitcoins = false;
void BitcoinMiner(CWallet* pwallet, bool fProofOfStake)
{
    LogPrintf("BTC2Miner started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("BTC2-miner");

    // Each thread has its own counter
    unsigned int nExtraNonce = 0;

    //control the amount of times the client will check for mintable coins
    static bool fMintableCoins = false;
    static int nMintableLastCheckHeight = 0;

    while (fGenerateBitcoins || fProofOfStake)
	{
        if (fProofOfStake)
		{
            if (chainActive.Tip()->nHeight < Params().LAST_POW_BLOCK()) {
                MilliSleep(10000);
                continue;
            }

			while (chainActive.Tip()->nTime < 1519096403 || (Params().MiningRequiresPeers() && vNodes.empty()) || pwallet->IsLocked() || !fMintableCoins || nReserveBalance >= pwallet->GetBalance() || (Params().MiningRequiresPeers() && !masternodeSync.IsSynced()))
			{
                MilliSleep(5000);

				if (!fMintableCoins && nMintableLastCheckHeight < chainActive.Tip()->nHeight) // Check once per block.
				{
					nMintableLastCheckHeight = chainActive.Tip()->nHeight;
					fMintableCoins = pwallet->MintableCoins();
				}
            }

			while(LastHashedBlockHeight == chainActive.Tip()->nHeight) // see if bestblock has been hashed yet
            {
				unsigned int nMaxHashDivNow = (GetTime() + nMaxStakingFutureDriftv3) / nStakeInterval;
				unsigned int nPreviousHashDiv = LastHashedBlockTime / nStakeInterval;
				if (nMaxHashDivNow <= nPreviousHashDiv)
				{
					// Sleep only 1 sec at a time, in case a new block comes in or we can hash again.
					MilliSleep(1000);
					continue;
				}
				else break;
            }
        }

        //
        // Create new block
        //
        unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();

        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock(CScript(), pwallet, fProofOfStake)); // PoW: CreateNewBlockWithKey(reservekey, pwallet, fProofOfStake)
        if (!pblocktemplate.get())
            continue;

        CBlock* pblock = &pblocktemplate->block;
		CBlockIndex* pindexPrev;
		{
			LOCK(cs_main);
			pindexPrev = chainActive.Tip();
			if (!pindexPrev) continue;
		}

		IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);
        //Stake miner main
        if (fProofOfStake) {
            LogPrintf("CPUMiner : proof-of-stake block found %s \n", pblock->GetHash().ToString().c_str());

            if (!pblock->SignBlock(*pwallet)) {
                LogPrintf("BitcoinMiner(): Signing new block failed \n");
                continue;
            }

            SetThreadPriority(THREAD_PRIORITY_NORMAL);
            ProcessBlockFound(pblock, *pwallet);
            SetThreadPriority(THREAD_PRIORITY_LOWEST);

            continue;
        }

        LogPrintf("Running Bitcoin2Miner with %u transactions in block (%u bytes)\n", pblock->vtx.size(),
            ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION));

        //
        // Search
        //
        int64_t nStart = GetTime();
        uint256 hashTarget = uint256().SetCompact(pblock->nBits);
        while (true) {
            unsigned int nHashesDone = 0;

            uint256 hash;
            while (true) {
                hash = pblock->GetHash();
                if (hash <= hashTarget) {
                    // Found a solution
                    SetThreadPriority(THREAD_PRIORITY_NORMAL);
                    //LogPrintf("BitcoinMiner:\n");
                    //LogPrintf("proof-of-work found  \n  hash: %s  \ntarget: %s\n", hash.GetHex(), hashTarget.GetHex());
                    ProcessBlockFound(pblock, *pwallet);
                    SetThreadPriority(THREAD_PRIORITY_LOWEST);

                    // In regression test mode, stop mining after a block is found. This
                    // allows developers to controllably generate a block on demand.
                    if (Params().MineBlocksOnDemand())
                        throw boost::thread_interrupted();

                    break;
                }
                pblock->nNonce += 1;
                nHashesDone += 1;
                if ((pblock->nNonce & 0xFF) == 0)
                    break;
            }

            // Meter hashes/sec
            static int64_t nHashCounter;
            if (nHPSTimerStart == 0) {
                nHPSTimerStart = GetTimeMillis();
                nHashCounter = 0;
            } else
                nHashCounter += nHashesDone;
            if (GetTimeMillis() - nHPSTimerStart > 4000) {
                static CCriticalSection cs;
                {
                    LOCK(cs);
                    if (GetTimeMillis() - nHPSTimerStart > 4000) {
                        dHashesPerSec = 1000.0 * nHashCounter / (GetTimeMillis() - nHPSTimerStart);
                        nHPSTimerStart = GetTimeMillis();
                        nHashCounter = 0;
                        static int64_t nLogTime;
                        if (GetTime() - nLogTime > 30 * 60) {
                            nLogTime = GetTime();
                            LogPrintf("hashmeter %6.0f khash/s\n", dHashesPerSec / 1000.0);
                        }
                    }
                }
            }

            // Check for stop or if block needs to be rebuilt
            boost::this_thread::interruption_point();
            // Regtest mode doesn't require peers
            if (vNodes.empty() && Params().MiningRequiresPeers())
                break;
            if (pblock->nNonce >= 0xffff0000)
                break;
            if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60)
                break;
            if (pindexPrev != chainActive.Tip())
                break;

            // Update nTime every few seconds
            UpdateTime(pblock, pindexPrev);
            if (Params().AllowMinDifficultyBlocks()) {
                // Changing pblock->nTime can change work required on testnet:
                hashTarget.SetCompact(pblock->nBits);
            }
        }
    }
}

void static ThreadBitcoinMiner(void* parg)
{
    boost::this_thread::interruption_point();
    CWallet* pwallet = (CWallet*)parg;
	if (chainActive.Tip()->nHeight <= Params().LAST_POW_BLOCK())
	{
		try {
			BitcoinMiner(pwallet, false);
			boost::this_thread::interruption_point();
		}
		catch (std::exception& e) {
			LogPrintf("ThreadBitcoinMiner() exception");
		}
		catch (...) {
			LogPrintf("ThreadBitcoinMiner() exception");
		}

		LogPrintf("ThreadBitcoinMiner exiting\n");
	}
}

void GenerateBitcoins(bool fGenerate, CWallet* pwallet, int nThreads)
{
    static boost::thread_group* minerThreads = NULL;
    fGenerateBitcoins = fGenerate;

    if (nThreads < 0) {
        // In regtest threads defaults to 1
        if (Params().DefaultMinerThreads())
            nThreads = Params().DefaultMinerThreads();
        else
            nThreads = boost::thread::hardware_concurrency();
    }

    if (minerThreads != NULL) {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = NULL;
    }

    if (nThreads == 0 || !fGenerate)
        return;

    minerThreads = new boost::thread_group();
    for (int i = 0; i < nThreads; i++)
        minerThreads->create_thread(boost::bind(&ThreadBitcoinMiner, pwallet));
}

#endif // ENABLE_WALLET
