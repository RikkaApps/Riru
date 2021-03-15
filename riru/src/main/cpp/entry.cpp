#include <dlfcn.h>
#include <android_prop.h>
#include <pthread.h>
#include "misc.h"
#include "jni_hooks.h"
#include "logging.h"
#include "module.h"
#include "hide_utils.h"
#include "status.h"
#include "magisk.h"
#include "entry.h"

static void *self_handle;

struct SelfUnloadGuard {

    SelfUnloadGuard() {
        pthread_mutex_init(&mutex_, nullptr);
    }

    ~SelfUnloadGuard() {
        LOGD("self unload lock (destructor)");
        pthread_mutex_lock(&mutex_);

        LOGD("self unload");

        timespec ts = {.tv_sec = 0, .tv_nsec = 1000000L};
        nanosleep(&ts, nullptr);
    }

    struct Holder {
        explicit Holder(pthread_mutex_t *mutex) : mutex_(mutex) {
            LOGD("self unload lock (holder constructor)");
            pthread_mutex_lock(mutex_);
        }

        Holder(Holder &&other) noexcept: mutex_(other.mutex_) {
            other.mutex_ = nullptr;
        }

        ~Holder() {
            if (mutex_) {
                pthread_mutex_unlock(mutex_);
                LOGD("self unload unlock (holder destructor)");
            }
        }

    private:
        pthread_mutex_t *mutex_;

    public:
        Holder(const Holder &) = delete;

        void operator=(const Holder &) = delete;
    };

    auto hold() { return Holder(&mutex_); };

private:
    pthread_mutex_t mutex_{};
} self_unload_guard;

static void SelfUnload() {
    LOGD("attempt to self unload");

    [[maybe_unused]] auto holder = self_unload_guard.hold();

    pthread_t thread;
    pthread_create(&thread, nullptr, (void *(*)(void *)) &dlclose, self_handle);
    pthread_detach(thread);
}

void Entry::Unload(jboolean hide_maps) {
    Hide::DoHide(false, hide_maps);

    bool selfUnload = true;
    for (auto module : Modules::Get()) {
        if (strcmp(module->id, MODULE_NAME_CORE) == 0) {
            continue;
        }

        if (module->allowUnload() != 0) {
            LOGD("unload %s", module->id);
            dlclose(module->handle);
        } else {
            if (module->apiVersion >= 25)
                LOGD("unload is not allow by module %s", module->id);
            else {
                LOGD("unload is not supported by module %s (API < 25), self unload is also disabled", module->id);
                selfUnload = false;
            }
        }
    }

    if (selfUnload) {
        SelfUnload();
    }
}

extern "C" __attribute__((visibility("default"))) __attribute__((used)) void init(void *handle) {
    self_handle = handle;

    LOGI("Magisk tmpfs path is %s", Magisk::GetPath());
    LOGI("Android %s (api %d, preview_api %d)", AndroidProp::GetRelease(), AndroidProp::GetApiLevel(),
         AndroidProp::GetPreviewApiLevel());

    JNI::InstallHooks();

    Modules::Load();

    Status::WriteSelfAndModules();
}