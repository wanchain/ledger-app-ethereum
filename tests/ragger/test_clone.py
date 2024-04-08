from web3 import Web3

from ragger.backend import BackendInterface
from ragger.firmware import Firmware
from ragger.navigator import Navigator

from test_sign import common


# Values used across all tests
ADDR = bytes.fromhex("5a321744667052affa8386ed49e00ef223cbffc3")
BIP32_PATH = "m/44'/1001'/0'/0/0"
NONCE = 68
GAS_PRICE = 13
GAS_LIMIT = 21000
VALUE = 31415926913374232


# Legacy Zemu etc with clone: Transfer on Ethereum clone app
def test_clone_etc(firmware: Firmware,
                   backend: BackendInterface,
                   navigator: Navigator,
                   test_name: str):
    tx_params: dict = {
        "nonce": NONCE,
        "gasPrice": Web3.to_wei(GAS_PRICE, 'gwei'),
        "gas": GAS_LIMIT,
        "to": ADDR,
        "value": VALUE,
        "chainId": 108
    }
    common(firmware, backend, navigator, tx_params, test_name)
