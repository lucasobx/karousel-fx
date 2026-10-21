configure_file(metadata.json.in ${CMAKE_CURRENT_BINARY_DIR}/metadata.json @ONLY)
set_source_files_properties(plugin.cpp PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/metadata.json)

add_library(karouselfxconfig STATIC)
kconfig_add_kcfg_files(karouselfxconfig karouselfxconfig.kcfgc)
set_target_properties(karouselfxconfig PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(karouselfxconfig PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(karouselfxconfig PUBLIC KF6::ConfigGui)

kcoreaddons_add_plugin(karouselfx
    SOURCES
        plugin.cpp
        karouselfxeffect.cpp
        focusring.cpp
    INSTALL_NAMESPACE "kwin/effects/plugins"
)

set_target_properties(karouselfx PROPERTIES OUTPUT_NAME ${KAROUSELFX_PLUGIN_ID})

target_include_directories(karouselfx PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(karouselfx PRIVATE
    karouselfxconfig
    Qt6::Core
    Qt6::Gui
    KF6::ColorScheme
    KF6::ConfigCore
    KF6::CoreAddons
    KDecoration3::KDecoration
    KWin::kwin
)

kcoreaddons_add_plugin(karouselfx_config
    SOURCES
        karouselfxeffectconfig.cpp
        karouselfxeffectconfig.ui
    INSTALL_NAMESPACE "kwin/effects/configs"
)

set_target_properties(karouselfx_config PROPERTIES OUTPUT_NAME ${KAROUSELFX_PLUGIN_ID}_config AUTOUIC ON)

target_compile_definitions(karouselfx_config PRIVATE KAROUSELFX_PLUGIN_ID="${KAROUSELFX_PLUGIN_ID}")

target_link_libraries(karouselfx_config PRIVATE
    karouselfxconfig
    Qt6::DBus
    Qt6::Widgets
    KF6::KCMUtils
    KF6::WidgetsAddons
)
