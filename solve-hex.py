from dataclasses import dataclass
from typing import List
from collections.abc import Callable
import sys

N_Hexes = 19
Required_Sum = 38

"""
          0     1     2
       3     4     5     6
    7     8     9    10    11
      12    13    14    15
         16    17    18
"""

Lines = [
    [0, 1, 2],
    [3, 4, 5, 6],
    [7, 8, 9, 10, 11],
    [12, 13, 14, 15],
    [16, 17, 18],
    [0, 3, 7],
    [1, 4, 8, 12],
    [2, 5, 9, 13, 16],
    [6, 10, 14, 17],
    [11, 15, 18],
    [2, 6, 11],
    [1, 5, 10, 15],
    [0, 4, 9, 14, 18],
    [3, 8, 13, 17],
    [7, 12, 16],
]

n_attempts = 0
def arrangement_is_solution(board):
    global n_attempts
    n_attempts += 1
    if n_attempts == 5000000:
        sys.exit(0)
    for line_idxs in Lines:
        if sum(board[i] for i in line_idxs) != Required_Sum:
            return False
    return True

@dataclass
class BoardState:
    partial_board: List[int]
    available_numbers: List[int]
    consume_fun: Callable[["BoardState"], None]

    @classmethod
    def new_initial(cls, consume_fun):
        return cls(
            [],
            list(range(1, N_Hexes + 1)),
            consume_fun,
        )

    def clone(self):
        return self.__class__(
            self.partial_board[:],
            self.available_numbers[:],
            self.consume_fun,
        )

    def solve(self):
        if (not self.available_numbers
            and arrangement_is_solution(self.partial_board)):
            self.consume_fun(self.partial_board)
        else:
            for idx, num in enumerate(self.available_numbers):
                new_board_state = self.clone()
                new_board_state.partial_board.append(num)
                del new_board_state.available_numbers[idx]
                new_board_state.solve()

    def dump(self):
        if self.available_numbers:
            raise RuntimeError("cannot dump unfinished board")
        print("HEX: " + " ".join(str(n) for n in self.partial_board))


bs = BoardState.new_initial(BoardState.dump)
bs.solve()
