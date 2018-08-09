/**
 * @file       Denominations.h
 *
 * @brief      Denomination info for the Zerocoin library.
 *
 * @copyright  Copyright 2017 PIVX Developers. Copyright 2018 Bitcoin 2 Developers
 * @license    This project is released under the MIT license.
 **/

#ifndef DENOMINATIONS_H_
#define DENOMINATIONS_H_

#include <cstdint>
#include <string>
#include <vector>

namespace libzerocoin {

enum  CoinDenomination { // In cents
	ZQ_ERROR = 0,
	ZQ_FIVECENTS = 5,
	ZQ_TWENTYCENTS = 20,
	ZQ_ONE = 100,
	ZQ_FIVE = 500,
	ZQ_TWENTY = 2000,
	ZQ_ONE_HUNDRED = 10000,
	ZQ_FIVE_HUNDRED = 50000,
	ZQ_TWO_THOUSAND = 200000
};

// Order is with the Smallest Denomination first and is important for a particular routine that this order is maintained
const std::vector<CoinDenomination> zerocoinDenomList = {ZQ_FIVECENTS, ZQ_TWENTYCENTS, ZQ_ONE, ZQ_FIVE, ZQ_TWENTY, ZQ_ONE_HUNDRED, ZQ_FIVE_HUNDRED, ZQ_TWO_THOUSAND};
// These are the max number you'd need at any one Denomination before moving to the higher denomination. Last number is 16, since it's the max number of
// possible spends at the moment. Not used at the moment.
//const std::vector<int> maxCoinsAtDenom   = {3, 4, 4, 3, 4, 4, 3, 16};

int64_t ZerocoinDenominationToInt(const CoinDenomination& denomination);
int64_t ZerocoinDenominationToAmount(const CoinDenomination& denomination);
CoinDenomination IntToZerocoinDenomination(int64_t amount);
CoinDenomination AmountToZerocoinDenomination(int64_t amount);
CoinDenomination AmountToClosestDenomination(int64_t nAmount, int64_t& nRemaining);
CoinDenomination get_denomination(std::string denomAmount);
int64_t get_amount(std::string denomAmount);

} /* namespace libzerocoin */
#endif /* DENOMINATIONS_H_ */
