/*
  Copyright 2023 Ben North <ben@redfrontdoor.org>

  This program is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General Public License along
  with this program. If not, see <https://www.gnu.org/licenses/>.

  ------------------------------------------------------------------------

  Compile with

  g++ -std=c++17 -O3 -march=native -o solve-hex solve-hex.cpp

  And then run with no command-line arguments to see the different solution
  strategies which can be attempted.  Very slow strategies are halted after
  a fixed number of trials.  Very quick strategies are run more than once.
  See code below for details.
*/

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <numeric>
#include <vector>
#include <iostream>
#include <string>

static const size_t N_Hexes = 19;
static const uint8_t Required_Sum = 38;

using VecUInt8 = std::vector<uint8_t>;
using ConsumeVecFun = std::function<void(const VecUInt8 &)>;

using ArrUInt8 = std::array<uint8_t, N_Hexes>;
using ConsumeArrFun = std::function<void(const ArrUInt8 &)>;

/*
  Raster order:

          0     1     2
       3     4     5     6
    7     8     9    10    11
      12    13    14    15
         16    17    18

  Spiral order:

          0     1     2
      11    12    13     3
   10    17    18    14     4
       9    16    15     5
          8     7     6
*/

template<typename NumbersT, typename... Ts>
inline bool sum_correct(const NumbersT & board, Ts... idxs)
{
  return (board[idxs] + ...) == Required_Sum;
}

static size_t n_attempts = 0;
static size_t n_attempts_log_period = 10000000;
static size_t n_attempts_bail = 100000000;

struct CheckHardcoded
{
  template<typename NumbersT>
  static bool is_solution(const NumbersT & board)
  {
    ++n_attempts;

    if (n_attempts % n_attempts_log_period == 0) {
      std::cout << n_attempts << " attempts\n";
    }

    if (n_attempts == n_attempts_bail) {
      std::cout << "stopping\n";
      std::exit(0);
    }

    return (sum_correct(board, 0, 1, 2)
            && sum_correct(board, 3, 4, 5, 6)
            && sum_correct(board, 7, 8, 9, 10, 11)
            && sum_correct(board, 12, 13, 14, 15)
            && sum_correct(board, 16, 17, 18)
            && sum_correct(board, 0, 3, 7)
            && sum_correct(board, 1, 4, 8, 12)
            && sum_correct(board, 2, 5, 9, 13, 16)
            && sum_correct(board, 6, 10, 14, 17)
            && sum_correct(board, 11, 15, 18)
            && sum_correct(board, 2, 6, 11)
            && sum_correct(board, 1, 5, 10, 15)
            && sum_correct(board, 0, 4, 9, 14, 18)
            && sum_correct(board, 3, 8, 13, 17)
            && sum_correct(board, 7, 12, 16));
  }
};

bool spiral_solution_is_correct(const ArrUInt8 & board)
{
  return (sum_correct(board, 0, 1, 2)
          && sum_correct(board, 11, 12, 13, 3)
          && sum_correct(board, 10, 17, 18, 14, 4)
          && sum_correct(board, 9, 16, 15, 5)
          && sum_correct(board, 8, 7, 6)
          && sum_correct(board, 0, 11, 10)
          && sum_correct(board, 1, 12, 17, 9)
          && sum_correct(board, 2, 13, 18, 16, 8)
          && sum_correct(board, 3, 14, 15, 7)
          && sum_correct(board, 4, 5, 6)
          && sum_correct(board, 2, 3, 4)
          && sum_correct(board, 1, 13, 14, 5)
          && sum_correct(board, 0, 12, 18, 15, 6)
          && sum_correct(board, 11, 17, 16, 7)
          && sum_correct(board, 10, 9, 8));
}

static const std::vector<std::vector<size_t>> hex_lines = {
  {0, 1, 2},
  {3, 4, 5, 6},
  {7, 8, 9, 10, 11},
  {12, 13, 14, 15},
  {16, 17, 18},
  {0, 3, 7},
  {1, 4, 8, 12},
  {2, 5, 9, 13, 16},
  {6, 10, 14, 17},
  {11, 15, 18},
  {2, 6, 11},
  {1, 5, 10, 15},
  {0, 4, 9, 14, 18},
  {3, 8, 13, 17},
  {7, 12, 16},
};

