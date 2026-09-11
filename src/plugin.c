// majestic-af — out-of-core autofocus plugin for majestic (OpenIPC).
//
// The ABI boundary (include/majestic/af_plugin_abi.h): majestic dlsym's
// af_plugin_call / af_plugin_exit here and calls them from its /autofocus and
// /zoom handlers; this plugin resolves the core HAL seams (sdk_get_focus_value,
// sdk_set_zoom_mag, config_get_*) against the executable at dlopen.
//
// The AF engine and search live in engine.c and the af*.c search modules. Motor
// access lives behind the interface in af_motor.c. This file is only the thin
// adapter from the two-token command ABI to the AF engine.

#include <majestic/af.h>
#include <majestic/af_plugin_abi.h>

#include <string.h>

// engine.c: synchronous teardown — joins the worker + reader before we return.
void af_engine_stop(void);

static const char *map_trigger(int r) {
    // af_trigger: 0 started, 2 preempted-and-rearmed, else busy (-1 shouldn't
    // happen — the plugin only loaded because autofocus is enabled).
    return r == 0 ? "started" : r == 2 ? "restarted" : "busy";
}

const char *af_plugin_call(const char *cmd, const char *val) {
    if (!cmd || !val) {
        return NULL;
    }

    if (!strcmp(cmd, "autofocus")) {
        if (!strcmp(val, "status")) {
            return af_status();               // "idle" | "running" | "done fv=… …"
        }
        if (!strcmp(val, "run")) {
            return map_trigger(af_trigger(false));
        }
        if (!strcmp(val, "settle")) {
            return map_trigger(af_trigger(true));
        }
        if (!strcmp(val, "cancel")) {
            int r = af_cancel_pass();
            return r == 0 ? "cancelled" : r == 1 ? "idle" : "unavailable";
        }
        return NULL;
    }

    if (!strcmp(cmd, "zoom")) {
        if (!strcmp(val, "tele")) {
            return af_zoom_pulse(1) == 0 ? "zooming" : "unavailable";
        }
        if (!strcmp(val, "wide")) {
            return af_zoom_pulse(-1) == 0 ? "zooming" : "unavailable";
        }
        if (!strcmp(val, "stop")) {
            return "stopped";                 // pulses self-terminate; nothing to halt
        }
        return NULL;
    }

    return NULL;                              // unknown command -> core falls back
}

void af_plugin_exit(void) {
    af_engine_stop();
}

// Start the magnification reader the moment the .so is loaded (the core dlopen's
// it in af_plugin_init, after config is loaded), so the OSD %@ token and /zoom
// (GET) have a value without waiting for the first AF. Torn down in
// af_plugin_exit / af_engine_stop before dlclose.
__attribute__((constructor)) static void af_plugin_load(void) {
    af_zoom_start();
}
