//
// Created by joker on 2020-03-14.
//

#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <cstdlib>
#include <string>
#include <ostream>
#include <map>


#define TAG "yongye"
#define LOGE(...)   __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

char* jstringToChar(JNIEnv* env, jstring jstr) {
    char* rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("GB2312");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char*) malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

jbyteArray readDexFileFromApk(JNIEnv* env, jobject oApplication){
    //调用java层方法从原DEX中获取壳DEX数据
    LOGE("开始获取壳DEX数据");
    jclass jcFUtil = env->FindClass("cn/yongye/nativeshell/common/FileUtils");
    jbyteArray jbaDex = static_cast<jbyteArray>(env->CallStaticObjectMethod(jcFUtil,
                                                                            env->GetStaticMethodID(
                                                                                    jcFUtil,
                                                                                    "readDexFileFromApk",
                                                                                    "(Landroid/content/Context;)[B"),
                                                                            oApplication));
    LOGE("成功获取到壳DEX数据");
    return jbaDex;
}

/**
 *  解密原始DEX
 * @param env
 * @param jbaShellDex
 * @return
 */
jstring decyptSrcDex(JNIEnv *env, jobject oApplication, jbyteArray jbaShellDex){
    LOGE("开始解密原始DEX");
    jbyte* jbDEX = env->GetByteArrayElements(jbaShellDex, JNI_FALSE);
    jsize inShDexLen = env->GetArrayLength(jbaShellDex);

    char * ptShDex = NULL;
    if(inShDexLen > 0){
        ptShDex = (char*)malloc(inShDexLen + 1);
        memcpy(ptShDex, jbDEX, inShDexLen);
        ptShDex[inShDexLen] = 0;
    }
    env->ReleaseByteArrayElements(jbaShellDex, jbDEX, 0);

    char *ptEncryptedSrcDexLen = &ptShDex[inShDexLen - 4];
    char ptSrcDEXLen[5] = {0};
    int inStart = 0;
    for (int i = 3; i >= 0; i--)
    {
        if (memcmp(&ptShDex[i], "\xff", 1) == 0)
            continue;
        ptSrcDEXLen[inStart] = ptEncryptedSrcDexLen[i] ^ 0xff;
        inStart += 1;
    }
    size_t inSrcDexLen = *(int*)ptSrcDEXLen;
    char *ptSrcDex = (char *)(malloc(inSrcDexLen + 1));
    memcpy(ptSrcDex, &ptShDex[inShDexLen-inSrcDexLen-4], inSrcDexLen);
    for(int i=0; i<inSrcDexLen; i++){
        ptSrcDex[i] ^= 0xff;
    }
    LOGE("解密完成");
    //将解密出的原DEX写入本地文件中
    LOGE("开始dump原DEX");
    jclass jsFile = env->FindClass("java/io/File");
    jclass jsContextWrapper = env->FindClass("android/content/ContextWrapper");

    jobject oCacheDir = env->CallObjectMethod(oApplication, env->GetMethodID(jsContextWrapper, "getCacheDir", "()Ljava/io/File;"));
    jstring stCacheDir = (jstring)env->CallObjectMethod(oCacheDir, env->GetMethodID(jsFile, "getAbsolutePath", "()Ljava/lang/String;"));
    std::string strSrcDexFp = jstringToChar(env, stCacheDir);
    std::string strSrcDexNa = "/yongye";
    strSrcDexFp.append(strSrcDexNa);
    char* ptSrcDexFp = (char *)strSrcDexFp.c_str();
    FILE* fSrcDEX = fopen(strSrcDexFp.c_str(), "w");
    fwrite(ptSrcDex, sizeof(char), inSrcDexLen, fSrcDEX);
    fclose(fSrcDEX);
    LOGE("DUMP 完成");
    return env->NewStringUTF(ptSrcDexFp);
}

/**
 *  加载原始DEX
 * @param env
 * @param oApplication
 * @param stSrcDEXFp
 */
