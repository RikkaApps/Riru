package rikka.riru

import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.os.SELinux
import android.os.SystemProperties
import android.util.Log
import com.topjohnwu.superuser.Shell
import com.topjohnwu.superuser.ipc.RootService
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.FileInputStream
import java.io.IOException

class Service : RootService() {

    internal class Impl : IService.Stub() {

        override fun read(): String {
            val sb = StringBuilder()
            try {
                read(sb)
            } catch (e: Throwable) {
                sb.append("\n\n${Log.getStackTraceString(e)}")
            }
            return sb.toString()
        }

        private fun read(message: StringBuilder) {
            if (!Shell.rootAccess()) {
                message.append("The device is not rooted, or permission denied.")
                return
            }

            // --------------------------------------------

            val devRandomFile = File("/data/adb/riru/dev_random")
            val devRandom = devRandomFile.readTextOrNull()
            if (devRandom == null) {
                message.append("Riru not installed (or version < v22).\n\n")
                return
            }

            // --------------------------------------------

            if (SELinux.isSELinuxEnabled() && SELinux.isSELinuxEnforced()) {
                (SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "file", "relabelfrom") ||
                        SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "dir", "relabelfrom")).let {
                    if (it) {
                        message.append("Your ROM has incorrect SELinux which will fially make Riru not working.\n\n" +
                                "Visit https://github.com/RikkaApps/Riru/wiki/Explain-about-incorrect-SELinux-rules-from-third-party-ROMs-cause-Riru-not-working for details.")
                        return
                    } else {
                        message.append("SELinux is enforcing and there are no incorrect rules related to Riru.\n\n")
                    }
                }
            } else {
                message.append("SELinux is not enabled/enforced, the device may be in danger.\n\n")
            }

            // --------------------------------------------

            val has64Bit = Build.SUPPORTED_64_BIT_ABIS.isNotEmpty()
            val root = if (has64Bit) "/dev/riru64_$devRandom" else "/dev/riru_$devRandom"

            val devRootFile = File(root)
            if (!devRootFile.exists()) {
                message.append("Riru not installed or not enabled.")

                if (SystemProperties.get("ro.dalvik.vm.native.bridge") != "libriruloader.so") {
                    message.appendLine("\n\nProperty \"ro.dalvik.vm.native.bridge\" is not \"libriruloader.so\".")
                    message.appendLine("\nMake sure you are not using other module which changes this property.")
                    message.appendLine("\nA typical example is, some \"optimize\" modules changes this property. Since changing this property is meaningless for \"optimization\", their quality is very questionable. In fact, changing properties for optimization is a joke.")
                }
                return
            }
            val apiFile = File("$root/api")
            val versionFile = File("$root/version")
            val versionNameFile = File("$root/version_name")
            val api = apiFile.readTextOrNull()
            val version = versionFile.readTextOrNull()
            val versionName = versionNameFile.readTextOrNull()

            message.appendLine("Riru $versionName ($version, API v$api) is in use.")

            // --------------------------------------------

            message.appendLine("\nLoaded modules:")

            val moduleFiles = File("$root/modules").listFiles()
            if (moduleFiles?.isNotEmpty() == true) {
                moduleFiles.forEach { module ->
                    message.append(module.name).append(": ")

                    File(module, "version_name").let {
                        val text = it.readTextOrNull()
                        if (text != null) {
                            message.append(text)
                        }
                    }

                    File(module, "version").let {
                        val text = it.readTextOrNull()
                        if (text != null) {
                            message.append(" (").append(text).append(")")
                        }
                    }

                    File(module, "api").let {
                        val text = it.readTextOrNull()
                        if (text != null) {
                            message.append(", api v").append(text)
                        }
                    }

                    File(module, "hide").let {
                        val text = it.readTextOrNull()
                        if (text != null) {
                            message.appendLine(if (text == "true") ", support hide" else ", no hide support")
                        } else {
                            message.appendLine()
                        }
                    }
                }
            } else {
                message.appendLine("(none)")
            }
        }
    }

    override fun onBind(intent: Intent): IBinder {
        return Impl()
    }
}

private fun File.readTextOrNull(): String? {
    if (!exists()) return null
    try {
        FileInputStream(this).use { `in` ->
            val os = ByteArrayOutputStream()
            val buffer = ByteArray(1024)
            var length: Int
            while (`in`.read(buffer).also { length = it } != -1) {
                os.write(buffer, 0, length)
            }
            return os.toString()
        }
    } catch (e: IOException) {
        e.printStackTrace()
        return null
    }
}
