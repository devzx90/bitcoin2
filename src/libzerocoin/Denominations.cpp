/**
 * @file       Denominations.cpp
 *
 * @brief      Functions for converting to/from Zerocoin Denominations to other values library.
 *
 * @copyright  Copyright 2017 PIVX Developers
 * @license    This project is released under the MIT license.
 **/

#include "Denominations.h"
#include "amount.h"

namespace libzerocoin {
// All denomination values should only exist in these routines for consistency.
// For serialization/unserialization enums are converted to int (denoted enumvalue in function name)

CoinDenomination IntToZerocoinDenomination(int64_t amount)
{
    CoinDenomination denomination;
    switch (amount) {
    case 5:		denomination = CoinDenomination::ZQ_FIVECENTS; break;
    case 20:	denomination = CoinDenomination::ZQ_TWENTYCENTS; break;
    case 100:	denomination = CoinDenomination::ZQ_ONE; break;
    case 500:	denomination = CoinDenomination::ZQ_FIVE; break;
    case 2000: denomination = CoinDenomination::ZQ_TWENTY; break;
    case 10000: denomination = CoinDenomination::ZQ_ONE_HUNDRED; break;
    case 50000: denomination = CoinDenomination::ZQ_FIVE_HUNDRED; break;
    case 200000: denomination = CoinDenomination::ZQ_TWO_THOUSAND; break;
    default:
        //not a valid denomination
        denomination = CoinDenomination::ZQ_ERROR; break;
    }

    return denomination;
}

int64_t ZerocoinDenominationToInt(const CoinDenomination& denomination)
{
    int64_t Value;
    switch (denomination) {
    case CoinDenomination::ZQ_FIVECENTS: Value = 5; break;
    case CoinDenomination::ZQ_TWENTYCENTS: Value = 20; break;
    case CoinDenomination::ZQ_ONE: Value = 100; break;
    case CoinDenomination::ZQ_FIVE : Value = 500; break;
    case CoinDenomination::ZQ_TWENTY: Value = 2000; break;
    case CoinDenomination::ZQ_ONE_HUNDRED: Value = 10000; break;
    case CoinDenomination::ZQ_FIVE_HUNDRED: Value = 50000; break;
    case CoinDenomination::ZQ_TWO_THOUSAND: Value = 200000; break;
    default:
        // Error Case
        Value = 0; break;
    }
    return Value;
}

CoinDenomination AmountToZerocoinDenomination(CAmount amount)
{
    return IntToZerocoinDenomination(amount / CENT);
}

// return the highest denomination that is less than or equal to the amount given
// use case: converting BTC2 to zBTC2 without user worrying about denomination math themselves
CoinDenomination AmountToClosestDenomination(CAmount nAmount, CAmount& nRemaining)
{
    if (nAmount < 5 * CENT) return ZQ_ERROR;

    CAmount nConvert = nAmount / CENT;
    CoinDenomination denomination = ZQ_ERROR;
    for (unsigned int i = 0; i < zerocoinDenomList.size(); i++) {
        denomination = zerocoinDenomList[i];

        //exact match
        if (nConvert == denomination) {
            nRemaining = 0;
            return denomination;
        }

        //we are beyond the value, use previous denomination
        if (denomination > nConvert && i) {
            CoinDenomination d = zerocoinDenomList[i - 1];
            nRemaining = nConvert - d;
            return d;
        }
    }
    //last denomination, the highest value possible
    nRemaining = nConvert - denomination;
    return denomination;
}

CAmount ZerocoinDenominationToAmount(const CoinDenomination& denomination)
{
    CAmount nValue = CENT * ZerocoinDenominationToInt(denomination);
    return nValue;
}


CoinDenomination get_denomination(std::string denomAmount) {
    int64_t val = std::stoi(denomAmount);
    return IntToZerocoinDenomination(val);
}


int64_t get_amount(std::string denomAmount) {
    int64_t nAmount = 0;
    CoinDenomination denom = get_denomination(denomAmount);
    if (denom == ZQ_ERROR) {
        // SHOULD WE THROW EXCEPTION or Something?
        nAmount = 0;
    } else {
        nAmount = ZerocoinDenominationToAmount(denom);
    }
    return nAmount;
}

} /* namespace libzerocoin */
