# Example applications. Each is a single self-contained console program that
# demonstrates a set of TinyTRL features and links against the TinyTRL library.

set(TINYTRL_EXAMPLES
  Arrays
  FlatMapsAndSets
  Streams)

foreach(example IN LISTS TINYTRL_EXAMPLES)
  add_executable(${example} "src/${example}.cpp")
  target_link_libraries(${example} PRIVATE TinyTRL::TinyTRL)
endforeach()
