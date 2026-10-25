idf_component_register(
    SRCS 
        "companion_computer.cpp"
        "mission_planner.cpp"
        "secure_mavlink.cpp"
        "mission_executor.cpp"
        "weather_sensor_driver.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver mqtt mbedtls json nvs_flash
)
