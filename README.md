# 简介

Native层DEX一键加固脚本。（和[Java层DEX加固](https://github.com/yongyecc/apksheller)相比，只是将Java层关于动态解密加载原始DEX的代码移到了Native层，加深**APK文件防编译保护**)

# 使用说明

```
python -f xxx.apk
```

# 加固原理

将动态加载原DEX的代码放到Native层通过反射来实现。

# 一键加固脚本实现步骤

1. 准备原DEX加密算法以及隐藏位置（壳DEX尾部）

```python
        """
        1. 第一步：确定加密算法
        """
        inKey = 0xFF
        print("[*] 确定加密解密算法，异或: {}".format(str(inKey)))
```

2. 生成壳DEX。（壳Application动态加载原application中需要原application的name字段）

```python
        """
        2. 第二步：准备好壳App
        """
        # 反编译原apk
        decompAPK(fp)
        # 获取Applicaiton name并保存到壳App源码中
        stSrcDexAppName = getAppName(fp)
        save_appName(stSrcDexAppName)
        # 编译出壳DEX
        compileShellDex()
        print("[*] 壳App的class字节码文件编译为:shell.dex完成")
```

3. 修改原APK文件中的AndroidManifest.xml文件的applicationandroid:name字段，实现从壳application启动

```python
        """
        3. 第三步：修改原apk AndroidManifest.xml文件中的Application name字段为壳的Application name字段
        """
        # 替换壳Applicaiton name到原apk的AndroidManifest.xml内
        replaceTag(fp, "cn.yongye.nativeshell.StubApp")
        print("[*] 原apk文件AndroidManifest.xml中application的name字段替换为壳application name字段完成")
```

4. 加密原DEX到壳DEX尾部并将壳DEX替换到原APK中

```python
        """
        4. 替换原apk中的DEX文件为壳DEX
        """
        replaceSDexToShellDex(os.path.join(stCurrentPt, "result.apk"))
        print("[*] 壳DEX替换原apk包内的DEX文件完成")
```

5. 添加脱壳lib库到原apk中

```python
        """
        5. 添加脱壳lib库到原apk中
        """
        addLib("result.apk")
```

6. apk签名

```python
        """
        6. apk签名
        """
        signApk(os.path.join(stCurrentPt, "result.apk"), os.path.join(stCurrentPt, "demo.keystore"))
        print("[*] 签名完成，生成apk文件: {}".format(os.path.join(stCurrentPt, "result.apk")))
```

