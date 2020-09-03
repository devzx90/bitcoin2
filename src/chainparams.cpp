// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2017-2019 The Bitcoin 2 developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzerocoin/Params.h"
#include "chainparams.h"
#include "random.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

/**
 * Main network
 */

//! Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress>& vSeedsOut, const SeedSpec6* data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7 * 24 * 60 * 60;
    for (unsigned int i = 0; i < count; i++) {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

//   What makes a good checkpoint block?
// + Is surrounded by blocks with reasonable timestamps
//   (no blocks before with a timestamp after, none after with
//    timestamp before)
// + Contains no strange transactions
static Checkpoints::MapCheckpoints mapCheckpoints =
	boost::assign::map_list_of(0, uint256("3ee1620fa1706966da5e5182a664220691491bdfd9289a73cadf6244fa5dccb5"))
	(102, uint256("60d4d5d791b05a8ffd1505a1dd46af08d406296a84aad7f1fd4e45da113039ab"))
	(202, uint256("054f47c07d6b324ce27c510a85353acf9369c8b8b787739afb69328692a20441"))
	(500, uint256("674f8e65e01ac23efa66e9d03f98fc795a4fd78f5495ce0e40a4d93fc4596249"))
	(1000, uint256("5a389fb75660ba5203927aff0e91b24193d187adca415c8c5415cdd5c6b88eb3"))
	(1190, uint256("c935255f9be0cfa15350ebfa2f3c07aa00e356e3767d373fbf7c65790fcd343"))
	(1390, uint256("318e54323519a7d35f8d5b6c1a515b03722f9464f3bcacc819b6124a59ba0b21"))
	(2244, uint256("9889988d627880d89d61b635b7bc47f1807586926cdd72ef440f887d4f4d81a6"))
	(6436, uint256("8bb42ae275c6b7de44e38a94dc4c674e94773f6cdf567b3315ce0934c850d101"))
	(129786, uint256("d7e8ec1d12c7e945b4f8bb682197177640f436309eabd6b4559ed44afb73a83f"))
	(554450, uint256("4f059f53ec53736e47620b0ef1f36956e0d4a9050d7d1fc125571726ed8992a6"))
	(625053, uint256("16167d70bd11524560179b7bd474e85d50124930e7b27e4e107c6048c915ea32"))
	(628729, uint256("6cba37f16a00e0b9d853dc220c2b88ab5b53217fcc268aabe79d9c18c4b62093"))
	(654956, uint256("c8ea8f9dc68a81bec35c07d29143f8e2355e7c175e747360b39890429fce0f62"))
	(654957, uint256("6ca8a0d29864b7a5bc8c86c50ba0e2888b5464c0ea66285fab7c5e5cd4b0906f"))
	(657352, uint256("9e073ca8980355b4480fb068380c71be9b91f8352151bdddfa73532ae3127838"))
	(856863, uint256("a660d7ea02004af6d4691f6da86f844fc7139267ce4b8b6f524a1e4590ec92ed"))
	(1016009, uint256("1c71373d1e224406db1e8b3bf6dd59b6eda67d9b67ffb442b143888709680da6"))
	(1122976, uint256("49d4f44ce66ed7232a3a98c94145b653b2f3c0c2b4c4adab3a15df9dd91ea309"));

static const Checkpoints::CCheckpointData data = {
    &mapCheckpoints,
	1598950710, // * UNIX timestamp of last checkpoint block. obtained by: date +%s.
	2299627,    // * total number of transactions between genesis and last checkpoint
                //   (the tx=... number in the UpdateTip: debug.log lines)
	2900        // * estimated number of transactions per day after checkpoint. 2 transactions per minute = 2880
};

static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
    boost::assign::map_list_of(0, uint256("3ee1620fa1706966da5e5182a664220691491bdfd9289a73cadf6244fa5dccb5"));

static const Checkpoints::CCheckpointData dataTestnet = {
    &mapCheckpointsTestnet,
	1518621005,
    0,
    250};

static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
    boost::assign::map_list_of(0, uint256("3ee1620fa1706966da5e5182a664220691491bdfd9289a73cadf6244fa5dccb5"));

static const Checkpoints::CCheckpointData dataRegtest = {
    &mapCheckpointsRegtest,
	1518621005,
    0,
    100};

libzerocoin::ZerocoinParams* CChainParams::Zerocoin_Params() const
{
    assert(this);
    static CBigNum bnTrustedModulus(zerocoinModulus);
    static libzerocoin::ZerocoinParams ZCParams = libzerocoin::ZerocoinParams(bnTrustedModulus);

    return &ZCParams;
}

class CMainParams : public CChainParams
{
public:
    CMainParams()
    {
        networkID = CBaseChainParams::MAIN;
        strNetworkID = "main";
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
		 These values are unique to BTC2.
         */
        pchMessageStart[0] = 232;
        pchMessageStart[1] = 196;
        pchMessageStart[2] = 240;
        pchMessageStart[3] = 246;
        vAlertPubKey = ParseHex("0481144fe24738164032b28205ed30fbb63db0d6764639c3dc6665286547de8bc0bdda77b75f912135fe3c8edde94e9f0ba45da9431e99313c3fb34b278463ea61"); // BTC2
        nDefaultPort = 8333; // 44798 should be good. 8333 used by Bitcoin, Bitcoin Gold and Bitcoin Cash. 49144 used by BTC2 before.
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        nMaxReorganizationDepth = 100;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 0; // Defaults to max that the hardware can efficiently use.
        nTargetTimespan = 60;
        nTargetSpacing = 60;  // 1 minute
        nMaturity = 100;
        nMaxMoneyOut = 21000000 * COIN; // This is the maximum supply.

        /** Height or Time Based Activations **/
        nLastPOWBlock = 1390;
        nModifierUpdateBlock = 1391;
        nZerocoinStartHeight = 1391;
        nZerocoinStartTime = 1519330596; // 22nd of February 2018

        /**
         * Build the genesis block. 0 output in Bitcoin 2.
         */
        const char* pszTimestamp = "Therapy Reverses Alzheimer's Brain Plaque Buildup -- in Mice";
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 0;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("0461465ccdd748883bc2ef4a68266d4c87324a1a7fca7c48f502c82c139a44890dcc8fb7e2307375618ff3cf7bc8a710ad28c438aab9c009bbdc70c7afdbd19bfd") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.nVersion = 1;
        genesis.nTime = 1518621005;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("3ee1620fa1706966da5e5182a664220691491bdfd9289a73cadf6244fa5dccb5"));
        assert(genesis.hashMerkleRoot == uint256("ce76be25027ca16966cec5d8a05c1eb36c545fe541a9fa07c4889933062823bb"));

		vSeeds.push_back(CDNSSeedData("seed.bitc2.org", "seed.bitc2.org"));  // Primary DNS Seeder
		vSeeds.push_back(CDNSSeedData("seeder.bitc2.org", "seeder.bitc2.org"));  // Secondary DNS Seeder on another server.
		convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main)); // Fixed seed IP addresses as a fallback.

		// bitcoin
		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 0);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 5);
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 128);
		base58Prefixes[EXT_PUBLIC_KEY] = { 0x04, 0x88, 0xB2, 0x1E };
		base58Prefixes[EXT_SECRET_KEY] = { 0x04, 0x88, 0xAD, 0xE4 };

        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fSkipProofOfWorkCheck = false;
        fTestnetToBeDeprecatedFieldRPC = false;
        fHeadersFirstSyncingActive = false;

        nPoolMaxTransactions = 3;
		strSporkKey = "0475d8b895b80516025a2f33d647551d9688ff16f620e0035132d29ea2fc9c7e4b2c3e80a2938f4595c60f52909d0bd8659b5cadc838425aced67081a8570b8e50";
        strObfuscationPoolDummyAddress = "1CzYXueWh2cT3p9LZRVQZvKmJsPijdiTV";
        nStartMasternodePayments = 1519330596; // 22nd of February 2018

        /** Zerocoin */
		zerocoinModulus = "c930c8ac8b35d1593d5aaad840457bf311d7af1e5eaf9e9bcee38e0ceb76cb1efbd71fe8bb2dc03b92169cd0ef67972260b8b5c53823a1010153059875db9f0"
			"9b36dd9ea6355016b063bf3b511928f7143632aeb3381c764b9b334fe52b2b496969e8b78f554aecb16c5501664a690528b93c75145ede5f562a1793162137863054add8cf88b"
			"9d1fbc622f0359d674c08085fb3797b75af59811e35f239807a9388d9a0aec09fd81f3850b53bd6b6970cf9d6f8eb7a30b7c5c26390a6325e0e429d1beababc0575764e6be03e"
			"f8e8df6f98ffa4fb1b90f8c8dceb41a5b02722e692fc89bcb0a4a4959907e1293c05db682c0ffe070fe2bfebe78546cd934b741079390da0b14ec95346f30f3add4e54aa548794"
			"9c34a491a46c77060cf810321e8b83fe5bbd0b1101f3b202ade8f89c948a917639b01cb8dc5cdcb7629c4a6dc50118e24e01730dbb7d81a496ede24a185779af3d41d7213fcaeb6"
			"3c1905f246732bbe432331914bae730108f2a88f1bec5368184435b782be6f9ad427db01a5";

        nMaxZerocoinSpendsPerTransaction = 17; // Assume about 20kb each
        nMinZerocoinMintFee = 100000; // higher fee required for zerocoin mints. 0.001 BTC2.
        nMintRequiredConfirmations = 20; //the maximum amount of confirmations until accumulated in 19
        nRequiredAccumulation = 1;
        nDefaultSecurityLevel = 100; //full security level for accumulators
        nZerocoinHeaderVersion = 4; //Block headers must be this version once zerocoin is active
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return data;
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams
{
public:
    CTestNetParams()
    {
        networkID = CBaseChainParams::TESTNET;
        strNetworkID = "test";
		pchMessageStart[0] = 32;
		pchMessageStart[1] = 96;
		pchMessageStart[2] = 240;
		pchMessageStart[3] = 146;
        vAlertPubKey = ParseHex("04bf10d29fbbc541d2e7da47574cb6ebbb056eb17628cb0837af00529e29ea04d74ab1714d250d049060d58e296b6fd3624e926d99a194c678c880b65564e9ef4a"); // BTC2
        nDefaultPort = 40446;
        nEnforceBlockUpgradeMajority = 51;
        nRejectBlockOutdatedMajority = 75;
        nToCheckBlockUpgradeMajority = 100;
        nMinerThreads = 0;
        nTargetTimespan = 60;
        nTargetSpacing = 60;
        nLastPOWBlock = 200;
        nMaturity = 15;
        nModifierUpdateBlock = 1391;
        nMaxMoneyOut = 21000000 * COIN;
        nZerocoinStartHeight = 1391;
        nZerocoinStartTime = 1519330596;

        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1518621005;
        genesis.nNonce = 0;

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("3ee1620fa1706966da5e5182a664220691491bdfd9289a73cadf6244fa5dccb5"));

        vFixedSeeds.clear();
        vSeeds.clear();

		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
		base58Prefixes[EXT_PUBLIC_KEY] = { 0x04, 0x35, 0x87, 0xCF };
		base58Prefixes[EXT_SECRET_KEY] = { 0x04, 0x35, 0x83, 0x94 };
        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 2;
        strSporkKey = "15FWA7rK3CFFGyn1ELrx8rbBRYhFGLpfwx";
        nStartMasternodePayments = 1519330596; // 22nd of February 2018
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataTestnet;
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CTestNetParams
{
public:
    CRegTestParams()
    {
        networkID = CBaseChainParams::REGTEST;
        strNetworkID = "regtest";
        strNetworkID = "regtest";
		pchMessageStart[0] = 132;
		pchMessageStart[1] = 146;
		pchMessageStart[2] = 140;
		pchMessageStart[3] = 227;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 1;
        nTargetTimespan = 24 * 60 * 60; // 1 day
        nTargetSpacing = 60;        // 1 minute
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        genesis.nTime = 1518621005;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 0;

		hashGenesisBlock = genesis.GetHash();
		nDefaultPort = 51476;
        assert(hashGenesisBlock == uint256("3ee1620fa1706966da5e5182a664220691491bdfd9289a73cadf6244fa5dccb5"));

        vFixedSeeds.clear(); //! Testnet mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Testnet mode doesn't have any DNS seeds.

		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
		base58Prefixes[EXT_PUBLIC_KEY] = { 0x04, 0x35, 0x87, 0xCF };
		base58Prefixes[EXT_SECRET_KEY] = { 0x04, 0x35, 0x83, 0x94 };

        fMiningRequiresPeers = false;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataRegtest;
    }
};
static CRegTestParams regTestParams;

/**
 * Unit test
 */
class CUnitTestParams : public CMainParams, public CModifiableParams
{
public:
    CUnitTestParams()
    {
        networkID = CBaseChainParams::UNITTEST;
        strNetworkID = "unittest";
        nDefaultPort = 51479;
        vFixedSeeds.clear(); //! Unit test mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Unit test mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fAllowMinDifficultyBlocks = false;
        fMineBlocksOnDemand = true;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        // UnitTest share the same checkpoints as MAIN
        return data;
    }

    //! Published setters to allow changing values in unit test cases
    virtual void setEnforceBlockUpgradeMajority(int anEnforceBlockUpgradeMajority) { nEnforceBlockUpgradeMajority = anEnforceBlockUpgradeMajority; }
    virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority) { nRejectBlockOutdatedMajority = anRejectBlockOutdatedMajority; }
    virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority) { nToCheckBlockUpgradeMajority = anToCheckBlockUpgradeMajority; }
    virtual void setDefaultConsistencyChecks(bool afDefaultConsistencyChecks) { fDefaultConsistencyChecks = afDefaultConsistencyChecks; }
    virtual void setAllowMinDifficultyBlocks(bool afAllowMinDifficultyBlocks) { fAllowMinDifficultyBlocks = afAllowMinDifficultyBlocks; }
    virtual void setSkipProofOfWorkCheck(bool afSkipProofOfWorkCheck) { fSkipProofOfWorkCheck = afSkipProofOfWorkCheck; }
};
static CUnitTestParams unitTestParams;


static CChainParams* pCurrentParams = 0;

CModifiableParams* ModifiableParams()
{
    assert(pCurrentParams);
    assert(pCurrentParams == &unitTestParams);
    return (CModifiableParams*)&unitTestParams;
}

const CChainParams& Params()
{
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(CBaseChainParams::Network network)
{
    switch (network) {
    case CBaseChainParams::MAIN:
        return mainParams;
    case CBaseChainParams::TESTNET:
        return testNetParams;
    case CBaseChainParams::REGTEST:
        return regTestParams;
    case CBaseChainParams::UNITTEST:
        return unitTestParams;
    default:
        assert(false && "Unimplemented network");
        return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);
    return true;
}
