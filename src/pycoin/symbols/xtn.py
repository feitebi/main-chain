from pycoin.networks.bitcoinish import create_bitcoinish_network


network = create_bitcoinish_network(
    symbol="XTN", network_name="Bitcoin", subnet_name="testnet3",
    wif_prefix_hex="ef", sec_prefix="XTNSEC:", address_prefix_hex="6f", pay_to_script_prefix_hex="c4",
    bip32_prv_prefix_hex="04358394", bip32_pub_prefix_hex="043587CF", bech32_hrp="tb",
    magic_header_hex="0B110907", default_port=18333,
    dns_bootstrap=[
        "bitcoin.petertodd.org", "testnet-seed.bitcoin.petertodd.org",
        "bluematt.me", "testnet-seed.bluematt.me"
    ])
