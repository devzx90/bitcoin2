// Copyright (c) 2012-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2017-2019 The Bitcoin 2 developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

const int REWARDFORK_BLOCK = 1141660; // Introduced for protocol 70914

/**
 * network protocol versioning
 */

const int PROTOCOL_VERSION = 70914;

//! initial proto version, to be increased after version/verack negotiation
const int INIT_PROTO_VERSION = 209;

//! In this version, 'getheaders' was introduced.
const int GETHEADERS_VERSION = 70077;

//! disconnect from peers older than this proto version
const int MIN_PEER_PROTO_VERSION_BEFORE_ENFORCEMENT = 70913;
const int MIN_PEER_PROTO_VERSION_AFTER_ENFORCEMENT = 70914;

//! nTime field added to CAddress, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
const int CADDR_TIME_VERSION = 31402;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
const int BIP0031_VERSION = 60000;

//! "mempool" command, enhanced "getdata" behavior starts with this version
const int MEMPOOL_GD_VERSION = 60002;

//! "filter*" commands are disabled without NODE_BLOOM after and including this version
const int NO_BLOOM_VERSION = 70005;


#endif // BITCOIN_VERSION_H
