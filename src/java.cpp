#include "java.hpp"

#include "jtypes.hpp"
#include "main.hpp"

struct cleanup {
    cleanup(std::function<void()> fn) : fn(fn) {}
    ~cleanup() { fn(); }

    std::function<void()> fn;
};

JNIEnv* MetaCore::Java::GetEnv() {
    JNIEnv* env;

    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_6;
    args.name = NULL;
    args.group = NULL;

    modloader_jvm->AttachCurrentThread(&env, &args);

    return env;
}

jclass MetaCore::Java::GetClass(JNIEnv* env, FindClass clazz) {
    if (clazz.clazz)
        return clazz.clazz;
    if (clazz.instance)
        return env->GetObjectClass(clazz.instance);
    if (!clazz.name.empty())
        return env->FindClass(clazz.name.c_str());
    return nullptr;
}

jmethodID MetaCore::Java::GetMethodID(JNIEnv* env, FindClass clazz, FindMethodID method) {
    if (method.method)
        return method.method;
    jclass foundClass = GetClass(env, clazz);
    if (!foundClass || method.name.empty() || method.signature.empty())
        return nullptr;
    if (clazz.instance)
        return env->GetMethodID(foundClass, method.name.c_str(), method.signature.c_str());
    else
        return env->GetStaticMethodID(foundClass, method.name.c_str(), method.signature.c_str());
    // delete local ref of class?
}

jfieldID MetaCore::Java::GetFieldID(JNIEnv* env, FindClass clazz, FindFieldID field) {
    if (field.field)
        return field.field;
    jclass foundClass = GetClass(env, clazz);
    if (!foundClass || field.name.empty() || field.signature.empty())
        return nullptr;
    if (clazz.instance)
        return env->GetFieldID(foundClass, field.name.c_str(), field.signature.c_str());
    else
        return env->GetStaticFieldID(foundClass, field.name.c_str(), field.signature.c_str());
}

jobject MetaCore::Java::NewObject(JNIEnv* env, FindClass clazz, FindMethodID init, ...) {
    va_list va;
    va_start(va, init);
    cleanup c([&va]() { va_end(va); });
    auto foundClass = GetClass(env, clazz);
    auto foundMethod = GetMethodID(env, {foundClass, foundClass}, init);
    if (!foundClass || !foundMethod)
        return nullptr;
    return env->NewObjectV(foundClass, foundMethod, va);
}

jobject MetaCore::Java::NewObject(JNIEnv* env, FindClass clazz, std::string init, ...) {
    va_list va;
    va_start(va, init);
    cleanup c([&va]() { va_end(va); });
    auto foundClass = GetClass(env, clazz);
    auto foundMethod = GetMethodID(env, {foundClass, foundClass}, {"<init>", init});
    if (!foundClass || !foundMethod)
        return nullptr;
    return env->NewObjectV(foundClass, foundMethod, va);
}

template <class T>
T MetaCore::Java::RunMethod(JNIEnv* env, FindClass clazz, FindMethodID method, ...) {
    va_list va;
    va_start(va, method);
    cleanup c([&va, &env]() {
        va_end(va);
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            throw std::runtime_error("JNI error in RunMethod");
        }
    });
    if (clazz.instance) {
        auto foundMethod = GetMethodID(env, clazz, method);
        if constexpr (std::is_same_v<T, void>)
            std::invoke(TypeResolver<T>::JMethod, env, clazz.instance, foundMethod, va);
        else
            return (T) std::invoke(TypeResolver<T>::JMethod, env, clazz.instance, foundMethod, va);
    } else {
        auto foundClass = GetClass(env, clazz);
        auto foundMethod = GetMethodID(env, foundClass, method);
        if constexpr (std::is_same_v<T, void>)
            std::invoke(TypeResolver<T>::JStaticMethod, env, foundClass, foundMethod, va);
        else
            return (T) std::invoke(TypeResolver<T>::JStaticMethod, env, foundClass, foundMethod, va);
    }
}

template <class T>
T MetaCore::Java::GetField(JNIEnv* env, FindClass clazz, FindFieldID field) {
    if (clazz.instance) {
        auto foundField = GetFieldID(env, clazz, field);
        return (T) std::invoke(TypeResolver<T>::JGetField, env, clazz.instance, foundField);
    } else {
        auto foundClass = GetClass(env, clazz);
        auto foundField = GetFieldID(env, foundClass, field);
        return (T) std::invoke(TypeResolver<T>::JGetStaticField, env, foundClass, foundField);
    }
}

template <class T>
void MetaCore::Java::SetField(JNIEnv* env, FindClass clazz, FindFieldID field, T value) {
    if (clazz.instance) {
        auto foundField = GetFieldID(env, clazz, field);
        std::invoke(TypeResolver<T>::JSetField, env, clazz.instance, foundField, value);
    } else {
        auto foundClass = GetClass(env, clazz);
        auto foundField = GetFieldID(env, foundClass, field);
        std::invoke(TypeResolver<T>::JSetStaticField, env, foundClass, foundField, value);
    }
}

jclass MetaCore::Java::LoadClass(JNIEnv* env, std::string name, std::string_view dexBytes) {
    JNIFrame frame(env, 12);

    auto dexBuffer = env->NewDirectByteBuffer((void*) dexBytes.data(), dexBytes.length() - 1);

    // not sure if necessary to run this on the UnityPlayer class
    auto baseClassLoader = RunMethod<jobject>(
        env, {GetClass(env, {"java/lang/Class"}), GetClass(env, {"com/unity3d/player/UnityPlayer"})}, {"getClassLoader", "()Ljava/lang/ClassLoader;"}
    );

    auto classLoader =
        NewObject(env, {"dalvik/system/InMemoryDexClassLoader"}, "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V", dexBuffer, baseClassLoader);

    auto loadedClass = RunMethod<jobject>(env, classLoader, {"loadClass", "(Ljava/lang/String;)Ljava/lang/Class;"}, env->NewStringUTF(name.c_str()));

    return (jclass) env->NewGlobalRef(loadedClass);
}

std::string MetaCore::Java::ConvertString(JNIEnv* env, jstring string) {
    if (!string)
        return "";
    char const* chars = env->GetStringUTFChars(string, nullptr);
    int length = env->GetStringUTFLength(string);
    std::string ret(chars, length);
    env->ReleaseStringUTFChars(string, chars);
    return ret;
}

std::string MetaCore::Java::GetClassName(JNIEnv* env, jclass clazz) {
    jstring string = RunMethod<jstring>(env, {clazz, clazz}, {"getName", "()Ljava/lang/String;"});
    return ConvertString(env, string);
}

#define SPECIALIZATION(type)                                                                            \
    template type MetaCore::Java::RunMethod(JNIEnv* env, FindClass clazz, FindMethodID method, ...); \
    template type MetaCore::Java::GetField(JNIEnv* env, FindClass clazz, FindFieldID field);         \
    template void MetaCore::Java::SetField(JNIEnv* env, FindClass clazz, FindFieldID field, type value)

SPECIALIZATION(jobject);
SPECIALIZATION(jstring);
SPECIALIZATION(bool);
SPECIALIZATION(uint8_t);
SPECIALIZATION(char);
SPECIALIZATION(short);
SPECIALIZATION(int);
SPECIALIZATION(long);
SPECIALIZATION(float);
SPECIALIZATION(double);

template void MetaCore::Java::RunMethod(JNIEnv* env, FindClass clazz, FindMethodID method, ...);
