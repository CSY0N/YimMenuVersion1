include(FetchContent)
message(STATUS "Fetching GTAV-Classes (Build_3751.0)")
FetchContent_Declare(
    gtav_classes
    GIT_REPOSITORY https://github.com/Hexlane/GTAV-Classes-1.git
    GIT_TAG Build_3751.0
)
FetchContent_MakeAvailable(gtav_classes)
