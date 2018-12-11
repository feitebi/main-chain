#!/usr/bin/env python

from __future__ import print_function

import argparse
import json
import re
import subprocess
import sys

from pycoin.encoding.hexbytes import b2h, h2b
from pycoin.ui.key_from_text import key_info_from_text
from pycoin.networks.default import get_current_netcode
from pycoin.networks.registry import network_codes, network_for_netcode


SEC_RE = re.compile(r"^(0[23][0-9a-fA-F]{64})|(04[0-9a-fA-F]{128})$")
HASH160_RE = re.compile(r"^([0-9a-fA-F]{40})$")


def gpg_entropy():
    try:
        output = subprocess.Popen(
            ["gpg", "--gen-random", "2", "64"], stdout=subprocess.PIPE).communicate()[0]
        return output
    except OSError:
        sys.stderr.write("warning: can't open gpg, can't use as entropy source\n")
    return b''


def get_entropy():
    entropy = bytearray()
    try:
        entropy.extend(gpg_entropy())
    except Exception:
        print("warning: can't use gpg as entropy source", file=sys.stdout)
    try:
        entropy.extend(open("/dev/random", "rb").read(64))
    except Exception:
        print("warning: can't use /dev/random as entropy source", file=sys.stdout)
    entropy = bytes(entropy)
    if len(entropy) < 64:
        raise OSError("can't find sources of entropy")
    return entropy


def parse_as_number(s):
    try:
        return int(s)
    except ValueError:
        pass
    try:
        return int(s, 16)
    except ValueError:
        pass


def parse_as_secret_exponent(s, generator):
    v = parse_as_number(s)
    if v and 0 < v < generator.order():
        return v


def parse_as_public_pair(s, generator):
    for c in ",/":
        if c in s:
            s0, s1 = s.split(c, 1)
            v0 = parse_as_number(s0)
            if v0:
                if s1 in ("even", "odd"):
                    is_y_odd = (s1 == "odd")
                    return generator.points_for_x(v0)[is_y_odd]
                v1 = parse_as_number(s1)
                if v1:
                    if not generator.contains_point(v0, v1):
                        sys.stderr.write("invalid (x, y) pair\n")
                        sys.exit(1)
                    return (v0, v1)


def create_output(item, key, network, output_key_set, subkey_path=None):
    ui_context = network.ui
    key._ui_context = ui_context
    output_dict = {}
    output_order = []

    def add_output(json_key, value=None, human_readable_key=None):
        if output_key_set and json_key not in output_key_set:
            return
        if human_readable_key is None:
            human_readable_key = json_key.replace("_", " ")
        if value:
            if human_readable_key == "legacy":
                output_dict[json_key.strip()] = value
            else:
                output_dict[json_key.strip().lower()] = value
                output_order.append((json_key.lower(), human_readable_key))

    full_network_name = "%s %s" % (network.network_name, network.subnet_name)
    add_output("input", item)
    add_output("network", full_network_name)
    add_output("symbol", network.symbol)

    if hasattr(key, "hwif"):
        if subkey_path:
            add_output("subkey_path", subkey_path)
        for k, v, text in network.output_for_hwif(key.serialize(), network, subkey_path, add_output):
            add_output(k, v, text)

    secret_exponent = key.secret_exponent()
    if secret_exponent:
        for k, v, text in network.output_for_secret_exponent(secret_exponent):
            add_output(k, v, text)

    public_pair = key.public_pair()
    if public_pair:
        for k, v, text in network.output_for_public_pair(public_pair):
            add_output(k, v, text)
    else:
        for k, v, text in network.output_for_h160(key.hash160()):
            add_output(k, v, text)

    return output_dict, output_order


def dump_output(output_dict, output_order):
    print('')
    max_length = max(len(v[1]) for v in output_order)
    for key, hr_key in output_order:
        space_padding = ' ' * (1 + max_length - len(hr_key))
        val = output_dict.get(key)
        if val is None:
            print(hr_key)
        else:
            if len(val) > 80:
                val = "%s\\\n%s%s" % (val[:66], ' ' * (5 + max_length), val[66:])
            print("%s%s: %s" % (hr_key, space_padding, val))


