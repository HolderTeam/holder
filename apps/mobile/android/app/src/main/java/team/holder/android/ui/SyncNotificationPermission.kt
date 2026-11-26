package team.holder.android.ui

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.runtime.Composable
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.platform.LocalContext
import androidx.core.content.ContextCompat
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch
import team.holder.android.HolderSettings

/** Holder's only notifications say that background sync or backup keeps failing (see
 * team.holder.android.diagnostics.ReliabilityNotifier). So the permission is asked for when
 * background sync starts to matter: when the user turns it on, or when GitHub gets connected
 * while it's on, which with sync on by default is the first moment it can run. The result is
 * unused: if the user says no, ReliabilityNotifier skips posting and nothing asks again on its
 * own; Android also stops showing the prompt after it has been declined twice. */
@Composable
fun rememberSyncNotificationPermissionRequest(): () -> Unit {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val launcher = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) {}
    return {
        if (needsNotificationPermission(context)) {
            scope.launch {
                if (HolderSettings.gitBackgroundSyncEnabled(context).first()) {
                    launcher.launch(Manifest.permission.POST_NOTIFICATIONS)
                }
            }
        }
    }
}

fun needsNotificationPermission(context: Context): Boolean =
    Build.VERSION.SDK_INT >= 33 &&
        ContextCompat.checkSelfPermission(context, Manifest.permission.POST_NOTIFICATIONS) !=
        PackageManager.PERMISSION_GRANTED
