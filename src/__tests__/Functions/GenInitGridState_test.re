open Jest;
open Expect;
open Types;
open Functions;

describe("GenInitGridState", () => {
  let state = genInitGridState(~gridWidth=10, ~gridHeight=23);
  let {grid} = state;

  test("Correct grid size", () => {
    let width = Array.length(grid[0]);
    let height = Array.length(grid);

    expect((width, height)) |> toEqual((10, 23));
  });
});
