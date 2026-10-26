add_executable(effect_emission_tests effect_emission_test.cc)
target_compile_features(effect_emission_tests PRIVATE cxx_std_17)
target_include_directories(effect_emission_tests PRIVATE ../../src/scene)
add_test(NAME effect_emission_tests COMMAND effect_emission_tests)
