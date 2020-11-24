#include <sys/stat.h>
#include <fcntl.h>
#include <climits>
#include <cstdio>
#include <sys/system_properties.h>
#include <unistd.h>
#include <cstdarg>
#include "status.h"
#include "logging.h"
#include "misc.h"
#include "module.h"
#include "config.h"
#include "daemon/status_writter.h"

#define TMP_DIR "/dev"

void Status::Write() {
#ifdef __LP64__
    bool is64bit = true;
#else
    bool is64bit = false;
#endif

    flatbuffers::FlatBufferBuilder builder;

    auto core = CreateCoreDirect(
            builder,
            RIRU_API_VERSION,
            RIRU_VERSION_CODE,
            RIRU_VERSION_NAME,
            is_hide_enabled());

    std::vector<flatbuffers::Offset<Module>> modules_vector;
    for (auto module : *get_modules()) {
        if (strcmp(module->name, MODULE_NAME_CORE) == 0) continue;
        modules_vector.emplace_back(CreateModuleDirect(
                builder,
                module->name,
                module->apiVersion,
                module->version,
                module->versionName,
                module->supportHide));
    }

    auto modules = builder.CreateVector(modules_vector);

    FbStatusBuilder status_builder(builder);
    status_builder.add_is_64bit(is64bit);
    status_builder.add_core(core);
    status_builder.add_modules(modules);
    FinishFbStatusBuffer(builder, status_builder.Finish());

    WriteToFile(GetFbStatus(builder.GetBufferPointer()));
}

void Status::WriteMethod(Method method, bool replaced, const char *sig) {
#ifdef __LP64__
    bool is64bit = true;
#else
    bool is64bit = false;
#endif

    static const char *method_name[Method::COUNT] = {
            "nativeForkAndSpecialize",
            "nativeForkSystemServer",
            "nativeSpecializeAppProcess"
    };

    flatbuffers::FlatBufferBuilder builder;

    flatbuffers::Offset<JNIMethod> method_data[] = {CreateJNIMethodDirect(builder, method_name[method], sig, replaced)};
    auto methods = builder.CreateVector(method_data, 1);

    FbStatusBuilder status_builder(builder);
    status_builder.add_is_64bit(is64bit);
    status_builder.add_jni_methods(methods);
    FinishFbStatusBuffer(builder, status_builder.Finish());

    WriteToFile(GetFbStatus(builder.GetBufferPointer()));
}