struct CheckVecOfVecs
{
  static bool is_solution(const VecUInt8 & soln)
  {
    ++n_attempts;

    if (n_attempts % n_attempts_log_period == 0) {
      std::cout << n_attempts << " attempts\n";
    }

    if (n_attempts == n_attempts_bail) {
      std::cout << "stopping\n";
      std::exit(0);
    }

    for (const auto & line : hex_lines) {
      int sum = 0;
      for (const auto idx : line)
        sum += soln[idx];
      if (sum != Required_Sum)
        return false;
    }

    return true;
  }
};

enum class FillOrder { Raster, Spiral };

struct BoardState
{
  VecUInt8 board;
  VecUInt8 available;
  ConsumeVecFun consume_fun;

  BoardState(ConsumeVecFun consume_fun)
    : available(N_Hexes)
    , consume_fun(consume_fun)
  {
    std::iota(available.begin(), available.end(), 1);
  }

  BoardState(const BoardState & rhs)
    : board(rhs.board)
    , available(rhs.available)
    , consume_fun(rhs.consume_fun)
  {
  }

  template<typename Check>
  void solve_check_when_full()
  {
    if (available.size() == 0
        && Check::is_solution(board)) {
      consume_fun(board);
    }
    else {
      const auto n_available = available.size();
      for (size_t idx = 0; idx != n_available; ++idx) {
        auto new_board_state = BoardState{*this};
        new_board_state.board.push_back(available[idx]);
        new_board_state.available.erase(
          new_board_state.available.begin() + idx
        );
        new_board_state.solve_check_when_full<Check>();
      }
    }
  }

  template<FillOrder FO>
  bool incorrect_already();

  template<FillOrder FO>
  void solve_test_as_lines_filled()
  {
    if (incorrect_already<FO>())
      return;

    const auto n_available = available.size();
    if (n_available == 0) {
      consume_fun(board);
      return;
    }

    for (size_t idx = 0; idx != n_available; ++idx) {
      auto new_board_state = BoardState{*this};
      new_board_state.board.push_back(available[idx]);
      new_board_state.available.erase(new_board_state.available.begin() + idx);
      new_board_state.solve_test_as_lines_filled<FO>();
    }
  }

  void move_to_board(size_t available_idx)
  {
    board.push_back(available[available_idx]);
    available.erase(available.begin() + available_idx);
  }

  void choose()
  {
    for (size_t i = 0; i != available.size(); ++i) {
      BoardState new_state{*this};
      new_state.move_to_board(i);
      new_state.solve_deduce_last_cell_of_line();
    }
  }

  template<typename... Ts>
  void deduce(Ts... have_idxs)
  {
    uint8_t needed = Required_Sum - (board[have_idxs] + ...);
    auto maybe_found = std::find(available.begin(), available.end(), needed);
    if (maybe_found != available.end()) {
      size_t needed_idx = maybe_found - available.begin();

      BoardState new_state{*this};
      new_state.move_to_board(needed_idx);
      new_state.solve_deduce_last_cell_of_line();
    }
  }

  void solve_deduce_last_cell_of_line()
  {
    switch (board.size()) {
    case 0: case 1: case 3: case 5: case 7: case 9: case 12: choose(); break;
    case 2: deduce(0, 1); break;
    case 4: deduce(2, 3); break;
    case 6: deduce(4, 5); break;
    case 8: deduce(7, 6); break;
    case 10: deduce(8, 9); break;
    case 11: deduce(0, 10); break;
    case 13: deduce(3, 11, 12); break;
    case 14: deduce(1, 5, 13); break;
    case 15: deduce(3, 7, 14); break;
    case 16: deduce(5, 9, 15); break;
    case 17: deduce(7, 11, 16); break;
    case 18: deduce(4, 10, 14, 17); break;
    case 19:
      if (CheckHardcoded::is_solution(board))
        consume_fun(board);
      break;
    }
  }
};

