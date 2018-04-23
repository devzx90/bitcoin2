# Seeds

Utility to generate the seeds.txt list that is compiled into the client
(see [src/chainparamsseeds.h](/src/chainparamsseeds.h) and other utilities in [contrib/seeds](/contrib/seeds)).

Be sure to update `PATTERN_AGENT` in `makeseeds.py` to include the current version,
and remove old versions as necessary.

The seeds compiled into the release are created like this:

    Script to generate list of seed nodes for chainparams.cpp.
    This script expects two text files in the directory that is passed as an
    argument:
        nodes_main.txt
        nodes_test.txt
    These files must consist of lines in the format
        <ip>
        <ip>:<port>
        [<ipv6>]
        [<ipv6>]:<port>
        <onion>.onion
        0xDDBBCCAA (IPv4 little-endian old pnSeeds format)
    
    python3 generate-seeds.py . > ../../src/chainparamsseeds.h

## Dependencies

Ubuntu:

    sudo apt-get install python3-dnspython
