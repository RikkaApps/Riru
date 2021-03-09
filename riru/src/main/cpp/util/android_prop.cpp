#include <sys/system_properties.h>

namespace AndroidProp {

    const char* GetRelease() {
        static char release[PROP_VALUE_MAX + 1]{0};
        if (release[0] != 0) return release;
        __system_property_get("ro.build.version.release", release);
        return release;
    }

    int GetApiLevel() {
        static int apiLevel = 0;
        if (apiLevel > 0) return apiLevel;

        char buf[PROP_VALUE_MAX + 1];
        if (__system_property_get("ro.build.version.sdk", buf) > 0)
            apiLevel = atoi(buf);

        return apiLevel;
    }

    int GetPreviewApiLevel() {
        static int previewApiLevel = 0;
        if (previewApiLevel > 0) return previewApiLevel;

        char buf[PROP_VALUE_MAX + 1];
        if (__system_property_get("ro.build.version.preview_sdk", buf) > 0)
            previewApiLevel = atoi(buf);

        return previewApiLevel;
    }
}