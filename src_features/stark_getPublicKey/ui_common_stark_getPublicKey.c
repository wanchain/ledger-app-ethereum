#ifdef HAVE_STARKWARE

#include "shared_context.h"
#include "common_ui.h"
#include "feature_stark_getPublicKey.h"
#include "apdu_constants.h"

unsigned int io_seproxyhal_touch_stark_pubkey_ok(__attribute__((unused)) const bagl_element_t *e) {
    uint32_t tx = set_result_get_stark_publicKey();
    reset_app_context();
    send_apdu_response(true, tx);
    // Display back the original UX
    ui_idle();
    return 0;  // do not redraw the widget
}

#endif
