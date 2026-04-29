# 说明
## 通过build.py来构建visual studio项目方法：
python build.py --framework glfw --generate_only
项目位置：.\build_system\android\app\build\outputs\apk
## 生成apk的方法
生成Release版本：python build.py --framework --config Release
生成Debug版本：python build.py --framework --config Debug
生成的apk在文件夹：.\build_system\android\app\build\outputs\apk
## 环境变量
vulkan
vcpkg
AndroidSDK


