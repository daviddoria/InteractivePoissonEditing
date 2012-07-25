cmake_minimum_required(VERSION 2.8)

# Where to copy executables when 'make install' is run
SET( INSTALL_DIR ${CMAKE_INSTALL_PREFIX} )

# Ensure CMake can find the .cmake files in this source directory
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/" ${CMAKE_MODULE_PATH})

# C++11 support
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x")

# ITK
FIND_PACKAGE(ITK REQUIRED ITKCommon ITKIOImageBase
ITKImageIntensity ITKImageFeature ITKMathematicalMorphology ITKBinaryMathematicalMorphology ITKDistanceMap)
INCLUDE(${ITK_USE_FILE})

# Qt
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})

# Eigen3
FIND_PACKAGE(Eigen3 REQUIRED) #requires FindEigen3.cmake to be in the source directory
include_directories(${EIGEN3_INCLUDE_DIR} ${EIGEN3_INCLUDE_DIR}/unsupported/)

# UMFPACK
# TODO: Note that this fails with a strange libamd error if these lines are repeated in a submodule.
# Also note that removing them here and leaving them in the submodule leads to a "can't find umfpack.h" compiler error
FIND_PACKAGE(Umfpack REQUIRED)
include_directories(${UMFPACK_INCLUDES})

# Submodules
if(NOT TARGET PoissonEditing)
  add_subdirectory(PoissonEditing)
  include_directories(${PoissonEditing_includes})
endif()

if(NOT TARGET QtHelpers)
  add_subdirectory(QtHelpers)
  include_directories(${QtHelpers_includes})
endif()

if(NOT TARGET ITKQtHelpers)
  add_subdirectory(ITKQtHelpers)
  include_directories(${ITKQtHelpers_includes})
endif()

# Let Qt find it's MOCed files
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

# Build a library of the reusable components
QT4_WRAP_UI(FileSelectorUISrcs FileSelectionWidget.ui FileSelector.ui)
QT4_WRAP_CPP(FileSelectorMOCSrcs FileSelectionWidget.h ImageFileSelector.h Panel.h)
add_library(FileSelectorLibrary ImageFileSelector.cpp Panel.cpp FileSelectionWidget.cpp ${FileSelectorUISrcs} ${FileSelectorMOCSrcs})
target_link_libraries(FileSelectorLibrary MaskQt)

SET(PoissonEditingInteractive CACHE BOOL "Build interactive Poisson editing?")
if(PoissonEditingInteractive)
  QT4_WRAP_UI(PoissonEditingUISrcs PoissonEditingWidget.ui)
  QT4_WRAP_CPP(PoissonEditingMOCSrcs PoissonEditingWidget.h)

  ADD_EXECUTABLE(PoissonEditingInteractive PoissonEditingInteractive.cpp PoissonEditingWidget.cxx
                 ${PoissonEditingUISrcs} ${PoissonEditingMOCSrcs})
  TARGET_LINK_LIBRARIES(PoissonEditingInteractive ${ITK_LIBRARIES} FileSelectorLibrary PoissonEditing ${QT_LIBRARIES})

  INSTALL( TARGETS PoissonEditingInteractive RUNTIME DESTINATION ${INSTALL_DIR} )
endif()

SET(PoissonCloningInteractive CACHE BOOL "Build interactive Poisson cloning?")
if(PoissonCloningInteractive)
  QT4_WRAP_UI(PoissonCloningUISrcs PoissonCloningWidget.ui)
  QT4_WRAP_CPP(PoissonCloningMOCSrcs PoissonCloningWidget.h)
  ADD_EXECUTABLE(PoissonCloningInteractive PoissonCloningInteractive.cpp PoissonCloningWidget.cxx
                 ${PoissonCloningUISrcs} ${PoissonCloningMOCSrcs})
  TARGET_LINK_LIBRARIES(PoissonCloningInteractive ${ITK_LIBRARIES} ${QT_LIBRARIES} FileSelectorLibrary PoissonEditing)

  INSTALL( TARGETS PoissonCloningInteractive RUNTIME DESTINATION ${INSTALL_DIR} )
endif()