template<>
bool BoardState::incorrect_already<FillOrder::Raster>()
{
  const size_t n_filled = board.size();
  return ((n_filled == 3 && !sum_correct(board, 0, 1, 2))
          || (n_filled == 7 && !sum_correct(board, 3, 4, 5, 6))
          || (n_filled == 8 && !sum_correct(board, 0, 3, 7))
          || (n_filled == 12 && !sum_correct(board, 7, 8, 9, 10, 11))
          || (n_filled == 12 && !sum_correct(board, 2, 6, 11))
          || (n_filled == 13 && !sum_correct(board, 1, 4, 8, 12))
          || (n_filled == 16 && !sum_correct(board, 12, 13, 14, 15))
          || (n_filled == 16 && !sum_correct(board, 1, 5, 10, 15))
          || (n_filled == 17 && !sum_correct(board, 7, 12, 16))
          || (n_filled == 17 && !sum_correct(board, 2, 5, 9, 13, 16))
          || (n_filled == 18 && !sum_correct(board, 3, 8, 13, 17))
          || (n_filled == 18 && !sum_correct(board, 6, 10, 14, 17))
          || (n_filled == 19 && !sum_correct(board, 0, 4, 9, 14, 18))
          || (n_filled == 19 && !sum_correct(board, 11, 15, 18))
          || (n_filled == 19 && !sum_correct(board, 16, 17, 18)));
}

template<>
bool BoardState::incorrect_already<FillOrder::Spiral>()
{
  const size_t n_filled = board.size();
  return ((n_filled == 3 && !sum_correct(board, 0, 1, 2))
          || (n_filled == 5 && !sum_correct(board, 2, 3, 4))
          || (n_filled == 7 && !sum_correct(board, 4, 5, 6))
          || (n_filled == 9 && !sum_correct(board, 6, 7, 8))
          || (n_filled == 11 && !sum_correct(board, 8, 9, 10))
          || (n_filled == 12 && !sum_correct(board, 0, 10, 11))
          || (n_filled == 14 && !sum_correct(board, 3, 11, 12, 13))
          || (n_filled == 15 && !sum_correct(board, 1, 13, 14, 5))
          || (n_filled == 16 && !sum_correct(board, 3, 14, 15, 7))
          || (n_filled == 17 && !sum_correct(board, 5, 15, 16, 9))
          || (n_filled == 18 && !sum_correct(board, 7, 16, 17, 11))
          || (n_filled == 18 && !sum_correct(board, 1, 12, 17, 9))
          || (n_filled == 19 && !sum_correct(board, 0, 12, 18, 15, 6))
          || (n_filled == 19 && !sum_correct(board, 2, 13, 18, 16, 8))
          || (n_filled == 19 && !sum_correct(board, 4, 14, 18, 17, 10)));
}

enum struct RecursionStrategy { CreateNew, SwapAndSwapBack };

struct ArrayBoardState
{
  ArrUInt8 numbers;
  size_t n_cells_filled;
  ConsumeArrFun consume_fun;

  ArrayBoardState(ConsumeArrFun consume_fun)
    : n_cells_filled(0)
    , consume_fun(consume_fun)
  {
    std::iota(numbers.begin(), numbers.end(), 1);
  }

  ArrayBoardState(const ArrayBoardState & other)
    : numbers(other.numbers)
    , n_cells_filled(other.n_cells_filled)
    , consume_fun(other.consume_fun)
  {
  }

  size_t find_needed(uint8_t needed_value)
  {
    for (size_t i = n_cells_filled; i != N_Hexes; ++i) {
      if (numbers[i] == needed_value) {
        return i;
      }
    }
    return N_Hexes;
  }

  template<RecursionStrategy RS>
  void choose();

  template<typename... Ts>
  size_t find_needed_from_idxs(Ts... have_idxs)
  {
    uint8_t needed = Required_Sum - (numbers[have_idxs] + ...);
    return find_needed(needed);
  }

  template<RecursionStrategy RS>
  void deduce(size_t i1, size_t i2)
  {
    deduce_from_idx<RS>(find_needed_from_idxs(i1, i2));
  }

