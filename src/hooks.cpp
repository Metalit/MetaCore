#include "hooks.hpp"

#include "main.hpp"
#include "types.hpp"
#include "beatsaber-hook/shared/binary.hpp"

using namespace MetaCore;

static void raise_exc_lol() {
    auto parse = i2c::find_method({"System", "Int32"}, {"Parse", {}, {i2c::type_of<StringW>()}});
    auto str = StringW("notanint");
    auto cvt = str.convert();
    Il2CppException* exc;
    i2c::functions::runtime_invoke(parse, nullptr, &cvt, &exc);
    if (exc) {
        logger.debug("raise");
        i2c::functions::raise_exception(exc);
    } else {
        logger.debug("no exc??");
    }
}

// hook abort and provide backtraces
MAKE_HOOK(Abort, (nullptr), void) {
    auto logger = Paper::ConstLoggerContext("abort_hook");
    logger.info("abort called");
    logger.Backtrace(40);
    // raise_exc_lol();

    Abort();
}

AUTO_HOOK_FUNCTION(Abort) {
    auto libc = dlopen("libc.so", RTLD_NOW);
    auto abort_address = dlsym(libc, "abort");
    INSTALL_HOOK(logger, Abort, abort_address);
}

// hook unity object destruction (OnDestroy and dtors need to be debugged in CT)
MAKE_HOOK(delete_object_internal_step1, (nullptr), void, char* object) {
    int instanceId = *(int*) (object + 8);
    auto destroy = ObjectSignal::onDestroys.find(instanceId);
    if (destroy != ObjectSignal::onDestroys.end() && destroy->second) {
        destroy->second();
        ObjectSignal::onDestroys.erase(destroy);
    }
    delete_object_internal_step1(object);
}

AUTO_HOOK_FUNCTION(delete_object_internal_step1) {
    // TODO: fix addr (or just remove hook)
    uintptr_t addr = i2c::binary::base_addr("libunity.so") + 0x8d2898;
    INSTALL_HOOK(logger, delete_object_internal_step1, (void*) addr);
}
