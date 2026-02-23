#ifndef RESTUNE_VISIBILITY_H
#define RESTUNE_VISIBILITY_H

// If we are building tests, we force these symbols to be visible (default)
#if defined(ENABLE_TEST_VISIBILITY)
    #define RESTUNE_INTERNAL_EXPORT __attribute__((visibility("default")))

// If we are building for production (and not testing), we hide them
#else
    #define RESTUNE_INTERNAL_EXPORT __attribute__((visibility("hidden")))
#endif

#endif // RESTUNE_VISIBILITY_H