  template<RecursionStrategy RS>
  void deduce(size_t i1, size_t i2, size_t i3)
  {
    deduce_from_idx<RS>(find_needed_from_idxs(i1, i2, i3));
  }

  template<RecursionStrategy RS>
  void deduce(size_t i1, size_t i2, size_t i3, size_t i4)
  {
    deduce_from_idx<RS>(find_needed_from_idxs(i1, i2, i3, i4));
  }

  template<RecursionStrategy RS>
  void deduce_from_idx(size_t maybe_needed_idx);

  template<RecursionStrategy RS>
  void solve()
  {
    switch (n_cells_filled) {
    case 0: case 1: case 3: case 5: case 7: case 9: case 12: choose<RS>(); break;
    case 2: deduce<RS>(0, 1); break;
    case 4: deduce<RS>(2, 3); break;
    case 6: deduce<RS>(4, 5); break;
    case 8: deduce<RS>(7, 6); break;
    case 10: deduce<RS>(8, 9); break;
    case 11: deduce<RS>(0, 10); break;
    case 13: deduce<RS>(3, 11, 12); break;
    case 14: deduce<RS>(1, 5, 13); break;
    case 15: deduce<RS>(3, 7, 14); break;
    case 16: deduce<RS>(5, 9, 15); break;
    case 17: deduce<RS>(7, 11, 16); break;
    case 18: deduce<RS>(4, 10, 14, 17); break;
    case 19:
      if (spiral_solution_is_correct(numbers))
        consume_fun(numbers);
      break;
    }
  }
};

template<>
void ArrayBoardState::choose<RecursionStrategy::CreateNew>()
{
  ++n_cells_filled;
  solve<RecursionStrategy::CreateNew>();
  --n_cells_filled;
  for (size_t i = n_cells_filled + 1; i != N_Hexes; ++i) {
    ArrayBoardState swapped_state{*this};
    std::swap(swapped_state.numbers[n_cells_filled], swapped_state.numbers[i]);
    ++swapped_state.n_cells_filled;
    swapped_state.solve<RecursionStrategy::CreateNew>();
  }
}

template<>
void ArrayBoardState::deduce_from_idx<RecursionStrategy::CreateNew>(
  size_t maybe_needed_idx
) {
  if (maybe_needed_idx != N_Hexes) {
    ArrayBoardState swapped_state{*this};
    std::swap(swapped_state.numbers[n_cells_filled], swapped_state.numbers[maybe_needed_idx]);
    ++swapped_state.n_cells_filled;
    swapped_state.solve<RecursionStrategy::CreateNew>();
  }
}

template<>
void ArrayBoardState::choose<RecursionStrategy::SwapAndSwapBack>()
{
  ++n_cells_filled;
  solve<RecursionStrategy::SwapAndSwapBack>();
  --n_cells_filled;
  for (size_t i = n_cells_filled + 1; i != N_Hexes; ++i) {
    std::swap(numbers[n_cells_filled], numbers[i]);
    ++n_cells_filled;
    solve<RecursionStrategy::SwapAndSwapBack>();
    --n_cells_filled;
    std::swap(numbers[n_cells_filled], numbers[i]);
  }
}

template<>
void ArrayBoardState::deduce_from_idx<RecursionStrategy::SwapAndSwapBack>(
  size_t maybe_needed_idx
) {
  if (maybe_needed_idx != N_Hexes) {
    std::swap(numbers[n_cells_filled], numbers[maybe_needed_idx]);
    ++n_cells_filled;
    solve<RecursionStrategy::SwapAndSwapBack>();
    --n_cells_filled;
    std::swap(numbers[n_cells_filled], numbers[maybe_needed_idx]);
  }
}

template<typename T>
void ignore(const T & /* board */)
{
}

template<typename T>
void dump(const T & board)
{
  std::cout << "HEX:";
  for (const uint8_t n : board)
    std::cout << " " << static_cast<int>(n);
  std::cout << "\n";
}

template<typename Check>
void solve_manual_perm()
{
  BoardState{dump<VecUInt8>}.solve_check_when_full<Check>();
}

