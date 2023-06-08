#include "common_ui.h"
#include "shared_context.h"
#include "ui_nbgl.h"
#include "nbgl_use_case.h"
#include "glyphs.h"
#include "network.h"

char staxSharedBuffer[SHARED_BUFFER_SIZE] = {0};
nbgl_page_t *pageContext;

#define FORMAT_PLUGIN "This app enables clear\nsigning transactions for\nthe %s dApp."
#define MAX_APP_NAME_FOR_LOCAL_TAGLINE SHARED_BUFFER_SIZE - 1 - (sizeof(TAGLINE_PART1) + sizeof(TAGLINE_PART2))

void releaseContext(void) {
    if (pageContext != NULL) {
        nbgl_pageRelease(pageContext);
        pageContext = NULL;
    }
}

void app_quit(void) {
    // exit app here
    os_sched_exit(-1);
}

const nbgl_icon_details_t *get_app_icon(bool caller_icon) {
    const nbgl_icon_details_t *icon = NULL;

    if (caller_icon && caller_app) {
        if (caller_app->icon) {
            icon = caller_app->icon;
        }
    } else {
        icon = &ICONGLYPH;
    }
    return icon;
}

void ui_idle(void) {
    const char *app_name = NULL;
    const char *tagline = NULL;

    if (caller_app) {
        app_name = caller_app->name;

        if (caller_app->type == CALLER_TYPE_PLUGIN) {
            snprintf(staxSharedBuffer, sizeof(staxSharedBuffer), FORMAT_PLUGIN, app_name);
            tagline = staxSharedBuffer;
        }
        // If app_name is too long to fit the sdk buffer we must declare it locally in the app.
        // If it is too long to fit in the shared buffer, the SDK will use a default value.
        else if (strlen(app_name) > MAX_APP_NAME_FOR_SDK_TAGLINE && strlen(app_name) < MAX_APP_NAME_FOR_LOCAL_TAGLINE) {
            snprintf(staxSharedBuffer, sizeof(staxSharedBuffer), "%s\n%s %s", TAGLINE_PART1, app_name, TAGLINE_PART2);
            tagline = staxSharedBuffer;
        }
    } else {  // Ethereum app
        app_name = get_app_network_name();
    }

    nbgl_useCaseHome((char *) app_name,
                     get_app_icon(true),
                     tagline,
                     true,
                     ui_menu_settings,
                     app_quit);
}
