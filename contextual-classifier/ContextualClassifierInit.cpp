// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <dlfcn.h>
#include <string>
#include <cstdarg>

#include "Logger.h"
#include "ComponentRegistry.h"

// Helper function from ContextualClassifier to format strings
static std::string format_string(const char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return std::string(buffer);
}

#define CLASSIFIER_TAG "CC-Init"

typedef ErrCode (*cc_init_fn)(void);
typedef ErrCode (*cc_term_fn)(void);

static void *g_cc_handle = nullptr;
static cc_init_fn g_cc_init = nullptr;
static cc_term_fn g_cc_term = nullptr;

static ErrCode init(void *arg = nullptr) {
    (void)arg;

    if (g_cc_handle) {
        // Already loaded; just call init again.
        return g_cc_init ? g_cc_init() : RC_SUCCESS;
    }

    // This should match the installed path of libContextualClassifier.so
    const char *so_name = "/usr/lib/libContextualClassifier.so.1";
    g_cc_handle = dlopen(so_name, RTLD_NOW);
    if (!g_cc_handle) {
        LOGE(CLASSIFIER_TAG,
             format_string("Failed to dlopen %s: %s", so_name, dlerror()));
        // Do not fail the entire URM; just disable classifier functionality.
        return RC_SUCCESS;
    }

    dlerror();

    g_cc_init = reinterpret_cast<cc_init_fn>(dlsym(g_cc_handle, "cc_init"));
    const char *err = dlerror();
    if (err != nullptr || !g_cc_init) {
        LOGE(CLASSIFIER_TAG,
             format_string("Failed to resolve cc_init in %s: %s", so_name,
                           err ? err : "unknown"));
        dlclose(g_cc_handle);
        g_cc_handle = nullptr;
        g_cc_init = nullptr;
        g_cc_term = nullptr;
        return RC_SUCCESS;
    }

    g_cc_term = reinterpret_cast<cc_term_fn>(dlsym(g_cc_handle, "cc_terminate"));
    err = dlerror();
    if (err != nullptr || !g_cc_term) {
        LOGE(CLASSIFIER_TAG,
             format_string("Failed to resolve cc_terminate in %s: %s", so_name,
                           err ? err : "unknown"));
        dlclose(g_cc_handle);
        g_cc_handle = nullptr;
        g_cc_init = nullptr;
        g_cc_term = nullptr;
        return RC_SUCCESS;
    }

    return g_cc_init();
}

static ErrCode terminate(void *arg = nullptr) {
    (void)arg;

    if (g_cc_term) {
        g_cc_term();
    }

    if (g_cc_handle) {
        dlclose(g_cc_handle);
        g_cc_handle = nullptr;
    }

    g_cc_init = nullptr;
    g_cc_term = nullptr;

    return RC_SUCCESS;
}

URM_REGISTER_MODULE(MOD_CLASSIFIER, init, terminate);
