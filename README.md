Bitcoin 2 integration/staging tree
=====================================

www.bitc2.org

What is Bitcoin 2?
----------------

Bitcoin 2 is a digital currency that enables nearly instant, private payments to
anyone, anywhere in the world. Bitcoin 2 uses peer-to-peer technology to operate
with no central authority: managing transactions and issuing money are carried
out collectively by the network. Bitcoin 2 Core is the name of the open source
software which enables the use of this currency.

For more information, as well as a binary version of the Bitcoin 2 Core software, see www.bitc2.org

## What are the main differences between Bitcoin and Bitcoin 2?

Bitcoin 2 has much lower fees and up to 40 times higher limit of transactions per second than Bitcoin.

Bitcoin 2 also implements Masternodes, which enable the Swift TX feature along with adding to the speed, connectivity and redundancy of the network. The Swift TX feature allows Masternodes to confirm your transaction almost immediately so that it can be considered confirmed even before it shows in a block. This kind of a transaction is good for Point of Sale situations in the real world. Anybody with 1000 BTC2 can run a Masternode.

Bitcoin 2 uses a custom, secure Proof of Stake algorithm. Greatly reducing electricity use compared to Bitcoin. This also enables anyone to run the core software and create blocks, without needing a ASIC miner. A laptop is enough.

Unlike Bitcoin, Bitcoin 2 supports truly anonymous transactions. What zBTC2 provides is a protocol-level coin mixing service using zero knowledge proofs to sever the link between the sender and the receiver with 100% anonymity and untraceability. [Technical paper about zBTC2](https://www.bitc2.org/zBTC2-Bitcoin-2-Zerocoin-Protocol).

The Bitcoin 2 blockchain is currently only about 1.5 GB and yet the balances of Bitcoin as of block 507850 were recorded to it. Bitcoin 2 effectively has replay protection.

Blocks are created around once per minute instead of once per 10 minutes, this means that transactions confirm faster. Bitcoin 2's mining/forging difficulty is adjusted every block.

Bitcoin 2 does not support Segwit or the Bech32 address format but other Bitcoin addresses are valid Bitcoin 2 addresses.

[Block rewards structure](https://www.bitc2.org/bitcoin2-block-rewards)

License
-------

Bitcoin 2 is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/bitcoin/bitcoin/tags) are created
regularly to indicate new official, stable release versions of Bitcoin 2 Core.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.
