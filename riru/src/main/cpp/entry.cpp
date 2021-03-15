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
static pthread_mutex_t self_close_mutext = PTHREAD_MUTEX_INITIALIZER;

static void SelfUnload() {
    LOGD("attempt to self unload");

    pthread_mutex_lock(&self_close_mutext);

    pthread_t thread;
    pthread_create(&thread, nullptr, (void *(*)(void *)) &dlclose, self_handle);
    pthread_detach(thread);

    pthread_mutex_unlock(&self_close_mutext);
}

void Entry::Unload(jboolean hide_maps) {
    Hide::DoHide(false, hide_maps);

    bool selfUnload = true;
    for (auto module : *get_modules()) {
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

extern "C" __attribute__((destructor)) void destructor() {
    pthread_mutex_lock(&self_close_mutext);

    LOGI("self unload successful");

    timespec ts = {.tv_sec = 0, .tv_nsec = 1000000L};
    nanosleep(&ts, nullptr);
}

extern "C" __attribute__((constructor)) void constructor() {
#ifdef DEBUG_APP
    hide::hide_modules(nullptr, 0);
#endif

    if (getuid() != 0)
        return;

    char cmdline[ARG_MAX + 1];
    get_self_cmdline(cmdline, 0);

    if (strcmp(cmdline, "zygote") != 0
        && strcmp(cmdline, "zygote32") != 0
        && strcmp(cmdline, "zygote64") != 0
        && strcmp(cmdline, "usap32") != 0
        && strcmp(cmdline, "usap64") != 0) {
        LOGW("not zygote (cmdline=%s)", cmdline);
        return;
    }

    LOGI("Riru %s (%d) in %s", RIRU_VERSION_NAME, RIRU_VERSION_CODE, cmdline);
    LOGI("Magisk tmpfs path is %s", Magisk::GetPath());
    LOGI("Android %s (api %d, preview_api %d)", AndroidProp::GetRelease(), AndroidProp::GetApiLevel(),
         AndroidProp::GetPreviewApiLevel());

    JNI::InstallHooks();

    Modules::Load();

    Status::WriteSelfAndModules();
}

extern "C" __attribute__((visibility("default"))) __attribute__((used)) void init(void *handle) {
    self_handle = handle;
}