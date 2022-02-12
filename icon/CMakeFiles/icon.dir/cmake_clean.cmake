file(REMOVE_RECURSE
  "libicon.a"
  "libicon.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/icon.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
