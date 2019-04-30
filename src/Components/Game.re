module RR = ReasonReact;
module Func = Functions;
module Document = Webapi.Dom.Document;
module Const = Constants;
open Types;

[@bs.val] external document: Dom.document = "document";

[%bs.raw {|require('./game.css')|}];

let initGridState = Func.genInitGridState(~gridWidth=10, ~gridHeight=22);

let initTimerIds = {
  tick: ref(None),
  countdown: ref(None),
  moveLeft: ref(None),
  moveRight: ref(None),
  rotate: ref(None),
};

let initGlobalState: globalState = {
  gridState: initGridState,
  nextBlock: Blocks.getRandomBlock(),
  countdownCounter: 0,
  stats: Const.initStats,
  timerIds: initTimerIds,
  counting: false,
  started: false,
  gameOver: false,
  screen: Title,
  handleKeyboard: ref(() => ()),
  unHandleKeyboard: ref(() => ()),
};

type state = globalState;

type action =
  | StartGame
  | StartCountdown
  | Countdown
  | Tick
  | MoveBlock(direction)
  | RotateBlock
  | UpdateLevel;

let getNextScreen = currentScreen => {
  switch (currentScreen) {
  | Title => Counter
  | Counter => Game
  | Game => Gameover
  | Gameover => Counter
  };
};

type self = RR.self(state, RR.noRetainedProps, action);

let keyDownToAction = (event, self: self) => {
  let key = Utils.Dom.getKeyName(event);
  let repeated = Utils.Dom.isKeyRepeated(event);

  let setIntervalForAction = (action, timerId, delay) => {
    Utils.clearIntervalId(timerId);
    let id = Js.Global.setInterval(() => self.RR.send(action), delay);
    timerId := Some(id);
  };

  if (repeated) {
    ();
  } else {
    switch (key) {
    | "ArrowDown" =>
      let tickId = self.state.timerIds.tick;
      setIntervalForAction(Tick, tickId, Const.dropDelay);

    | "ArrowLeft" =>
      let moveLeftId = self.state.timerIds.moveLeft;
      setIntervalForAction(MoveBlock(Left), moveLeftId, Const.moveDelay);

    | "ArrowRight" =>
      let moveRightId = self.state.timerIds.moveRight;
      setIntervalForAction(MoveBlock(Right), moveRightId, Const.moveDelay);

    | "ArrowUp" =>
      let rotateId = self.state.timerIds.rotate;
      setIntervalForAction(RotateBlock, rotateId, Const.rotateDelay);

    | _ => ()
    };
  };
};

let keyUpToAction = (event, self: self) => {
  let key = Utils.Dom.getKeyName(event);

  switch (key) {
  | "ArrowLeft" => Utils.clearIntervalId(self.state.timerIds.moveLeft)
  | "ArrowRight" => Utils.clearIntervalId(self.state.timerIds.moveRight)
  | "ArrowUp" => Utils.clearIntervalId(self.state.timerIds.rotate)
  | "ArrowDown" =>
    let timerId = self.state.timerIds.tick;
    Utils.clearIntervalId(timerId);

    let delay = Func.calcDelay(self.state.stats.level);
    let intervalId = Js.Global.setInterval(() => self.send(Tick), delay);
    timerId := Some(intervalId);
  | _ => ()
  };
};

let component = RR.reducerComponent("Game");

let make = _children => {
  ...component,
  initialState: () => initGlobalState,

  didMount: self => {
    let keyDownHandler = self.handle(keyDownToAction);
    let keyUpHandler = self.handle(keyUpToAction);

    let addKeyboardListeners = () => {
      Document.addKeyDownEventListener(keyDownHandler, document);
      Document.addKeyUpEventListener(keyUpHandler, document);
    };

    let removeKeyboardListeners = () => {
      Document.removeKeyDownEventListener(keyDownHandler, document);
      Document.removeKeyUpEventListener(keyUpHandler, document);
    };

    self.state.handleKeyboard := addKeyboardListeners;
    self.state.unHandleKeyboard := removeKeyboardListeners;

    self.onUnmount(() => self.state.unHandleKeyboard^());
  },

  reducer: (action, state) =>
    switch (action) {
    | StartGame =>
      let delay = Func.calcDelay(state.stats.level);

      UpdateWithSideEffects(
        {...state, started: true},
        self => {
          let timerId = state.timerIds.tick;
          let intervalId =
            Js.Global.setInterval(() => self.send(Tick), delay);
          timerId := Some(intervalId);
          self.state.handleKeyboard^();
        },
      );

    | StartCountdown =>
      UpdateWithSideEffects(
        {
          ...initGlobalState,
          countdownCounter: 3,
          screen: getNextScreen(state.screen),
        },
        self => {
          let timerId = state.timerIds.countdown;
          let countdownId =
            Js.Global.setInterval(() => self.send(Countdown), 1000);
          timerId := Some(countdownId);
        },
      )

    | Countdown =>
      let counter = state.countdownCounter;

      if (counter > 1) {
        Update({...state, countdownCounter: state.countdownCounter - 1});
      } else {
        UpdateWithSideEffects(
          {
            ...state,
            countdownCounter: 0,
            screen: getNextScreen(state.screen),
          },
          self => {
            Utils.clearIntervalId(state.timerIds.countdown);
            self.send(StartGame);
          },
        );
      };

    | MoveBlock(direction) =>
      Update({
        ...state,
        gridState: Func.getGridStateAfterMove(direction, state.gridState),
      })

    | RotateBlock =>
      Update({
        ...state,
        gridState: Func.getGridStateAfterRotate(state.gridState),
      })

    | UpdateLevel =>
      let delay = Func.calcDelay(state.stats.level);
      let tickId = state.timerIds.tick;
      Utils.clearIntervalId(tickId);

      SideEffects(
        self => {
          let intervalId =
            Js.Global.setInterval(() => self.send(Tick), delay);
          tickId := Some(intervalId);
        },
      );

    | Tick =>
      let next =
        Func.tick(
          state.gridState,
          state.stats,
          ~nextBlock=state.nextBlock,
          (),
        );

      let {gridState, stats, gameOver, nextBlockToShow} = next;

      if (gameOver) {
        UpdateWithSideEffects(
          {...state, gameOver: true, screen: getNextScreen(state.screen)},
          _self => {
            state.unHandleKeyboard^();
            Utils.clearIntervalId(state.timerIds.tick);
          },
        );
      } else {
        let nextState = {
          ...state,
          nextBlock: nextBlockToShow,
          gridState,
          stats,
        };

        if (state.stats.level < next.stats.level) {
          UpdateWithSideEffects(nextState, self => self.send(UpdateLevel));
        } else {
          Update(nextState);
        };
      };
    },

  render: ({send, state}) => {
    let {nextBlock, gridState, countdownCounter, stats, started, screen} = state;

    let gridToRender =
      started ? Func.mapBlockToGrid(gridState) : initGridState.grid;

    <div className="game">
      <NextBlock nextBlock started />
      <div className="gridContainer">
        <Grid grid=gridToRender />
        <GridOverlay
          screen
          countdownCounter
          clickStart={_event => send(StartCountdown)}
        />
      </div>
      <Stats stats started />
    </div>;
  },
};
