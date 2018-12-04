# node-ygocore

**[WIP]** node bindings for ygopro-core (https://github.com/moecube/ygopro-core)

# Install

```
npm install ygocore
```

# How to use?

``` typescript
import { engine } from 'ygocore';

// 1. load card data from ygopro's cards.cdb
//    and register them to the core engine
const cards = await loadAllCards();
for (const card of cards) {
  engine.registerCard(card);
}

// 2. load necessary scripts from ygopro's script
//    folder and register them to the engine
const scripts = await loadAllScripts();
for (const [ name, content ] of scripts) {
  engine.registerScript(name, content);
}

// 3. create a duel instance
const duel = engine.createDuel(0 /* any random seed */);

// 4. where are the duelists?
engine.setPlayerInfo(duel, { player: 0, lp: 8000, start: 5, draw: 1 });
engine.setPlayerInfo(duel, { player: 1, lp: 8000, start: 5, draw: 1 });

// 5. and setup their decks
for (const playerId of [0, 1]) {
  for (const code of players[playerId].deck.main) {
    engine.newStartupCardMain(duel, playerId, code);
  }

  for (const code of players[playerId].deck.extra) {
    engine.newStartupCardExtra(duel, playerId, code);
  }
}

// 6. start the duel
engine.startDuel(duel, options);

// 7. process each move
while (true) {
  const { messages } = engine.process(duel);
  for (const m of messages) {
   
    handleMessage(m);
    // set response use:
    engine.setResponse(duel, userResponse);
  }

 if (messages.some(m => m.message === 'MSG_WIN')) {
    // we've got a winner!
    break;
  }

 if (messages.some(m => m.message === 'MSG_RETRY')) {
    // something went wrong...
    break;
  }
}

// 8. cleanup
engine.endDuel(duel);

```
