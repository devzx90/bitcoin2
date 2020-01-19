// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin developers
// Copyright (c) 2017-2020 The Bitcoin 2 developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "chain.h"
#include "chainparams.h"
#include "main.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

#include <math.h>


unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast)
{
	// Bigger difficulty number means less work.
    if (pindexLast == NULL || pindexLast->nHeight <= Params().LAST_POW_BLOCK()) {
        return Params().ProofOfWorkLimit().GetCompact();
    }

	int64_t nTargetSpacing = 60;
	int64_t nActualTimespan;

	////////// Old, version 2.0.0
	if (pindexLast->nHeight < 6538) // CheckWork() will pass Block 6538 and earlier to this old version.
	{
		nActualTimespan = pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();
		if (nActualTimespan < 30) nActualTimespan = 30;
		else if (nActualTimespan > 120) nActualTimespan = 120;

		// Retarget
		const uint256 bnPowLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // Same as in Bitcoin
		uint256 bnNew;
		bnNew.SetCompact(pindexLast->nBits);
		bnNew *= nActualTimespan;
		bnNew /= nTargetSpacing;

		if (bnNew > bnPowLimit) bnNew = bnPowLimit;

		if (pindexLast->nHeight < Params().LAST_POW_BLOCK() + 2 && bnNew.GetCompact() > 469827583U) bnNew.SetCompact(469827583U); // PoS Starting difficulty can't be too low.

		return bnNew.GetCompact();
	}
	/////////////
	
	if (pindexLast->nHeight + 1 == GetSporkValue(SPORK_13_STAKING_PROTOCOL_2)) nActualTimespan = 2000; // Reduce difficulty for a smoother transition to new staking protocol.
	else
	{
		int nHeightFirst = pindexLast->nHeight - 10;

		// Re-target every 10 blocks.
		if (pindexLast->nHeight % 10 != 1) return pindexLast->nBits;

		CBlockIndex* pindexFirst = pindexLast->pprev;
		while (pindexFirst->nHeight > nHeightFirst) {
			pindexFirst = pindexFirst->pprev;
		}
		assert(pindexFirst);

		nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();

		nActualTimespan /= 10;

		// Limit adjustment step
	
		if (nActualTimespan < 30) nActualTimespan = 30;
		else if (nActualTimespan > 120) nActualTimespan = 120;
	}

	// Retarget
	uint256 bnNew;
	
	bnNew.SetCompact(pindexLast->nBits);
	
	if (bnNew == Params().ProofOfWorkLimit() && nActualTimespan >= nTargetSpacing) return bnNew.GetCompact();
	
	bnNew /= nTargetSpacing;
	bnNew *= nActualTimespan;

	if (bnNew > Params().ProofOfWorkLimit()) bnNew = Params().ProofOfWorkLimit();

	//LogPrintf("difficulty: %u\n", bnNew.GetCompact());

	return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    uint256 bnTarget;

    if (Params().SkipProofOfWorkCheck())
        return true;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > Params().ProofOfWorkLimit())
        error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
    if (hash > bnTarget)
        return false; //error("CheckProofOfWork() : hash doesn't match nBits");

    return true;
}

uint256 GetBlockProof(const CBlockIndex& block)
{
    uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}
