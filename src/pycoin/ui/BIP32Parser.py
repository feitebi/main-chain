from pycoin.encoding.hexbytes import h2b

from pycoin.ui.Parser import Parser, make_base58_prefixes


class BIP32Parser(Parser):
    TYPE = "bip32"

    def __init__(self, generator, bip32_private_prefix, bip32_public_prefix, bip32node_class):
        self._generator = generator
        self._bip32node_class = bip32node_class
        self._base58_prefixes = make_base58_prefixes([
            (bip32_private_prefix, self.info_for_private),
            (bip32_public_prefix, self.info_for_public),
        ])
        self._colon_prefixes = dict(H=[self.info_for_H], P=[self.info_for_P])

    def info_for_private(self, data):
        return self.info_for_data(data, is_private=True)

    def info_for_public(self, data):
        return self.info_for_data(data, is_private=False)

    def info_for_data(self, data, is_private):
        return dict(type="key", key_type="bip32", bip32_type="plain", is_private=is_private,
                    key_class=self._bip32node_class, create_f=lambda: self._bip32node_class.deserialize(data=data))

    def info_for_H(self, prefix, data):
        bin_data = h2b(data)
        kwargs = dict(generator=self._generator, master_secret=bin_data)
        return dict(type="key", key_type="bip32", bip32_type="seeded", is_private=True, key_class=self._bip32node_class,
                    seed_type="hex", create_f=lambda: self._bip32node_class.from_master_secret(**kwargs),
                    kwargs=kwargs)

    def info_for_P(self, prefix, data):
        kwargs = dict(generator=self._generator, master_secret=data.encode("utf8"))
        return dict(type="key", key_type="bip32", bip32_type="seeded", is_private=True, key_class=self._bip32node_class,
                    seed_type="text", create_f=lambda: self._bip32node_class.from_master_secret(**kwargs),
                    kwargs=kwargs)
