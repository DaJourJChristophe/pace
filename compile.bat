@echo off

g++ -Iinclude -Ilib/xxHash -std=c++20 -ggdb3 -O0 -march=native -Wall -Wextra -Werror^
  -fno-omit-frame-pointer -fno-optimize-sibling-calls -o bin/main src/main.cc^
  src/context.cc src/event.cc src/map.cc src/profiler.cc src/trie.cc -ldbghelp -limagehlp


g++ -Iinclude -Ilib/xxHash -std=c++20 -ggdb3 -O0 -march=native -Wall -Wextra^
  -Werror -fno-omit-frame-pointer -fno-optimize-sibling-calls -o bin/test_map^
  src/map.cc test/test_map.cc

g++ -Iinclude -Ilib/xxHash -std=c++20 -ggdb3 -O0 -march=native -Wall -Wextra^
  -Werror -fno-omit-frame-pointer -fno-optimize-sibling-calls -o bin/test_queue^
  test/test_queue.cc

g++ -Iinclude -std=c++20 -ggdb3 -O0 -march=native -Wall -Wextra -Werror^
  -fno-omit-frame-pointer -fno-optimize-sibling-calls -o bin/test_stack^
  test/test_stack.cc

g++ -Iinclude -Ilib/xxHash -std=c++20 -ggdb3 -O0 -march=native -Wall -Wextra -Werror^
  -fno-omit-frame-pointer -fno-optimize-sibling-calls -o bin/test_trie^
  src/map.cc src/trie.cc test/test_trie.cc


g++ -Iinclude -Ilib/xxHash -std=c++20 -s -O3 -march=native -Wall -Wextra^
  -Werror -fno-omit-frame-pointer -fno-optimize-sibling-calls^
  -o bin/BENCHMARK/test_map BENCHMARK/test_map.cc src/map.cc

g++ -Iinclude -Ilib/xxHash -std=c++20 -s -O3 -march=native -Wall -Wextra^
  -Werror -fno-omit-frame-pointer -fno-optimize-sibling-calls^
  -o bin/BENCHMARK/test_queue BENCHMARK/test_queue.cc

g++ -Iinclude -std=c++20 -s -O3 -march=native -Wall -Wextra -Werror^
  -fno-omit-frame-pointer -fno-optimize-sibling-calls -o bin/BENCHMARK/test_stack^
  BENCHMARK/test_stack.cc

g++ -Iinclude -Ilib/xxHash -std=c++20 -s -O3 -march=native -Wall -Wextra -Werror^
  -fno-omit-frame-pointer -fno-optimize-sibling-calls -o bin/BENCHMARK/test_trie^
  BENCHMARK/test_trie.cc src/map.cc src/trie.cc
