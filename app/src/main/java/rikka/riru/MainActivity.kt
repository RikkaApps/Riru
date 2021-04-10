package rikka.riru

import android.annotation.SuppressLint
import android.app.Activity
import android.content.ComponentName
import android.content.Intent
import android.content.ServiceConnection
import android.os.Bundle
import android.os.IBinder
import com.topjohnwu.superuser.ipc.RootService
import rikka.riru.databinding.MainActivityBinding

@SuppressLint("SetTextI18n")
class MainActivity : Activity() {

    private lateinit var binding: MainActivityBinding

    private val connection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            IService.Stub.asInterface(service)?.let {
                val string = it.read()
                binding.text1.post {
                    binding.text1.text = string
                }
            }

            RootService.unbind(this)
        }

        override fun onServiceDisconnected(name: ComponentName) {
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = MainActivityBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.text1.text = "Requesting root..."
        val intent = Intent(this, Service::class.java)
        RootService.bind(intent, connection)
    }
}