template<typename Check>
void solve_std_perm()
{
  n_attempts_log_period = 250000000;
  n_attempts_bail = 2500000000;

  VecUInt8 board(N_Hexes);
  std::iota(board.begin(), board.end(), 1);

  while (true) {
    if (Check::is_solution(board))
      dump(board);
    if ( ! std::next_permutation(board.begin(), board.end()))
      break;
  }
}

template<FillOrder FO>
void solve_test_line_by_line()
{
  BoardState{dump<VecUInt8>}.solve_test_as_lines_filled<FO>();
}

void solve_deduce_last_cell_of_line()
{
  for (size_t i = 0; i != 99; ++i)
    BoardState{ignore<VecUInt8>}.solve_deduce_last_cell_of_line();

  BoardState{dump<VecUInt8>}.solve_deduce_last_cell_of_line();
}

template<RecursionStrategy RS>
void solve_deduce_last_cell_of_line_array()
{
  for (size_t i = 0; i != 99; ++i)
    ArrayBoardState{ignore<ArrUInt8>}.solve<RS>();

  ArrayBoardState{dump<ArrUInt8>}.solve<RS>();
}

struct StrategyOption
{
  std::string arg;
  std::string summary;
  std::function<void(void)> solve;
};

static const std::vector<StrategyOption>
strategies{
  {
    "manual-perm-vec-vecs-check",
    R"(
    Manually generate all permutations of values into cells, checking
    once the board is completely filled whether it is a solution,
    using a vector of vectors of indexes to encode the lines.
    )",
    solve_manual_perm<CheckVecOfVecs>
  },
  {
    "manual-perm-hardcoded-check",
    R"(
    As "manual-perm-vec-vecs-check", except check whether a board is a
    solution using a hard-coded list of"if" statements, one per line.
    )",
    solve_manual_perm<CheckHardcoded>
  },
  {
    "stdlib-perm-vec-vecs-check",
    R"(
    As "manual-perm-vec-vecs-check", except iterate over the
    permutations using the standard library next_permutation()
    function.
    )",
    solve_std_perm<CheckVecOfVecs>},
  {
    "stdlib-perm-hardcoded-check",
    R"(
    As "manual-perm-hardcoded-check", except iterate over the
    permutations using the standard library next_permutation()
    function.
    )",
    solve_std_perm<CheckHardcoded>
  },
  {
    "line-by-line-check",
    R"(
    As soon as any line is filled, check whether that line has the
    correct sum, and abandon the exploration if not.  Fill the cells
    in raster order.
    )",
    solve_test_line_by_line<FillOrder::Raster>
  },
  {
    "line-by-line-check-spiral",
    R"(
    As "line-by-line-check", except fill the cells in an inwards
    spiral order.
    )",
    solve_test_line_by_line<FillOrder::Spiral>
  },
  {
    "deduce",
    R"(
    When filling in a cell which will complete a line, work out what
    value has to be used to give the correct sum, and search for it
    in the collection of available numbers.  Abandon the exploration
    if it is not available.  Fill the cells in an inwards spiral order.
    )",
    solve_deduce_last_cell_of_line
  },
  {
    "deduce-array",
    R"(
    As "deduce", except store the partially-filled board and the set
    of available numbers in one 19-element array.
    )",
    solve_deduce_last_cell_of_line_array<RecursionStrategy::CreateNew>
  },
  {
    "deduce-array-swap",
    R"(
    As "deduce-array", except instead of cloning the solver to explore
    a possibility or deduction, swap elements, explore, then swap back.
    )",
    solve_deduce_last_cell_of_line_array<RecursionStrategy::SwapAndSwapBack>
  },
};

int main(int argc, char ** argv)
{
  if (argc == 2) {
    std::string strategy{argv[1]};
    for (const auto & strat : strategies) {
      if (strategy == strat.arg) {
        strat.solve();
        return 0;
      }
    }
  }

  std::cerr << "Bad strategy label: allowed values:\n";
  for (const auto & strat : strategies) {
    std::cerr << "\n" << strat.arg << strat.summary << "\n";
  }

  return 1;
}
