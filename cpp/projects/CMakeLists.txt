cmake_minimum_required (VERSION 2.6)
project(StereoTests)

# Projects
add_subdirectory(objectsMap)
add_subdirectory(disparityTest)
add_subdirectory(stereoAcquisition)
add_subdirectory(trainingApp)
add_subdirectory(datasetRecorder)
add_subdirectory(ekfHexa)
add_subdirectory(calibrationApp)
add_subdirectory(localizationGroundTruth)