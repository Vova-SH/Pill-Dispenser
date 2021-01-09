FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
    SRCS "main.c"
        "servo.c"
        "led.c"
        "button.c"
        "wifi.c"
        "server.c"
        "flash.c"
        "ntp.c"
        "schedule.c"
        "server_auth_digest.c"
        "wifi_connection/esptouch.c"
        "wifi_connection/wps.c"
    REQUIRES idf::mbedtls
)