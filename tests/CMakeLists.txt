
set(sources
  Main/options.cxx
  Main/fg_commands.cxx
  Main/fg_props.cxx
  Main/globals.cxx
  Main/locale.cxx
  Main/util.cxx
  Main/positioninit.cxx
  Add-ons/Addon.cxx
  Add-ons/AddonManager.cxx
  Add-ons/AddonMetadataParser.cxx
  Add-ons/AddonResourceProvider.cxx
  Add-ons/AddonVersion.cxx
  Add-ons/contacts.cxx
  Add-ons/exceptions.cxx
  Aircraft/controls.cxx
  Aircraft/FlightHistory.cxx
  Aircraft/flightrecorder.cxx
  Aircraft/replay.cxx
  Autopilot/route_mgr.cxx
  Airports/airport.cxx
  Airports/airport.hxx
  Airports/apt_loader.cxx
  Airports/airportdynamicsmanager.cxx
  Airports/airportdynamicsmanager.hxx
  Airports/dynamicloader.cxx
  Airports/dynamics.cxx
  Airports/xmlloader.cxx
  Airports/runwaybase.cxx
  Airports/pavement.cxx
  Airports/parking.cxx
  Airports/groundnetwork.cxx
  Airports/gnnode.cxx
  Airports/runways.cxx
  Airports/runwayprefs.cxx
  Airports/runwayprefloader.cxx
  ATC/CommStation.cxx
#  ATC/GroundController.cxx
#  ATC/atc_mgr.cxx
  Environment/atmosphere.cxx
  Environment/environment.cxx
  Environment/environment_mgr.cxx
  Environment/environment_ctrl.cxx
  Environment/presets.cxx
  Environment/gravity.cxx
  Environment/ridge_lift.cxx
  Environment/magvarmanager.cxx
  Navaids/airways.cxx
  Navaids/fixlist.cxx
  Navaids/markerbeacon.cxx
  Navaids/NavDataCache.cxx
  Navaids/navdb.cxx
  Navaids/navlist.cxx
  Navaids/navrecord.cxx
  Navaids/poidb.cxx
  Navaids/procedure.cxx
  Navaids/positioned.cxx
  Navaids/PositionedOctree.cxx
  Navaids/routePath.cxx
  Navaids/route.cxx
  Navaids/waypoint.cxx
  Navaids/FlightPlan.cxx
  Navaids/LevelDXML.cxx
  Network/HTTPClient.cxx
  Time/TimeManager.cxx
  Time/bodysolver.cxx
  Scripting/NasalSys.cxx
  Scripting/NasalAddons.cxx
  Scripting/NasalCondition.cxx
  Scripting/NasalAircraft.cxx
  Scripting/NasalString.cxx
  Scripting/NasalPositioned.cxx
  Scripting/NasalPositioned_cppbind.cxx
  Scripting/nasal-props.cxx
  Scripting/NasalSGPath.cxx
  Scripting/NasalHTTP.cxx
  Viewer/view.cxx
  Viewer/viewmgr.cxx
)

foreach(s ${sources})
  set_property(DIRECTORY APPEND PROPERTY fgtestlib_sources "${CMAKE_SOURCE_DIR}/src/${s}")
endforeach()

set_property(DIRECTORY APPEND PROPERTY fgtestlib_sources "${CMAKE_SOURCE_DIR}/3rdparty/cjson/cJSON.c")

get_property(fgtestlib_sources DIRECTORY PROPERTY fgtestlib_sources)

add_library(fgtestlib SHARED ${fgtestlib_sources}
  "${CMAKE_SOURCE_DIR}/test_suite/helpers/globals.cxx"
  "${CMAKE_SOURCE_DIR}/test_suite/dataStore.cxx"
  testStubs.cxx
  fake_sgSky.cxx
  fake_sgPrecipitation.cxx
  fake_sound.cxx)

set_target_properties (fgtestlib
  PROPERTIES
  COMPILE_DEFINITIONS "FG_TESTLIB"
)

if(FG_HAVE_GPERFTOOLS)
    target_include_directories(fgtestlib PRIVATE ${GooglePerfTools_INCLUDE_DIR})
    target_link_libraries(fgtestlib ${GooglePerfTools_LIBRARIES})
endif()
target_link_libraries(fgtestlib SimGearCore ${PLATFORM_LIBS} ${SQLITE3_LIBRARY})

add_executable(fgtest fgTestDriver.cxx)
target_link_libraries(fgtest fgtestlib)

macro(flightgear_test name sources)
  add_executable(${name} ${sources})
  target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/tests)
  target_link_libraries(${name} fgtestlib)
  add_test(${name} ${EXECUTABLE_OUTPUT_PATH}/${name})
endmacro()

flightgear_test(test_navs test_navaids2.cxx)

add_executable(testAeroElement testAeroElement.cxx ${CMAKE_SOURCE_DIR}/src/FDM/AIWake/AeroElement.cxx)
target_link_libraries(testAeroElement SimGearCore)
add_test(testAeroElement ${EXECUTABLE_OUTPUT_PATH}/testAeroElement)

add_executable(testAeroMesh testAeroMesh.cxx
  ${CMAKE_SOURCE_DIR}/src/FDM/AIWake/AeroElement.cxx
  ${CMAKE_SOURCE_DIR}/src/FDM/AIWake/AircraftMesh.cxx
  ${CMAKE_SOURCE_DIR}/src/FDM/AIWake/WakeMesh.cxx
  ${CMAKE_SOURCE_DIR}/src/FDM/AIWake/AIWakeGroup.cxx
  ${CMAKE_SOURCE_DIR}/src/FDM/LaRCsim/ls_matrix.c
  )
set_target_properties (testAeroMesh PROPERTIES COMPILE_DEFINITIONS "FG_TESTLIB")
target_include_directories(testAeroMesh PRIVATE ${CMAKE_SOURCE_DIR}/tests
  ${CMAKE_SOURCE_DIR}/src/FDM/JSBSim)
target_link_libraries(testAeroMesh SimGearCore JSBSim)
add_test(testAeroMesh ${EXECUTABLE_OUTPUT_PATH}/testAeroMesh)