void loadSrcDEX(JNIEnv *env, jobject oApplication, jstring stSrcDEXFp){
    LOGE("开始加载原始DEX");

    jclass clsContextWrapper = env->FindClass("android/content/ContextWrapper");
    jclass clsFile = env->FindClass("java/io/File");
    jclass clsActivityThread = env->FindClass("android/app/ActivityThread");
    jclass clsLoadedApk = env->FindClass("android/app/LoadedApk");
    jclass clsFileUtils = env->FindClass("cn/yongye/nativeshell/common/FileUtils");
    jclass clsMap = env->FindClass("java/util/Map");
    jclass clsDexLoader = env->FindClass("dalvik/system/DexClassLoader");
    jclass clsReference = env->FindClass("java/lang/ref/Reference");


    jobject oCacheDir = env->CallObjectMethod(oApplication, env->GetMethodID(clsContextWrapper, "getCacheDir", "()Ljava/io/File;"));
    jstring stCacheDir = (jstring)env->CallObjectMethod(oCacheDir, env->GetMethodID(clsFile, "getAbsolutePath", "()Ljava/lang/String;"));


    jstring stCurrntPkgNa = static_cast<jstring>(env->CallObjectMethod(oApplication,
                                                                       env->GetMethodID(
                                                                               clsContextWrapper,
                                                                               "getPackageName","()Ljava/lang/String;")));
    jobject olibsFile = env->CallObjectMethod(oApplication, env->GetMethodID(clsContextWrapper, "getDir", "(Ljava/lang/String;I)Ljava/io/File;"), env->NewStringUTF("libs"), 0);
    jobject oParentDirPathFile = env->CallStaticObjectMethod(clsFileUtils, env->GetStaticMethodID(clsFileUtils, "getParent", "(Ljava/io/File;)Ljava/io/File;"), olibsFile);
    jobject oNativeLibFile = env->NewObject(clsFile, env->GetMethodID(clsFile, "<init>", "(Ljava/io/File;Ljava/lang/String;)V"), oParentDirPathFile, env->NewStringUTF("lib"));
    jstring stNativeLibFp = static_cast<jstring>(env->CallObjectMethod(oNativeLibFile,
                                                                       env->GetMethodID(clsFile,
                                                                                        "getAbsolutePath",
                                                                                        "()Ljava/lang/String;")));
    jobject oCurActivityThread = env->CallStaticObjectMethod(clsActivityThread, env->GetStaticMethodID(clsActivityThread, "currentActivityThread", "()Landroid/app/ActivityThread;"));
    jobject oMPkgs = env->GetObjectField(oCurActivityThread, env->GetFieldID(clsActivityThread, "mPackages", "Landroid/util/ArrayMap;"));
    jobject oWRShellLoadedApk = env->CallObjectMethod(oMPkgs, env->GetMethodID(clsMap, "get", "(Ljava/lang/Object;)Ljava/lang/Object;"), stCurrntPkgNa);
    jobject oShellLoadedApk = env->CallObjectMethod(oWRShellLoadedApk, env->GetMethodID(clsReference, "get", "()Ljava/lang/Object;"));
    jobject oSrcDexClassLoader = env->NewObject(clsDexLoader, env->GetMethodID(clsDexLoader, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V"),
                                                stSrcDEXFp, stCacheDir, stNativeLibFp, env->GetObjectField(oShellLoadedApk, env->GetFieldID(clsLoadedApk, "mClassLoader", "Ljava/lang/ClassLoader;")));
    //将原始DEX的classLoader替换进当前线程块中
    env->SetObjectField(oShellLoadedApk, env->GetFieldID(clsLoadedApk, "mClassLoader", "Ljava/lang/ClassLoader;"), oSrcDexClassLoader);
    LOGE("加载原始DEX完成");
}


void startSrcApplication(JNIEnv *env){
    LOGE("开始启动原始DEX的Application组件");
    jclass clsActivityThread = env->FindClass("android/app/ActivityThread");
    jclass clsConfig = env->FindClass("cn/yongye/nativeshell/common/Config");
    jclass clsAppBindData = env->FindClass("android/app/ActivityThread$AppBindData");
    jclass clsApplicationInfo = env->FindClass("android/content/pm/ApplicationInfo");
    jclass clsList = env->FindClass("java/util/List");
    jclass clsApplication = env->FindClass("android/app/Application");
    jclass clsLoadedApk = env->FindClass("android/app/LoadedApk");


    jobject oCurActivityThread = env->CallStaticObjectMethod(clsActivityThread, env->GetStaticMethodID(clsActivityThread, "currentActivityThread", "()Landroid/app/ActivityThread;"));
    jstring strSrcDexMainAppNa = (jstring)env->GetStaticObjectField(clsConfig, env->GetStaticFieldID(clsConfig, "MAIN_APPLICATION", "Ljava/lang/String;"));
    jobject mBoundApplication = env->GetObjectField(oCurActivityThread, env->GetFieldID(clsActivityThread, "mBoundApplication", "Landroid/app/ActivityThread$AppBindData;"));
    jobject oloadedApkInfo = env->GetObjectField(mBoundApplication, env->GetFieldID(clsAppBindData, "info", "Landroid/app/LoadedApk;"));
    env->SetObjectField(oloadedApkInfo, env->GetFieldID(clsLoadedApk, "mApplication", "Landroid/app/Application;"), nullptr);
    jobject omInitApplication = env->GetObjectField(oCurActivityThread, env->GetFieldID(clsActivityThread, "mInitialApplication", "Landroid/app/Application;"));
    jobject omAllApplications = env->GetObjectField(oCurActivityThread, env->GetFieldID(clsActivityThread, "mAllApplications", "Ljava/util/ArrayList;"));
    jboolean oRemoveRet = (jboolean)env->CallBooleanMethod(omAllApplications, env->GetMethodID(clsList, "remove", "(Ljava/lang/Object;)Z"), omInitApplication);
    jobject omApplicationInfo = env->GetObjectField(oloadedApkInfo, env->GetFieldID(clsLoadedApk, "mApplicationInfo", "Landroid/content/pm/ApplicationInfo;"));
    env->SetObjectField(omApplicationInfo, env->GetFieldID(clsApplicationInfo, "className", "Ljava/lang/String;"), strSrcDexMainAppNa);
    jobject oappInfo = env->GetObjectField(mBoundApplication, env->GetFieldID(clsAppBindData, "appInfo", "Landroid/content/pm/ApplicationInfo;"));
    env->SetObjectField(oappInfo, env->GetFieldID(clsApplicationInfo, "className", "Ljava/lang/String;"), strSrcDexMainAppNa);
    jobject omakeApplication = env->CallObjectMethod(oloadedApkInfo, env->GetMethodID(clsLoadedApk, "makeApplication", "(ZLandroid/app/Instrumentation;)Landroid/app/Application;"),
                                                     false, nullptr);
    env->SetObjectField(oCurActivityThread, env->GetFieldID(clsActivityThread, "mInitialApplication", "Landroid/app/Application;"), omakeApplication);
    env->CallVoidMethod(omakeApplication, env->GetMethodID(clsApplication, "onCreate", "()V"));
    LOGE("启动原始DEX Application完成");
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_yongye_nativeshell_StubApp_loadDEX(JNIEnv *env, jobject app) {
    // TODO: implement loadDEX()
    /**
     * 第一步：从原APK中读取壳DEX
     */
    jbyteArray jbaShellDEX = readDexFileFromApk(env, app);

    /**
     * 第二步：解密出原DEX到本地
     */
    jstring strSrcDexFp = decyptSrcDex(env, app, jbaShellDEX);

    /**
     * 第三步：加载原始DEX
     */
     loadSrcDEX(env, app, strSrcDexFp);

     /**
      * 第四步：启动原始DEX的Application组件
      */
    startSrcApplication(env);
}






