// `just check`: the update checker's tag compare. Header-only, no JUCE.
#include "ui/VersionCompare.h"

#include <cstdio>

static int failures = 0;
#define CHECK(x) do { if (! (x)) { std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #x); ++failures; } } while (0)

int main()
{
    using v4jb::versionNewer;
    using v4jb::newestNewerTag;

    CHECK (versionNewer ("0.0.2", "0.0.1"));
    CHECK (versionNewer ("v0.1.0", "0.0.9"));
    CHECK (versionNewer ("1.0", "0.9.9"));
    CHECK (versionNewer ("0.0.10", "0.0.9"));      // numeric, not string compare
    CHECK (! versionNewer ("0.0.1", "0.0.1"));
    CHECK (! versionNewer ("0.0.1.0", "0.0.1"));    // trailing zero is equal
    CHECK (! versionNewer ("0.0.1", "0.0.2"));
    CHECK (! versionNewer ("nightly", "0.0.1"));    // non-version tags are ignored
    CHECK (! versionNewer ("0.0.2-beta", "0.0.1"));
    CHECK (! versionNewer ("0.0.", "0.0.1"));

    CHECK (newestNewerTag ({ "v0.0.3", "0.0.2", "junk", "0.0.10" }, "0.0.1") == "0.0.10");
    CHECK (newestNewerTag ({ "0.0.1", "0.0.0" }, "0.0.1").empty());
    CHECK (newestNewerTag ({}, "0.0.1").empty());

    std::printf (failures == 0 ? "version_check OK\n" : "version_check: %d failed\n", failures);
    return failures == 0 ? 0 : 1;
}
