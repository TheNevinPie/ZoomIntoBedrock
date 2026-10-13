#include "mod/ZoomMod.h"
#include "mod/ZoomState.h"

#include "ll/api/input/KeyHandle.h"
#include "ll/api/input/KeyRegistry.h"
#include "ll/api/mod/RegisterHelper.h"
#include "mc/client/game/IClientInstance.h"
#include "mc/deps/input/Keyboard.h"
#include "mc/deps/input/enums/FocusImpact.h"

namespace zoom {

ZoomMod& ZoomMod::getInstance() {
    static ZoomMod instance;
    return instance;
}

bool ZoomMod::load() {
    // Zoom key: hold C to zoom (remappable in Settings > Keyboard).
    // ::Keyboard::C (67) is the default taken verbatim from the
    // LeviLamina 26.51 headers (mc/deps/input/Keyboard.h).
    auto& key = ll::input::KeyRegistry::getInstance().getOrCreateKey("zoom", {::Keyboard::C}, true);
    key.registerButtonDownHandler([](::FocusImpact, ::IClientInstance&) {
        ZoomState::getInstance().setZoomed(true);
    });
    key.registerButtonUpHandler([](::FocusImpact, ::IClientInstance&) {
        ZoomState::getInstance().setZoomed(false);
    });
    getSelf().getLogger().info("ZoomIntoBedrock loaded: hold C to zoom, scroll to zoom further.");
    return true;
}

bool ZoomMod::enable() {
    getSelf().getLogger().debug("Enabling ZoomIntoBedrock...");
    return true;
}

bool ZoomMod::disable() {
    getSelf().getLogger().debug("Disabling ZoomIntoBedrock...");
    ZoomState::getInstance().setZoomed(false);
    return true;
}

} // namespace zoom

LL_REGISTER_MOD(zoom::ZoomMod, zoom::ZoomMod::getInstance());
