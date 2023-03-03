#include "shared_context.h"
#include "feature_getPublicKey.h"
#include "common_ui.h"
#include "feature_performPrivacyOperation.h"
#include "apdu_constants.h"

unsigned int io_seproxyhal_touch_privacy_ok(__attribute__((unused)) const bagl_element_t *e) {
    uint32_t tx = set_result_perform_privacy_operation();
    reset_app_context();
    send_apdu_response(true, tx);
    // Display back the original UX
    ui_idle();
    return 0;  // do not redraw the widget
}

unsigned int io_seproxyhal_touch_privacy_cancel(__attribute__((unused)) const bagl_element_t *e) {
    reset_app_context();
    // Send back the response, do not restart the event loop
    send_apdu_response_explicit(APDU_SW_CONDITION_NOT_SATISFIED, 0);
    // Display back the original UX
    ui_idle();
    return 0;  // do not redraw the widget
}
