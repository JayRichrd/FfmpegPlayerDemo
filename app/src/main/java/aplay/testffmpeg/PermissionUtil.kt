package aplay.testffmpeg

import android.app.Activity
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.util.Log
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import permissions.dispatcher.ktx.constructPermissionsRequest

/**
 *  author : cainjiang
 *  date : 2022/3/28
 *  description : 动态权限工具
 */
object PermissionUtil {
    const val TAG = "PermissionUtil"

    // Map of dangerous permissions introduced in later framework versions.
    // Used to conditionally bypass permission-hold checks on older devices.
    // ref: https://developer.android.com/reference/android/Manifest.permission
    private val MIN_SDK_PERMISSIONS: MutableMap<String, Int> by lazy {
        mutableMapOf<String, Int>().apply {
            this["com.android.voicemail.permission.ADD_VOICEMAIL"] = 14
            this["android.permission.READ_CALL_LOG"] = 16
            this["android.permission.READ_EXTERNAL_STORAGE"] = 16
            this["android.permission.WRITE_CALL_LOG"] = 16
            this["android.permission.BODY_SENSORS"] = 20
            this["android.permission.SYSTEM_ALERT_WINDOW"] = 23
            this["android.permission.WRITE_SETTINGS"] = 23
            this["android.permission.READ_PHONE_NUMBERS"] = 26
            this["android.permission.ANSWER_PHONE_CALLS"] = 26
            this["android.permission.ACCEPT_HANDOVER"] = 28
            this["android.permission.ACTIVITY_RECOGNITION"] = 29
            this["android.permission.ACCESS_MEDIA_LOCATION"] = 29
            this["android.permission.ACCESS_BACKGROUND_LOCATION"] = 29
        }
    }

    /**
     * 申请权限
     * @param permissions 申请的权限
     * @param callback 权限申请，用户操作回调
     */
    fun requestPermissions(activity: Activity, permissions: Array<out String>, callback: RequestPermissionCallback) {
        if (activity !is FragmentActivity) {
            val errorMsg = "activity can't cast to FragmentActivity!"
            callback.onError(permissions, Throwable(errorMsg))
            Log.e(TAG, "requestPermission# errorMsg: $errorMsg")
            return
        }
        Log.i(TAG, "requestPermissions# ready to request permissions: ${permissions.toMutableList()}")
        activity.constructPermissionsRequest(*permissions,
            onShowRationale = {
                callback.onNeedShowRationale(permissions)
            },
            onPermissionDenied = {
                callback.onPermissionDenied(permissions)
            },
            onNeverAskAgain = {
                callback.onNeverAskAgain(permissions)
            },
            requiresPermission = {
                callback.onPermissionGranted(permissions)
            }
        ).launch()
    }

    /**
     * Checks all given permissions have been granted.
     *
     * @param grantResults results
     * @return returns true if all permissions have been granted.
     */
    fun verifyPermissions(vararg grantResults: Int): Boolean {
        if (grantResults.isEmpty()) {
            return false
        }
        for (result in grantResults) {
            if (result != PackageManager.PERMISSION_GRANTED) {
                return false
            }
        }
        return true
    }

    /**
     * Returns true if the permission exists in this SDK version
     *
     * @param permission permission
     * @return returns true if the permission exists in this SDK version
     */
    private fun permissionExists(permission: String): Boolean {
        // Check if the permission could potentially be missing on this device
        val minVersion = MIN_SDK_PERMISSIONS[permission]
        // If null was returned from the above call, there is no need for a device API level check for the permission;
        // otherwise, we check if its minimum API level requirement is met
        return minVersion == null || Build.VERSION.SDK_INT >= minVersion
    }

    /**
     * Returns true if the Activity or Fragment has access to all given permissions.
     *
     * @param context     context
     * @param permissions permission list
     * @return returns true if the Activity or Fragment has access to all given permissions.
     */
    fun hasSelfPermissions(context: Context, vararg permissions: String): Boolean {
        for (permission in permissions) {
            if (permissionExists(permission) && !hasSelfPermission(context, permission)) {
                return false
            }
        }
        return true
    }

    /**
     * Determine context has access to the given permission.
     *
     *
     * This is a workaround for RuntimeException of Parcel#readException.
     * For more detail, check this issue https://github.com/hotchemi/PermissionsDispatcher/issues/107
     *
     * @param context    context
     * @param permission permission
     * @return true if context has access to the given permission, false otherwise.
     * @see .hasSelfPermissions
     */
    private fun hasSelfPermission(context: Context, permission: String): Boolean {
        return try {
            ContextCompat.checkSelfPermission(context, permission) == PackageManager.PERMISSION_GRANTED
        } catch (t: RuntimeException) {
            false
        }
    }

    /**
     * Checks given permissions are needed to show rationale.
     *
     * @param activity    activity
     * @param permissions permission list
     * @return returns true if one of the permission is needed to show rationale.
     */
    fun shouldShowRequestPermissionRationale(activity: Activity?, vararg permissions: String?): Boolean {
        for (permission in permissions) {
            if (ActivityCompat.shouldShowRequestPermissionRationale(activity!!, permission!!)) {
                return true
            }
        }
        return false
    }

    /**
     * Checks given permissions are needed to show rationale.
     *
     * @param fragment    fragment
     * @param permissions permission list
     * @return returns true if one of the permission is needed to show rationale.
     */
    fun shouldShowRequestPermissionRationale(fragment: Fragment, vararg permissions: String?): Boolean {
        for (permission in permissions) {
            if (fragment.shouldShowRequestPermissionRationale(permission!!)) {
                return true
            }
        }
        return false
    }


    /**
     * 权限申请回调
     */
    interface RequestPermissionCallback {
        /**
         * 权限被拒绝
         * @param permissions 被拒绝的权限
         */
        fun onPermissionDenied(permissions: Array<out String>)

        /**
         * 同意授权
         * @param permissions 同意授予的权限
         */
        fun onPermissionGranted(permissions: Array<out String>)

        /**
         * 用户选择不在询问回调
         * @param permissions 申请的权限
         */
        fun onNeverAskAgain(permissions: Array<out String>)

        /**
         * 权限申请需要展示必要性回调
         * @param permissions 申请的权限
         */
        fun onNeedShowRationale(permissions: Array<out String>) {}

        /**
         * 申请权限失败报错
         * @param permissions 申请的权限
         * @param throwable 返回失败的错误
         */
        fun onError(permissions: Array<out String>, throwable: Throwable)
    }
}