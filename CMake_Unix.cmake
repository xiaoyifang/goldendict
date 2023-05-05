#### Include Paths

if (APPLE)
    # old & new homebrew's include paths
    target_include_directories(${GOLDENDICT} PRIVATE /usr/local/include /opt/homebrew/include)
endif ()

target_include_directories(${GOLDENDICT} PRIVATE
        ${PROJECT_SOURCE_DIR}/thirdparty)

#### Special Platform supporting libraries

if (LINUX OR BSD)
    find_package(X11 REQUIRED)
    pkg_check_modules(LIBXTST IMPORTED_TARGET xtst)
    target_compile_definitions(${GOLDENDICT} PUBLIC HAVE_X11)
    target_link_libraries(${GOLDENDICT} PRIVATE X11 PkgConfig::LIBXTST)
endif ()

if (APPLE)
    find_library(CARBON_LIBRARY Carbon REQUIRED)
    target_link_libraries(${GOLDENDICT} PRIVATE ${CARBON_LIBRARY})
endif ()

##### Finding packages from package manager


find_package(PkgConfig REQUIRED)
# Provided by Cmake
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Iconv REQUIRED)


# Consider all PkgConfig dependencies as one
pkg_check_modules(PKGCONFIG_DEPS IMPORTED_TARGET
        hunspell
        lzo2
        opencc
        vorbis # .ogg
        vorbisfile
        liblzma
        libzstd
        )

target_link_libraries(${GOLDENDICT} PRIVATE
        # pkg-config packages need manually link
        PkgConfig::PKGCONFIG_DEPS
        BZip2::BZip2
        ZLIB::ZLIB #hidden requirement of dsl_details.cc and more?
        Iconv::Iconv
        )

if (WITH_FFMPEG_PLAYER)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
            libavcodec
            libavformat
            libavutil
            libswresample
            )
    target_link_libraries(${GOLDENDICT} PRIVATE PkgConfig::FFMPEG)
endif ()

if (WITH_XAPIAN)
    find_package(Xapian REQUIRED) # https://github.com/xapian/xapian/tree/master/xapian-core/cmake
    target_link_libraries(${GOLDENDICT} PRIVATE ${XAPIAN_LIBRARIES})
endif ()

if (WITH_EPWING_SUPPORT)
    add_subdirectory(thirdparty/eb)
    target_link_libraries(${GOLDENDICT} PRIVATE eb)
endif ()

