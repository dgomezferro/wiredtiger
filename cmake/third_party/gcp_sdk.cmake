include(ExternalProject)
include(FetchContent)
include(GNUInstallDirs)
include(${CMAKE_SOURCE_DIR}/cmake/helpers.cmake)

# Skip the GCP SDK build step if the extension is not enabled.
if(NOT ENABLE_GCP)
    return()
endif()

config_choice(
    IMPORT_GCP_SDK
    "Specify how to import the GCP SDK"
    OPTIONS
        "none;IMPORT_GCP_SDK_NONE;NOT ENABLE_GCP"
        "package;IMPORT_GCP_SDK_PACKAGE;ENABLE_GCP"
        "external;IMPORT_GCP_SDK_EXTERNAL;ENABLE_GCP"
)

if(IMPORT_GCP_SDK_NONE)
    message(FATAL_ERROR "Cannot enable GCP extension without specifying an IMPORT_GCP_SDK method (package, external).")
endif()

if(IMPORT_GCP_SDK_PACKAGE)
    #set(google-cloud-cpp)
    #find_package(googleapis)
    # We want to find the packge installed on the system.
elseif(IMPORT_GCP_SDK_EXTERNAL)
    find_package(absl REQUIRED)
    # Download and install the GCP CPP SDK into the build directory.
    ExternalProject_Add(gcp-sdk
        PREFIX gcp-sdk-cpp
        GIT_REPOSITORY https://github.com/googleapis/google-cloud-cpp.git
        GIT_TAG v2.5.0
        CMAKE_ARGS
            -DBUILD_DEPS=ON
            -DBUILD_SHARED_LIBS=ON
            -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/gcp-sdk-cpp/install
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DGOOGLE_CLOUD_CPP_ENABLE=storage
            -DGOOGLE_CLOUD_CPP_ENABLE_EXAMPLES=OFF
            -DBUILD_TESTING=OFF
        BUILD_ALWAYS FALSE
        BUILD_BYPRODUCTS
            ${CMAKE_CURRENT_BINARY_DIR}/gcp-sdk-cpp/install/${CMAKE_INSTALL_LIBDIR}/libgoogle_cloud_cpp_storage${CMAKE_SHARED_LIBRARY_SUFFIX}
            ${CMAKE_CURRENT_BINARY_DIR}/gcp-sdk-cpp/install/${CMAKE_INSTALL_LIBDIR}/libgoogle_cloud_cpp_common${CMAKE_SHARED_LIBRARY_SUFFIX}
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/gcp-sdk-cpp/install
        TEST_COMMAND ""
        UPDATE_COMMAND ""
    )
    ExternalProject_Get_Property(gcp-sdk INSTALL_DIR)
    file(MAKE_DIRECTORY ${INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR})
    set(gcp_sdk_include_location ${INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR})

    # Set the path variables to be used for the GCP targets.
    set(gcp_storage_lib_location ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/libgoogle_cloud_cpp_storage${CMAKE_SHARED_LIBRARY_SUFFIX})
    #set(gcp_common_lib_location ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/libgoogle_cloud_cpp_common${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()

set(gcp_sdk_include_location ${INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR})

add_library(gcp_storage_lib SHARED IMPORTED)
#add_library(gcp_common_lib SHARED IMPORTED)

# Small workaround to declare the include directory under INTERFACE_INCLUDE_DIRECTORIES during the configuration phase.
set_target_properties(gcp_storage_lib PROPERTIES
    IMPORTED_LOCATION ${gcp_storage_lib_location}
    INTERFACE_INCLUDE_DIRECTORIES ${gcp_sdk_include_location}
)

# Small workaround to declare the include directory under INTERFACE_INCLUDE_DIRECTORIES during the configuration phase.
# set_target_properties(gcp_common_lib PROPERTIES
#     IMPORTED_LOCATION ${gcp_common_lib_location}
#     INTERFACE_INCLUDE_DIRECTORIES ${gcp_sdk_include_location}
# )

# add_dependencies(gcp_storage_lib gcp_common_lib)