type cell =
  | O
  | X;

type row = array(cell);

type grid = array(row);

type block = array(row);

type blockPosition = {
  x: int,
  y: int,
};

type gridState = {
  block : block,
  blockPosition: blockPosition,
  grid: grid,
};

type blockSize = {
  width: int,
  height: int,
};

type gameOver = bool;

type gameState = {
  block,
  gridState,
  gameOver
};