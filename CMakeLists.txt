cmake_minimum_required(VERSION 3.31)

project(SpectrometerUI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)
set(QT_QML_GENERATE_AOTSTATS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(BUILD_TESTS "Build tests" OFF)
option(USE_ASAN "Use Address Sanitizer" OFF)

if(ANDROID)
    set(USE_QT_SERIAL_PORT OFF)
else()
    option(USE_QT_SERIAL_PORT "Use QSerialPort" ON)
endif()

find_package(Qt6 6.11 REQUIRED COMPONENTS QuickControls2 ShaderTools)
if(USE_QT_SERIAL_PORT)
    find_package(Qt6 REQUIRED COMPONENTS SerialPort)
else()
    if(NOT ANDROID)
        cmake_pkg_config(IMPORT libusb-1.0 REQUIRED)
    endif()
endif()

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0002 NEW)
qt_policy(SET QTP0004 NEW)

if(USE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Rel")
    unset(MAYBE_NO_CACHEGEN)
    set(MAYBE_DISCARD_QML_CONTENTS DISCARD_QML_CONTENTS)
else()
    set(MAYBE_NO_CACHEGEN NO_CACHEGEN)
    unset(MAYBE_DISCARD_QML_CONTENTS)
endif()

set(BACKEND_FILES
    Headers.hpp
    BlackBody.hpp BlackBody.cpp
    DaylightWorker.hpp DaylightWorker.cpp
    Exposure.hpp
    SpdData.hpp
    Application.cpp Application.hpp
    SpectrometerWorker.hpp SpectrometerWorker.cpp
    TorchBearerWorker.hpp TorchBearerWorker.cpp
    FileWorker.hpp FileWorker.cpp
    BlackBodyWorker.hpp BlackBodyWorker.cpp
    DataSeqIterp.hpp
    Cri.cpp Cri.hpp
    CriData.cpp CriData.hpp
    Tm30.cpp Tm30.hpp
    Tm30Data.cpp Tm30Data.hpp
    BlueLightHazardData.cpp BlueLightHazardData.hpp
    LuminousEfficiencyData.cpp LuminousEfficiencyData.hpp
    ErythemalActionData.cpp ErythemalActionData.hpp
    VitaminDActionData.cpp VitaminDActionData.hpp
    CieXyz1931TwoDegData.cpp CieXyz1931TwoDegData.hpp
    CieXyz1964TenDegData.cpp CieXyz1964TenDegData.hpp
    CieDIlluminantData.cpp CieDIlluminantData.hpp
    ScotopicEfficiencyData.cpp ScotopicEfficiencyData.hpp
    MelanopicActionData.cpp MelanopicActionData.hpp
    Ohno2013.hpp Ohno2013.cpp
    Helpers.hpp Helpers.cpp
    MacAdamEllipse.hpp MacAdamEllipse.cpp
    CMF10deg.inc
)
if(NOT USE_QT_SERIAL_PORT)
    list(APPEND BACKEND_FILES
        TorchBearerDevice.hpp TorchBearerDevice.cpp
        CH341Uart.cpp CH341Uart.hpp
    )
endif()

qt_add_executable(${PROJECT_NAME}
    ${BACKEND_FILES}
)

if(NOT ANDROID)
    qt_add_resources(${PROJECT_NAME} "resources"
        FILES
            "icons/icon.png"
    )
endif()

qt_add_shaders(${PROJECT_NAME} "shaders"
    GLSL "300 es,120,330"
    FILES
        qml/Default.vert
        qml/LightSpectrumFill.frag
        qml/ChromaticityDiagramFill.frag
)

set_source_files_properties(
    qml/AppQmlSingleton.qml
    PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(${PROJECT_NAME}
    URI ${PROJECT_NAME}_files
    ${MAYBE_DISCARD_QML_CONTENTS}
    ${MAYBE_NO_CACHEGEN}
    QML_FILES
        qml/MyDoubleSpinBox.qml
        qml/MySwipeView.qml
        qml/MyListView.qml
        qml/MyComboBox.qml
        qml/MyMessageDialog.qml
        qml/MainWindow.qml
        qml/MainView.qml
        qml/WavelengthSpinBox.qml
        qml/StoredMeasurements.qml
        qml/LikedChooserDialog.qml
        qml/FlickableScrollBehavior.qml
        qml/MeasurementNameDialog.qml
        qml/Spectrum.qml
        qml/CriGraph.qml
        qml/DuvGraph.qml
        qml/Tm30Graph.qml
        qml/SpectrometerPanel.qml
        qml/ButtonsPanel.qml
        qml/MeasurementsPanel.qml
        qml/LikedMeasurementsView.qml
        qml/MeasurementsView.qml
        qml/AppQmlSingleton.qml
        qml/DeviceOpenDialog.qml
        qml/BBOpenDialog.qml
        qml/FileOpenDialog.qml
        qml/DIlluminantOpenDialog.qml
        qml/OverlayItem.qml
        qml/MyDialog.qml
        qml/MyMenu.qml
    SOURCES
        Spectrum.hpp Spectrum.cpp
        CriGraph.hpp CriGraph.cpp
        DuvGraph.hpp DuvGraph.cpp
        Tm30Graph.hpp Tm30Graph.cpp
        SpectrometerBridge.hpp SpectrometerBridge.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    Headers.hpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::QuickControls2
)
if(USE_QT_SERIAL_PORT)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt6::SerialPort
    )
endif()

if(ANDROID)
    set(ANDROID_LIBUSB_DIR "" CACHE STRING "Android libusb directory")

    set(ANDROID_EXTRA_LIBS
        "${ANDROID_LIBUSB_DIR}/android/libs/${ANDROID_ABI}/libusb1.0.so"
    )
    list(JOIN ANDROID_EXTRA_LIBS "," ANDROID_EXTRA_LIBS_STR)

    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${ANDROID_EXTRA_LIBS}
    )

    target_include_directories(${PROJECT_NAME}
        PRIVATE
        "${ANDROID_LIBUSB_DIR}/libusb"
    )

    set_target_properties(${PROJECT_NAME}
        PROPERTIES
        QT_ANDROID_EXTRA_LIBS ${ANDROID_EXTRA_LIBS_STR}
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android"
        QT_ANDROID_VERSION_CODE 1
        QT_ANDROID_VERSION_NAME ${CMAKE_PROJECT_VERSION}
    )
