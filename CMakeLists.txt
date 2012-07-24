cmake_minimum_required(VERSION 2.8)

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
# FIND_PACKAGE(Eigen3 REQUIRED) #requires FindEigen3.cmake to be in the source directory
include_directories(/media/portable/src/Eigen)
include_directories(${EIGEN3_INCLUDE_DIR} ${EIGEN3_INCLUDE_DIR}/unsupported/)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET")

# UMFPACK
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

#   QT4_WRAP_UI(FileSelectorUISrcs FileSelector.ui FileSelectionWidget.ui)
#   QT4_WRAP_CPP(FileSelectorMOCSrcs FileSelector.h FileSelectionWidget.h)
#   add_library(FileSelectorLibrary FileSelector.cpp FileSelectionWidget.cpp ${FileSelectorUISrcs} ${FileSelectorMOCSrcs})
  QT4_WRAP_UI(FileSelectorUISrcs FileSelectionWidget.ui FileSelector.ui)
  QT4_WRAP_CPP(FileSelectorMOCSrcs FileSelectionWidget.h ImageFileSelector.h Panel.h)
  add_library(FileSelectorLibrary ImageFileSelector.cpp Panel.cpp FileSelectionWidget.cpp ${FileSelectorUISrcs} ${FileSelectorMOCSrcs})
#endif()

SET(PoissonEditingInteractive CACHE BOOL "Build the Poisson editing interactive version")
if(PoissonEditingInteractive)
  message("Building PoissonEditingInteractive")
  QT4_WRAP_UI(PoissonEditingUISrcs PoissonEditingWidget.ui)
  QT4_WRAP_CPP(PoissonEditingMOCSrcs PoissonEditingWidget.h)

  ADD_EXECUTABLE(PoissonEditingInteractive PoissonEditingInteractive.cpp PoissonEditingWidget.cxx
                 ${PoissonEditingUISrcs} ${PoissonEditingMOCSrcs})
  TARGET_LINK_LIBRARIES(PoissonEditingInteractive ${ITK_LIBRARIES} umfpack ${QT_LIBRARIES} FileSelectorLibrary libHelpers libITKHelpers libQtHelpers libMask)
  #set_target_properties(PoissonCloning PROPERTIES COMPILE_DEFINITIONS "${DEFINITIONS}")
  INSTALL( TARGETS PoissonEditingInteractive RUNTIME DESTINATION ${INSTALL_DIR} )
  #ADD_TEST(NAME PoissonCloningTest COMMAND ${EXECUTABLE_OUTPUT_PATH}/PoissonCloning ${CMAKE_SOURCE_DIR}/Testing/data/F16/F16Source.png ${CMAKE_SOURCE_DIR}/Testing/data/F16/F16Mask.png ${CMAKE_SOURCE_DIR}/Testing/data/F16/F16Target.png cloning.png)
endif()
# 
# SET(PoissonCloningInteractive CACHE BOOL "Build the Poisson cloning interactive version")
# if(PoissonCloningInteractive)
#   message("Building PoissonCloningInteractive")
#   QT4_WRAP_UI(PoissonCloningUISrcs PoissonCloningWidget.ui)
#   QT4_WRAP_CPP(PoissonCloningMOCSrcs PoissonCloningWidget.h)
#   ADD_EXECUTABLE(PoissonCloningInteractive PoissonCloningInteractive.cpp PoissonCloningWidget.cxx
#                  HelpersQt.cpp ${PoissonCloningUISrcs} ${PoissonCloningMOCSrcs})
#   TARGET_LINK_LIBRARIES(PoissonCloningInteractive ${ITK_LIBRARIES} umfpack ${QT_LIBRARIES} FileSelectorLibrary HelpersLibrary)
#   #set_target_properties(PoissonCloning PROPERTIES COMPILE_DEFINITIONS "${DEFINITIONS}")
#   INSTALL( TARGETS PoissonCloningInteractive RUNTIME DESTINATION ${INSTALL_DIR} )
#   #ADD_TEST(NAME PoissonCloningTest COMMAND ${EXECUTABLE_OUTPUT_PATH}/PoissonCloning ${CMAKE_SOURCE_DIR}/Testing/data/F16/F16Source.png ${CMAKE_SOURCE_DIR}/Testing/data/F16/F16Mask.png ${CMAKE_SOURCE_DIR}/Testing/data/F16/F16Target.png cloning.png)
# endif()
