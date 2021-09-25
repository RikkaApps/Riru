#include <sys/system_properties.h>

namespace android_prop {

    const char *GetRelease() {
        static char release[PROP_VALUE_MAX + 1]{0};
        if (release[0] != 0) return release;
        __system_property_get("ro.build.version.release", release);
        return release;
    }

    auto GetApiLevel() {
        static auto kApiLevel = (({
            int result = 0;
            if (char buf[PROP_VALUE_MAX + 1];
                    __system_property_get("ro.build.version.sdk", buf) > 0) {
                result = atoi(buf);
            }
            result;
        }));
        return kApiLevel;
    }

    auto GetPreviewApiLevel() {
        static auto kPreviewApiLevel = (({
            int result = 0;
            if (char buf[PROP_VALUE_MAX + 1];
                    __system_property_get("ro.build.version.preview_sdk", buf) > 0) {
                result = atoi(buf);
            }
            result;
        }));
        return kPreviewApiLevel;
    }

    auto CheckZTE() {
        static bool kZTE = __system_property_find("ro.vendor.product.ztename");
        return kZTE;
    }
}  // namespace android_prop
