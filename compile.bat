@echo off

g++ -Iinclude -std=c++20 -ggdb3 -O0 -march=native -Wall -Wextra -Werror -fno-omit-frame-pointer -fno-optimize-sibling-calls -o bin/main src/main.cc src/trie.cc -ldbghelp -limagehlp

g++ -Iinclude -Ilib/xxHash -std=c++20 -ggdb3 -O0 -march=native -Wall -Wextra -Werror -fno-omit-frame-pointer -fno-optimize-sibling-calls -o bin/test_map test/test_map.cc
g++ -Iinclude -std=c++20 -ggdb3 -O0 -march=native -Wall -Wextra -Werror -fno-omit-frame-pointer -fno-optimize-sibling-calls -o bin/test_trie src/trie.cc test/test_trie.cc

g++ -Iinclude -std=c++20 -ggdb3 -O0 -march=native -Wall -Wextra -Werror -fno-omit-frame-pointer -fno-optimize-sibling-calls -o bin/BENCHMARK/test_trie BENCHMARK/test_trie.cc src/trie.cc