else()
    if(NOT USE_QT_SERIAL_PORT)
        target_link_libraries(${PROJECT_NAME}
            PRIVATE
            PkgConfig::libusb-1.0
        )
    endif()
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_USE_QSTRINGBUILDER
    -DPROJECT_VERSION="${CMAKE_PROJECT_VERSION}"
)
if(USE_QT_SERIAL_PORT)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        USE_QT_SERIAL_PORT
    )
endif()

set(COMPILE_OPTIONS
    -Wsign-compare
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
    ${COMPILE_OPTIONS}
)

if(BUILD_TESTS)
    enable_testing()

    find_package(Qt6 REQUIRED COMPONENTS Test)

    add_executable(AlgorithmTests
        tests/AlgorithmTests.hpp
        tests/AlgorithmTests.cpp
        tests/AlgorithmTests.qrc
        Tm30.cpp
        Cri.cpp
        Tm30Data.cpp
        CriData.cpp
        BlackBody.cpp
        CieXyz1931TwoDegData.cpp
        CieXyz1964TenDegData.cpp
        CieDIlluminantData.cpp
        Ohno2013.cpp
        Helpers.cpp
        MacAdamEllipse.cpp
    )
    target_precompile_headers(AlgorithmTests PRIVATE Headers.hpp)
    target_include_directories(AlgorithmTests PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(AlgorithmTests PRIVATE Qt6::Test)
    target_compile_definitions(AlgorithmTests
        PRIVATE
            ALGORITHM_TESTS
            QT_NO_CAST_FROM_ASCII
            QT_USE_QSTRINGBUILDER
    )
    target_compile_options(AlgorithmTests
        PRIVATE
        ${COMPILE_OPTIONS}
    )
    add_test(NAME AlgorithmTests COMMAND $<TARGET_FILE:AlgorithmTests>)
endif()
