cmake_minimum_required(VERSION 3.14)
project(dynamic_string VERSION 1.0.0 LANGUAGES C)

# 引入标准 CMake 模块用于安装和包配置。
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 查找/声明依赖包。
find_package(attrs REQUIRED)
find_package(safe_calc REQUIRED)

# 创建库目标（静态库）。
add_library(dstr_adt STATIC src/dynamic_string.c)
add_library(dstr_adt::dstr_adt ALIAS dstr_adt)
target_link_libraries(dstr_adt PUBLIC attrs::attrs)
target_link_libraries(dstr_adt PRIVATE safe_calc::safe_calc)

# 配置包含目录，使用生成器表达式：
# - BUILD_INTERFACE：直接构建项目时使用。
# - INSTALL_INTERFACE：使用已安装的包时使用。
target_include_directories(dstr_adt PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# 添加测试可执行文件（仅用于开发测试，不安装）。
add_executable(dynamic_string_test src/main.c)
target_link_libraries(dynamic_string_test PRIVATE dstr_adt)

# 安装头文件到标准包含目录。
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.h"
)

# 安装库文件到标准库目录。
install(TARGETS dstr_adt
	EXPORT dstr_adtTargets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 导出目标定义供下游项目使用。
install(EXPORT dstr_adtTargets
	FILE dstr_adtTargets.cmake
	NAMESPACE dstr_adt::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dstr_adt
)

# 从模板生成包配置文件。
configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/dstr_adtConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/dstr_adtConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dstr_adt
)

# 生成版本兼容性文件（支持同主版本兼容策略）。
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/dstr_adtConfigVersion.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)

# 安装包配置文件和版本文件，以支持 find_package() 功能。
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/dstr_adtConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/dstr_adtConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dstr_adt
)