def create_parser():
    codes = network_codes()
    parser = argparse.ArgumentParser(
        description='Crypto coin utility ku ("key utility") to show'
        ' information about Bitcoin or other cryptocoin data structures.',
        epilog=('Known networks codes:\n  ' +
                ', '.join(['%s (%s)' % (i, network_for_netcode(i).full_name()) for i in codes]))
    )
    parser.add_argument('-w', "--wallet", help='show just Bitcoin wallet key', action='store_true')
    parser.add_argument('-W', "--wif", help='show just Bitcoin WIF', action='store_true')
    parser.add_argument('-a', "--address", help='show just Bitcoin address', action='store_true')
    parser.add_argument(
        '-u', "--uncompressed", help='show output in uncompressed form', action='store_true')
    parser.add_argument(
        '-P', "--public", help='only show public version of wallet keys', action='store_true')

    parser.add_argument('-j', "--json", help='output as JSON', action='store_true')

    parser.add_argument('-b', "--brief", nargs="*", help='brief output; display a single field')

    parser.add_argument('-s', "--subkey", help='subkey path (example: 0H/2/15-20)', default="")
    parser.add_argument('-n', "--network", help='specify network', choices=codes)
    parser.add_argument(
        "--override-network", help='override detected network type', default=None, choices=codes)

    parser.add_argument(
        'item', nargs="*", help='a BIP0032 wallet key string;'
        ' a WIF;'
        ' a bitcoin address;'
        ' an SEC (ie. a 66 hex chars starting with 02, 03 or a 130 hex chars starting with 04);'
        ' the literal string "create" to create a new wallet key using strong entropy sources;'
        ' P:wallet passphrase (NOT RECOMMENDED);'
        ' H:wallet passphrase in hex (NOT RECOMMENDED);'
        ' E:electrum value (either a master public, master private, or initial data);'
        ' secret_exponent (in decimal or hex);'
        ' x,y where x,y form a public pair (y is a number or one of the strings "even" or "odd");'
        ' hash160 (as 40 hex characters).'
        ' If this argument is missing, input data will be read from stdin.')
    return parser


def _create_bip32(network):
    max_retries = 64
    for _ in range(max_retries):
        try:
            return network.extras.BIP32Node.from_master_secret(get_entropy())
        except ValueError as e:
            continue
    # Probably a bug if we get here
    raise RuntimeError("can't create BIP32 key")


def parse_key(item, networks):
    default_network = networks[0]
    Key = default_network.extras.Key
    generator = Key._default_generator
    if item == 'create':
        return None, _create_bip32(default_network)

    for network, key_info in key_info_from_text(item, networks=networks):
        return network, key_info["create_f"]()

    if HASH160_RE.match(item):
        return None, Key(hash160=h2b(item))

    secret_exponent = parse_as_secret_exponent(item, generator)
    if secret_exponent:
        return None, Key(secret_exponent=secret_exponent)

    if SEC_RE.match(item):
        return None, Key.from_sec(h2b(item))

    public_pair = parse_as_public_pair(item, generator)
    if public_pair:
        return None, Key(public_pair=public_pair)

    return None, None


def generate_output(args, output_dict, output_order):
    if args.json:
        # the python2 version of json.dumps puts an extra blank prior to the end of each line
        # the "replace" is a hack to make python2 produce the same output as python3
        print(json.dumps(output_dict, indent=3, sort_keys=True).replace(" \n", "\n"))
        return

    if len(output_order) == 0:
        print("no output: use -j option to see keys")
    elif len(output_dict) == 1:
        print(output_dict[output_order[0][0]])
    else:
        dump_output(output_dict, output_order)


def ku(args, parser):
    fallback_network = network_for_netcode(args.network or get_current_netcode())
    parse_networks = [fallback_network] + [network_for_netcode(netcode) for netcode in network_codes()]
    if args.network:
        parse_networks = [network_for_netcode(args.network)]

    override_network = None
    if args.override_network:
        # Override the network value, so we can take the same xpubkey and view what
        # the values would be on each other network type.
        override_network = network_for_netcode(args.override_network)

    def parse_stdin():
        return [item for item in sys.stdin.readline().strip().split(' ') if len(item) > 0]

    output_key_set = set(args.brief or [])
    if args.wallet:
        output_key_set.add("wallet_key")
    elif args.wif:
        output_key_set.add("wif_uncompressed" if args.uncompressed else "wif")
    elif args.address:
        output_key_set.add("address" + ("_uncompressed" if args.uncompressed else ""))

    items = args.item if len(args.item) > 0 else parse_stdin()

    for item in items:
        key_network, key = parse_key(item, parse_networks)
        if key is None:
            print("can't parse %s" % item, file=sys.stderr)
            continue

        display_network = override_network or key_network or fallback_network

        for key in key.subkeys(args.subkey):
            if args.public:
                key = key.public_copy()

            output_dict, output_order = create_output(item, key, display_network, output_key_set)

            generate_output(args, output_dict, output_order)


def main():
    parser = create_parser()
    args = parser.parse_args()
    ku(args, parser)


if __name__ == '__main__':
    main()
