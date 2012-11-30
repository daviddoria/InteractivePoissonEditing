cmake_minimum_required(VERSION 2.8)

project(InteractivePoissonEditing)

add_subdirectory(CMakeHelpers)

# Where to copy executables when 'make install' is run
SET( INSTALL_DIR ${CMAKE_INSTALL_PREFIX} )

# Ensure CMake can find the .cmake files in this source directory
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/" ${CMAKE_MODULE_PATH})

# C++11 support
if(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++11")
endif(UNIX)

# ITK
FIND_PACKAGE(ITK REQUIRED ITKCommon ITKIOImageBase
ITKImageIntensity ITKImageFeature ITKMathematicalMorphology
ITKBinaryMathematicalMorphology ITKDistanceMap ITKTestKernel)
INCLUDE(${ITK_USE_FILE})

# Qt
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})

# Eigen3
FIND_PACKAGE(Eigen3 REQUIRED) #requires FindEigen3.cmake to be in the source directory
include_directories(${EIGEN3_INCLUDE_DIR})

# Submodules
set(Mask_BuildMaskQt ON)
UseSubmodule(PoissonEditing InteractivePoissonEditing)
UseSubmodule(QtHelpers InteractivePoissonEditing)
UseSubmodule(ITKQtHelpers InteractivePoissonEditing)

# Add non-compiled files to the project
add_custom_target(PoissonEditingInteractiveSources SOURCES
FileSelectionWidget.h
ImageFileSelector.h
Panel.h
PoissonCloningWidget.h
PoissonEditingWidget.h
)

# Let Qt find it's MOCed files
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

# Build a library of the reusable components
QT4_WRAP_UI(FileSelectorUISrcs FileSelectionWidget.ui FileSelector.ui)
QT4_WRAP_CPP(FileSelectorMOCSrcs FileSelectionWidget.h ImageFileSelector.h Panel.h)
add_library(FileSelectorLibrary ImageFileSelector.cpp Panel.cpp FileSelectionWidget.cpp
           ${FileSelectorUISrcs} ${FileSelectorMOCSrcs})
target_link_libraries(FileSelectorLibrary MaskQt)

# Poisson editing
QT4_WRAP_UI(PoissonEditingUISrcs PoissonEditingWidget.ui)
QT4_WRAP_CPP(PoissonEditingMOCSrcs PoissonEditingWidget.h)

ADD_EXECUTABLE(PoissonEditingInteractive PoissonEditingInteractive.cpp PoissonEditingWidget.cxx
             ${PoissonEditingUISrcs} ${PoissonEditingMOCSrcs})
TARGET_LINK_LIBRARIES(PoissonEditingInteractive ${ITK_LIBRARIES} FileSelectorLibrary ${InteractivePoissonEditing_libraries} ${QT_LIBRARIES})

INSTALL( TARGETS PoissonEditingInteractive RUNTIME DESTINATION ${INSTALL_DIR} )

# Poisson cloning
QT4_WRAP_UI(PoissonCloningUISrcs PoissonCloningWidget.ui)
QT4_WRAP_CPP(PoissonCloningMOCSrcs PoissonCloningWidget.h)
ADD_EXECUTABLE(PoissonCloningInteractive PoissonCloningInteractive.cpp PoissonCloningWidget.cxx
             ${PoissonCloningUISrcs} ${PoissonCloningMOCSrcs})
TARGET_LINK_LIBRARIES(PoissonCloningInteractive ${ITK_LIBRARIES} ${QT_LIBRARIES} FileSelectorLibrary ${InteractivePoissonEditing_libraries})

INSTALL( TARGETS PoissonCloningInteractive RUNTIME DESTINATION ${INSTALL_DIR} )
