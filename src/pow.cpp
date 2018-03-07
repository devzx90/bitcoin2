// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin developers
// Copyright (c) 2017-2018 The Bitcoin 2 developers
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


unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock)
{
	// Bigger difficulty number means less work.
    const CBlockIndex* BlockLastSolved = pindexLast;

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight <= Params().LAST_POW_BLOCK()) {
        return Params().ProofOfWorkLimit().GetCompact();
    }

	int64_t nTargetSpacing = 60;

	// Limit adjustment step
	int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();
	if (nActualTimespan < 30) nActualTimespan = 30;
	else if (nActualTimespan > 120) nActualTimespan = 120;

	// Retarget
	const uint256 bnPowLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff") // Same as in Bitcoin
	uint256 bnNew;
	bnNew.SetCompact(pindexLast->nBits);
	bnNew *= nActualTimespan;
	bnNew /= nTargetSpacing;

	if (bnNew > bnPowLimit) bnNew = bnPowLimit;

	LogPrintf("difficulty: %u\n", bnNew.GetCompact());
	return bnNew.GetCompact();



	// Bitcoin 2: target change every block, with a maximum change of +100% or -50%.
	/*int64_t nActualSpacing = 0;
	if (pindexLast->nHeight != 0)
		nActualSpacing = pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();

	if (nActualSpacing < 0) nActualSpacing = 1;

    uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);

	if (pindexLast->nHeight == Params().LAST_POW_BLOCK() + 2 && nActualSpacing < nTargetSpacing) bnNew *= (nTargetSpacing / nActualSpacing); // Initial adjustment.
	else
	{
		if (nActualSpacing <= nTargetSpacing / 2) bnNew /= 2;
		else if (nActualSpacing >= nTargetSpacing * 2) bnNew *= 2;
		else
		{
			double factor = (double)nTargetSpacing / (double)nActualSpacing;
			uint64_t intfactor = factor * 10000;
			bnNew *= 10000;
			bnNew /= intfactor;
		}
	}

	if (bnNew == 0 || bnNew.GetCompact() > MaxDifficulty)
	{
		LogPrintf("difficulty was set to Maximum\n");
		bnNew.SetCompact(MaxDifficulty);
	}
	LogPrintf("difficulty: %u\n", bnNew.GetCompact());
    return bnNew.GetCompact();*/
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
        return false; // error("CheckProofOfWork() : nBits below minimum work");

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
