# Install the extension "UrmPlugin.so" lib
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Extensions/*.cpp")
add_library(UrmPlugin ${SOURCES})
set_target_properties(UrmPlugin PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_link_libraries(UrmPlugin UrmExtAPIs RestuneCore UrmAuxUtils)
target_include_directories(UrmPlugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Extensions/Include)

# install to standard /usr/lib or /lib
install(TARGETS UrmPlugin DESTINATION ${CMAKE_INSTALL_LIBDIR}/urm/)

# Install the configs to /etc/urm/custom
file(GLOB pluginConfigs "${CMAKE_CURRENT_SOURCE_DIR}/Configs/*.yaml")
install(
    FILES ${pluginConfigs}
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/urm/target
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

# Install target-configs to /etc/urm/target/
file(GLOB targetConfigs "${CMAKE_CURRENT_SOURCE_DIR}/Configs/target-specific/*")
install(
    DIRECTORY ${targetConfigs}
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/urm/target
)

# Install post_boot scripts to /usr/libexec/urm/initscripts/post_boot/
# These are vendor-provided executables, not configuration, so they do not
# belong below /etc.
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/initscripts/post_boot/
    DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/urm/initscripts/post_boot
    FILES_MATCHING
    PATTERN "*.sh"
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
)
