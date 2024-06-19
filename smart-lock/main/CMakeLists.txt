# 注册组件
idf_component_register(
    # SRCS 要包含的c语言文件的相对路径
    # 相对于当前这个文件CMakeLists.txt路径的相对路径
    SRCS
        "./smart_lock_main.c"
        "./drivers/KEYBOARD_Driver.c"
        "./drivers/LED_Driver.c"
        "./drivers/AUDIO_Driver.c"
        "./drivers/FINGERPRINT_Driver.c"
        "./drivers/MOTOR_Driver.c"
        "./drivers/BLUETOOTH_Driver.c"
        "./drivers/WIFI_Driver.c"
        "./drivers/OTA_Driver.c"
        "./drivers/MQTT_Driver.c"
        "./drivers/SNTP_Driver.c"
        "./drivers/PASSWORD_Driver.c"
    # INCLUDE_DIRS 要包含的头文件的相对路径
    INCLUDE_DIRS "." "./drivers"
)
