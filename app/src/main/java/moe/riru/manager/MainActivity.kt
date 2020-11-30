package moe.riru.manager

import android.annotation.SuppressLint
import android.os.Bundle
import android.os.SystemProperties
import android.util.Log
import com.topjohnwu.superuser.Shell
import com.topjohnwu.superuser.io.SuFile
import com.topjohnwu.superuser.io.SuFileInputStream
import kotlinx.coroutines.*
import moe.riru.manager.app.AppActivity
import moe.riru.manager.databinding.MainActivityBinding
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.IOException

@SuppressLint("SetTextI18n")
class MainActivity : AppActivity() {

    private lateinit var binding: MainActivityBinding
    private var job: Job? = null

    private fun read(message: StringBuilder, detail: StringBuilder) {
        if (!Shell.rootAccess()) {
            message.append("The device is not rooted, or permission denied.")
            return
        }

        // --------------------------------------------

        val devRandomFile = SuFile.open("/data/adb/riru/dev_random")
        val devRandom = devRandomFile.readTextOrNull()
        if (devRandom == null) {
            message.append("Riru not installed (or version < v22).\n\n")
            detail.append("$devRandomFile not exist")
            return
        }

        // --------------------------------------------

        detail.appendLine("$devRandomFile: $devRandom")

        val devRootFile = SuFile.open("/dev/riru_$devRandom")
        if (!devRootFile.exists()) {
            message.append("Riru not installed or not enabled.")
            detail.append("$devRootFile not exist")

            if (SystemProperties.get("ro.dalvik.vm.native.bridge") != "libriruloader.so") {
                message.appendLine("\n\nProperty \"ro.dalvik.vm.native.bridge\" is not \"libriruloader.so\".")
                message.appendLine("\nMake sure you are not using other module which changes this property.")
                message.appendLine("\nA typical example is, some \"optimize\" modules changes this property. Since changing this property is meaningless for \"optimization\", their quality is very questionable. In fact, changing properties for optimization is a joke.")
                detail.append("\nro.dalvik.vm.native.bridge=${SystemProperties.get("ro.dalvik.vm.native.bridge")}")
            }
            return
        }
        val apiFile = SuFile.open("/dev/riru_$devRandom/api")
        val hideFile = SuFile.open("/dev/riru_$devRandom/hide")
        val versionFile = SuFile.open("/dev/riru_$devRandom/version")
        val versionNameFile = SuFile.open("/dev/riru_$devRandom/version_name")
        val api = apiFile.readTextOrNull()
        val hide = hideFile.readTextOrNull()
        val version = versionFile.readTextOrNull()
        val versionName = versionNameFile.readTextOrNull()

        message.appendLine("Riru $versionName ($version, API v$api) is in use.")
        message.appendLine("Hide is ${if (hide == "true") "enabled" else "disabled"}.")

        detail.appendLine("$apiFile: $api")
        detail.appendLine("$hideFile: $hide")
        detail.appendLine("$versionFile: $version")
        detail.appendLine("$versionNameFile: $versionName")

        // --------------------------------------------

        message.appendLine("\nNative methods:")

        Riru.methodNames.forEach { methodName ->
            SuFile.open("/dev/riru_$devRandom/methods/$methodName").let {
                message.append(methodName).append(": ")

                val method = it.readTextOrNull()?.split('\n')
                if (method != null && method.size >= 2) {
                    message.appendLine(if (method[0] == "true") "ok" else "!!! not replaced")
                } else {
                    message.appendLine("(unknown)")
                }

                detail.appendLine("$methodName: ${method?.joinToString("\n")}")
            }
        }

        // --------------------------------------------

        message.appendLine("\nLoaded modules:")

        val moduleFiles = SuFile.open("/dev/riru_$devRandom/modules").listFiles()
        if (moduleFiles?.isNotEmpty() == true) {
            moduleFiles.forEach { module ->
                message.append(module.name).append(": ")

                SuFile.open(module, "version_name").let {
                    val text = it.readTextOrNull()
                    if (text != null) {
                        message.append(text)
                    }
                    detail.appendLine("$it: $text")
                }

                SuFile.open(module, "version").let {
                    val text = it.readTextOrNull()
                    if (text != null) {
                        message.append(" (").append(text).append(")")
                    }
                    detail.appendLine("$it: $text")
                }

                SuFile.open(module, "api").let {
                    val text = it.readTextOrNull()
                    if (text != null) {
                        message.append(", api v").append(text)
                    }
                    detail.appendLine("$it: $text")
                }

                SuFile.open(module, "hide").let {
                    val text = it.readTextOrNull()
                    if (text != null) {
                        message.appendLine(if (text == "true") ", support hide" else ", no hide support")
                    } else {
                        message.appendLine()
                    }
                    detail.appendLine("$it: $text")
                }
            }
        } else {
            message.appendLine("(none)")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = MainActivityBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.text1.text = "Requesting root..."

        val message = StringBuilder()
        val detail = StringBuilder()
        job = GlobalScope.launch(Dispatchers.Main.immediate) {
            withContext(Dispatchers.IO) {
                try {
                    read(message, detail)
                } catch (e: Throwable) {
                    detail.append("\n", Log.getStackTraceString(e))
                }
            }
            binding.text1.text = "$message\nFile contents:\n$detail"
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        job?.cancel()
    }

}

private fun File.readTextOrNull(): String? {
    if (!exists()) return null
    try {
        SuFileInputStream(this).use { `in` ->
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