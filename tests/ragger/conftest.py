import sys
from pathlib import Path
import warnings

from ragger.conftest import configuration

#######################
# CONFIGURATION START #
#######################

# You can configure optional parameters by overriding the value of
# ragger.configuration.OPTIONAL_CONFIGURATION
# Please refer to ragger/conftest/configuration.py for their descriptions and accepted values

def pytest_addoption(parser):
    parser.addoption("--with_lib_mode", action="store_true", help="Run the test with Library Mode")


collect_ignore = []
if "--with_lib_mode" in sys.argv:

    # ==============================================================================
    # /!\ Tests are started in Library mode: unselect (ignore) unrelated modules /!\
    # ==============================================================================

    warnings.warn("Main app is started in library mode")

    # Check dependencies directory exists
    parent: Path = Path(__file__).parent.parent
    test_dir: Path = Path(parent, ".test_dependencies")
    assert test_dir.exists()

    configuration.OPTIONAL.MAIN_APP_DIR = "tests/.test_dependencies/"

    collect_ignore.append("test_blind_sign.py")
    collect_ignore.append("test_configuration_cmd.py")
    collect_ignore.append("test_domain_name.py")
    collect_ignore.append("test_eip1559.py")
    collect_ignore.append("test_eip191.py")
    collect_ignore.append("test_eip2930.py")
    collect_ignore.append("test_eip712.py")
    collect_ignore.append("test_erc20information.py")
    collect_ignore.append("test_get_address.py")
    collect_ignore.append("test_nft.py")
    collect_ignore.append("test_sign.py")
else:

    # ===========================================================================
    # /!\ Standards tests without Library mode: unselect (ignore) clone tests /!\
    # ===========================================================================

    collect_ignore.append("test_clone.py")


#####################
# CONFIGURATION END #
#####################

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest", )
