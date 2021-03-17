package rikka.riru

import android.annotation.SuppressLint
import android.app.Activity
import android.os.Build
import android.os.Bundle
import android.os.SystemProperties
import android.util.Log
import com.topjohnwu.superuser.Shell
import com.topjohnwu.superuser.io.SuFile
import com.topjohnwu.superuser.io.SuFileInputStream
import kotlinx.coroutines.*
import moe.riru.manager.databinding.MainActivityBinding
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.IOException

@SuppressLint("SetTextI18n")
class MainActivity : Activity() {

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

        val has64Bit = Build.SUPPORTED_64_BIT_ABIS.isNotEmpty()
        val root = if (has64Bit) "/dev/riru64_$devRandom" else "/dev/riru_$devRandom"

        val devRootFile = SuFile.open(root)
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
        val apiFile = SuFile.open("$root/api")
        val hideFile = SuFile.open("$root/hide")
        val versionFile = SuFile.open("$root/version")
        val versionNameFile = SuFile.open("$root/version_name")
        val api = apiFile.readTextOrNull()
        val version = versionFile.readTextOrNull()
        val versionName = versionNameFile.readTextOrNull()

        message.appendLine("Riru $versionName ($version, API v$api) is in use.")

        detail.appendLine("$apiFile: $api")
        detail.appendLine("$versionFile: $version")
        detail.appendLine("$versionNameFile: $versionName")

        // --------------------------------------------

        message.appendLine("\nLoaded modules:")

        val moduleFiles = SuFile.open("$root/modules").listFiles()
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