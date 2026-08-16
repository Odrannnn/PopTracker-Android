#include "android_bridge.h"

#include <SDL2/SDL_system.h>
#include <jni.h>
#include <mutex>
#include <utility>

namespace {

struct ActivityScope {
    JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    jclass type = env && activity ? env->GetObjectClass(activity) : nullptr;

    ~ActivityScope()
    {
        if (!env)
            return;
        if (type)
            env->DeleteLocalRef(type);
        if (activity)
            env->DeleteLocalRef(activity);
    }

    bool valid() const { return env && activity && type; }
};

jstring toJava(JNIEnv* env, const std::string& value)
{
    return env->NewStringUTF(value.c_str());
}

bool takeJavaException(JNIEnv* env)
{
    if (!env->ExceptionCheck())
        return false;
    env->ExceptionDescribe();
    env->ExceptionClear();
    return true;
}

std::mutex connectionRequestMutex;
bool connectionRequestPending = false;
std::string connectionRequestHost;
std::string connectionRequestSlot;
std::string connectionRequestPassword;
std::string connectionRequestGame;

std::string fromJava(JNIEnv* env, jstring value)
{
    if (!env || !value)
        return {};
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (!chars)
        return {};
    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

} // namespace

extern "C" JNIEXPORT void JNICALL
Java_io_github_poptracker_android_PopTrackerActivity_nativeSetLaunchRequest(
    JNIEnv* env,
    jclass,
    jstring host,
    jstring slot,
    jstring password,
    jstring game)
{
    std::string nativeHost = fromJava(env, host);
    std::string nativeGame = fromJava(env, game);
    if (nativeHost.empty() && nativeGame.empty())
        return;

    std::lock_guard<std::mutex> lock(connectionRequestMutex);
    connectionRequestHost = std::move(nativeHost);
    connectionRequestSlot = fromJava(env, slot);
    connectionRequestPassword = fromJava(env, password);
    connectionRequestGame = std::move(nativeGame);
    connectionRequestPending = true;
}

namespace AndroidBridge {

bool inputBox(
    const std::string& title,
    const std::string& message,
    const std::string& defaultValue,
    std::string& result,
    bool password)
{
    ActivityScope scope;
    if (!scope.valid())
        return false;
    jmethodID method = scope.env->GetMethodID(
        scope.type,
        "showInputDialog",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Z)Ljava/lang/String;"
    );
    if (!method || takeJavaException(scope.env))
        return false;

    jstring jTitle = toJava(scope.env, title);
    jstring jMessage = toJava(scope.env, message);
    jstring jDefault = toJava(scope.env, defaultValue);
    auto value = static_cast<jstring>(scope.env->CallObjectMethod(
        scope.activity, method, jTitle, jMessage, jDefault, password ? JNI_TRUE : JNI_FALSE
    ));
    scope.env->DeleteLocalRef(jTitle);
    scope.env->DeleteLocalRef(jMessage);
    scope.env->DeleteLocalRef(jDefault);
    if (takeJavaException(scope.env) || !value)
        return false;

    const char* chars = scope.env->GetStringUTFChars(value, nullptr);
    if (!chars) {
        scope.env->DeleteLocalRef(value);
        return false;
    }
    result = chars;
    scope.env->ReleaseStringUTFChars(value, chars);
    scope.env->DeleteLocalRef(value);
    return true;
}

int messageBox(
    const std::string& title,
    const std::string& message,
    int buttons,
    int icon,
    int defaultResult)
{
    ActivityScope scope;
    if (!scope.valid())
        return defaultResult;
    jmethodID method = scope.env->GetMethodID(
        scope.type,
        "showMessageDialog",
        "(Ljava/lang/String;Ljava/lang/String;III)I"
    );
    if (!method || takeJavaException(scope.env))
        return defaultResult;

    jstring jTitle = toJava(scope.env, title);
    jstring jMessage = toJava(scope.env, message);
    jint value = scope.env->CallIntMethod(
        scope.activity, method, jTitle, jMessage, buttons, icon, defaultResult
    );
    scope.env->DeleteLocalRef(jTitle);
    scope.env->DeleteLocalRef(jMessage);
    return takeJavaException(scope.env) ? defaultResult : static_cast<int>(value);
}

bool openDocument(const std::string& mimeType, std::string& path)
{
    ActivityScope scope;
    if (!scope.valid())
        return false;
    jmethodID method = scope.env->GetMethodID(
        scope.type,
        "openDocument",
        "(Ljava/lang/String;)Ljava/lang/String;"
    );
    if (!method || takeJavaException(scope.env))
        return false;

    jstring jMimeType = toJava(scope.env, mimeType);
    auto value = static_cast<jstring>(scope.env->CallObjectMethod(scope.activity, method, jMimeType));
    scope.env->DeleteLocalRef(jMimeType);
    if (takeJavaException(scope.env) || !value)
        return false;

    const char* chars = scope.env->GetStringUTFChars(value, nullptr);
    if (!chars) {
        scope.env->DeleteLocalRef(value);
        return false;
    }
    path = chars;
    scope.env->ReleaseStringUTFChars(value, chars);
    scope.env->DeleteLocalRef(value);
    return true;
}

bool importTrackerPack()
{
    ActivityScope scope;
    if (!scope.valid())
        return false;
    jmethodID method = scope.env->GetMethodID(scope.type, "launchPackPicker", "()V");
    if (!method || takeJavaException(scope.env))
        return false;
    scope.env->CallVoidMethod(scope.activity, method);
    return !takeJavaException(scope.env);
}

bool takeLaunchRequest(
    std::string& host,
    std::string& slot,
    std::string& password,
    std::string& game)
{
    std::lock_guard<std::mutex> lock(connectionRequestMutex);
    if (!connectionRequestPending)
        return false;
    host = std::move(connectionRequestHost);
    slot = std::move(connectionRequestSlot);
    password = std::move(connectionRequestPassword);
    game = std::move(connectionRequestGame);
    connectionRequestHost.clear();
    connectionRequestSlot.clear();
    connectionRequestPassword.clear();
    connectionRequestGame.clear();
    connectionRequestPending = false;
    return true;
}

} // namespace AndroidBridge
