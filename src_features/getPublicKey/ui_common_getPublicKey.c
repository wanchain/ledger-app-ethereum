#include "shared_context.h"
#include "feature_getPublicKey.h"
#include "common_ui.h"
#include "apdu_constants.h"

unsigned int io_seproxyhal_touch_address_ok(__attribute__((unused)) const bagl_element_t *e) {
    uint32_t tx = set_result_get_publicKey();
    reset_app_context();
    send_apdu_response(true, tx);
    // Display back the original UX
    ui_idle();
    return 0;  // do not redraw the widget
}

unsigned int io_seproxyhal_touch_address_cancel(__attribute__((unused)) const bagl_element_t *e) {
    reset_app_context();
    send_apdu_response_explicit(APDU_SW_CONDITION_NOT_SATISFIED, 0);
    // Display back the original UX
    ui_idle();
    return 0;  // do not redraw the widget
}
