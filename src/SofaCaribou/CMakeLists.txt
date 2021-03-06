project(SofaCaribou)

set(HEADER_FILES
    Algebra/EigenMatrixWrapper.h
    Forcefield/FictitiousGridElasticForce.h
    Forcefield/FictitiousGridHyperelasticForce.h
    Forcefield/HexahedronElasticForce.h
    Forcefield/HyperelasticForcefield.h
    Forcefield/TetrahedronElasticForce.h
    Forcefield/TractionForce.h
    Material/HyperelasticMaterial.h
    Material/NeoHookeanMaterial.h
    Material/SaintVenantKirchhoffMaterial.h
    Ode/StaticODESolver.h
    Solver/ConjugateGradientSolver.h
    Solver/EigenSparseSolver.h
    Solver/SparseLDLTSolver.h
    Solver/SparseLLTSolver.h
    Solver/SparseLUSolver.h
    Topology/CircleIsoSurface.h
    Topology/CylinderIsoSurface.h
    Topology/FictitiousGrid.h
    Topology/IsoSurface.h
    Topology/SphereIsoSurface.h
    Visitor/AssembleGlobalMatrix.h
    Visitor/ConstrainGlobalMatrix.h
    Visitor/MultiVecEqualVisitor.h
    )

set(TEMPLATE_FILES
    Forcefield/HyperelasticForcefield.inl
    Solver/EigenSparseSolver.inl
    Topology/FictitiousGrid.inl
    )

set(SOURCE_FILES
    Forcefield/FictitiousGridElasticForce.cpp
    Forcefield/FictitiousGridHyperelasticForce.cpp
    Forcefield/HexahedronElasticForce.cpp
    Forcefield/HyperelasticForcefield.cpp
    Forcefield/TetrahedronElasticForce.cpp
    Forcefield/TractionForce.cpp
    Material/HyperelasticMaterial.cpp
    Ode/StaticODESolver.cpp
    Solver/ConjugateGradientSolver.cpp
    Solver/SparseLDLTSolver.cpp
    Solver/SparseLLTSolver.cpp
    Solver/SparseLUSolver.cpp
    Topology/FictitiousGrid.cpp
    Topology/IsoSurface.cpp
    Visitor/AssembleGlobalMatrix.cpp
    Visitor/ConstrainGlobalMatrix.cpp
    Visitor/MultiVecEqualVisitor.cpp
    init.cpp)

# Caribou
if (NOT TARGET Caribou::Algebra)
    find_package(Caribou COMPONENTS Algebra Geometry Topology Mechanics REQUIRED QUIET)
endif()

# Sofa's packages
find_package(SOFA COMPONENTS SofaFramework SofaBase SofaCommon QUIET REQUIRED)

# Optional packages
find_package(Eigen3 REQUIRED NO_MODULE QUIET)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${TEMPLATE_FILES} ${HEADER_FILES})
target_include_directories(${PROJECT_NAME} INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../>")
target_include_directories(${PROJECT_NAME} INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../../>")
target_include_directories(${PROJECT_NAME} PUBLIC "$<INSTALL_INTERFACE:include>")
target_link_libraries(${PROJECT_NAME} PUBLIC SofaCore SofaBaseTopology SofaBaseLinearSolver SofaEigen2Solver)
target_link_libraries(${PROJECT_NAME} PUBLIC Caribou::Algebra Caribou::Geometry Caribou::Topology Caribou::Mechanics)
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
)

if (CARIBOU_OPTIMIZE_FOR_NATIVE)
#    todo(jnbrunet2000@gmail.com) Deactivated since it seems to create mis-alignments
#                                 issues for the SparseMatrix used in the CG. To reproduce:
#                                 https://gist.github.com/jnbrunet/7dfb354fbf2c6e0971674a1a83c29806
#    target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
endif()

if (CARIBOU_WITH_MKL)
    set(MKL_STATIC ON)
    if (NOT CARIBOU_WITH_OPENMP)
        set(MKL_THREADING_VENDOR SEQUENTIAL)
    endif()
    find_package(MKL REQUIRED QUIET)

    target_compile_definitions(${PROJECT_NAME} PUBLIC EIGEN_USE_MKL_ALL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC CARIBOU_WITH_MKL)
    target_include_directories(${PROJECT_NAME} PUBLIC ${MKL_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE  ${MKL_LIBRARIES})
    target_link_options(${PROJECT_NAME} PRIVATE ${MKL_LINKER_FLAGS})
endif()


if (CARIBOU_WITH_OPENMP)
    find_package(OpenMP REQUIRED QUIET)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC CARIBOU_WITH_OPENMP)
endif()

if (CARIBOU_BUILD_TESTS AND SOFAHELPER_HAVE_GTEST)
    add_subdirectory(test)
endif()

if (CARIBOU_WITH_PYTHON_3)
    add_subdirectory(Python)
endif()

# CMake files
list(APPEND HEADER_FILES ${TEMPLATE_FILES})
foreach(header ${HEADER_FILES})
    file(RELATIVE_PATH path_from_package "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
    get_filename_component(output_directory "${path_from_package}" DIRECTORY)
    if (header MATCHES "\\.in$")
        string(REGEX REPLACE "\\.in$" "" output_filepath ${path_from_package})
        configure_file("${header}" "${CMAKE_CURRENT_BINARY_DIR}/${output_filepath}")
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${output_filepath} DESTINATION "include/SofaCaribou/${output_directory}" COMPONENT headers)
    else()
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${path_from_package} DESTINATION "include/SofaCaribou/${output_directory}" COMPONENT headers)
    endif()
endforeach()

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION "bin" COMPONENT applications
    LIBRARY DESTINATION "lib" COMPONENT libraries
    ARCHIVE DESTINATION "lib" COMPONENT libraries
    PUBLIC_HEADER DESTINATION "include/${PROJECT_NAME}" COMPONENT headers
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/SofaCaribouConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/cmake/SofaCaribou/SofaCaribouConfig.cmake"
    INSTALL_DESTINATION
    lib/cmake/SofaCaribou
)
write_basic_package_version_file(
        ${CMAKE_BINARY_DIR}/cmake/SofaCaribou/SofaCaribouConfigVersion.cmake
        VERSION ${Caribou_VERSION}
        COMPATIBILITY AnyNewerVersion)

install(
    EXPORT SofaCaribouTargets 
    DESTINATION "lib/cmake/SofaCaribou" 
    COMPONENT headers)
install(FILES 
        "${CMAKE_BINARY_DIR}/cmake/SofaCaribou/SofaCaribouConfig.cmake"
        "${CMAKE_BINARY_DIR}/cmake/SofaCaribou/SofaCaribouConfigVersion.cmake"
        DESTINATION 
        "lib/cmake/SofaCaribou")
