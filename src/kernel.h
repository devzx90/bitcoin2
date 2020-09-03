// Copyright (c) 2012-2013 The PPCoin developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2017-2020 The Bitcoin 2 developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KERNEL_H
#define BITCOIN_KERNEL_H

#include "main.h"


// MODIFIER_INTERVAL: time to elapse before new modifier is computed
static const unsigned int MODIFIER_INTERVAL = 60;
static const unsigned int MODIFIER_INTERVAL_TESTNET = 60;
extern unsigned int nModifierInterval;
extern unsigned int getIntervalVersion(bool fTestNet);

// MODIFIER_INTERVAL_RATIO:
// ratio of group interval length between the last group and the first group
static const int MODIFIER_INTERVAL_RATIO = 3;

// Compute the hash modifier for proof-of-stake
bool CalculateStakeModifierV3(uint64_t& nStakeModifier, const CBlockIndex* pindexPrev);
uint256 ComputeStakeModifierV2(const CBlockIndex* pindexPrev, const uint256& kernel);
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier);

// Check whether stake kernel meets hash target
// Sets hashProofOfStake on success return
bool stakeTargetHit(const uint256& hashProofOfStake, const int64_t& nValueIn, const uint256& bnTarget);
bool CheckStakeKernelHash(unsigned int nBits, const CBlock& blockFrom, const CTransaction& txPrev, const COutPoint& prevout, unsigned int& nTimeTx, bool fCheck, uint256& hashProofOfStake);
bool CheckStakeKernelHashV2(unsigned int nBits, uint256& StakeModifierV2, unsigned int nChainTime, unsigned int nTimeBlockFrom, CAmount theValueIn, const COutPoint& prevout, unsigned int& nTimeTx, bool fCheck, uint256& hashProofOfStake);
bool CheckStakeKernelHashV3(unsigned int nBits, uint256& StakeModifierV2, uint64_t nStakeModifier, unsigned int nTimeBlockFrom, CAmount theValueIn, const COutPoint& prevout, unsigned int& nTimeTx, bool fCheck, uint256& hashProofOfStake);

// Check kernel hash target and coinstake signature
// Sets hashProofOfStake on success return
bool CheckProofOfStake(const CBlock& block, CBlockIndex* pindexPrev, uint256& hashProofOfStake);
#endif // BITCOIN_KERNEL_H
